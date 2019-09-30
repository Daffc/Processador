#include "simulator.hpp"

int64_t contador_branch, contador_miss_BTB, contador_misprediction, contador_predictions;

BTB_t *btb;
Predictor *predictor;
char  delay;


/// Get the next instruction from the trace
opcode_package_t *new_instruction, *next_instruction;

// Se o valor da direction_PHT selectionado pelo choice_PHD for igual a WNTAKEN ou SNTAKEN, return taken.
bool prediction(Predictor* predict, uint64_t address){

	uint16_t index_choice, index_direction;
	unsigned char choice_bits, direction_bits;
	
	// Calculating indexes.
	index_choice = address & 1023;
	index_direction = index_choice ^ predict->global_history;
	index_choice = index_choice ^ predict->global_history;

	choice_bits = predict->choice_predictor[index_choice];
	direction_bits = predict->direction_predictors[(OFFSET * (choice_bits / 2)) + index_direction];

	// ORCS_PRINTF("choice_bits:%d\n", choice_bits);
	// ORCS_PRINTF("Taken? :%d\n", OFFSET * (choice_bits / 2));
	// ORCS_PRINTF("direction_bits:%d\n", direction_bits);

	if (direction_bits < WNTAKEN){
		return true;		
	}
	return false;
}

// Adjust global_history, direction_predictor[selected], choice_predictor;
void adjust(Predictor * predict, int adjustment, uint64_t address, bool hit){
	
	// ORCS_PRINTF("before Global_history = %u \n", predict->global_history);

	uint16_t index_choice, index_direction, direction_position;
	unsigned char choice_bits;

	index_choice = address & 1023;
	index_direction = index_choice ^ predict->global_history;
	index_choice = index_choice ^ predict->global_history;

	choice_bits = predict->choice_predictor[index_choice];

	direction_position = (OFFSET * (choice_bits / 2)) + index_direction;
	
	// ORCS_PRINTF("index_choice = %d\t index_direction = %d\t choice_bits = %d\t global_history = %d\t hit = %d\t adjustment = %d, direction = %d\n" , index_choice, index_direction, predict->choice_predictor[index_choice], predict->global_history, hit, adjustment, predict->direction_predictors[direction_position]);

	// Update "choice_predictor", unless it was a hit but the "choice_predictor" mispredict the direction
	if(!(hit && ((choice_bits <= WTAKEN && adjustment == 1) || ( choice_bits >= WNTAKEN && adjustment == -1)))) {
		predict->choice_predictor[index_choice] = predict->choice_predictor[index_choice] + adjustment;
	}

	//direction[index] = direction[index] + adjustment
	predict->direction_predictors[direction_position] = predict->direction_predictors[direction_position] + adjustment;
	
	// Adjusting "direction_predictions" to the boundaries.
	if(predict->direction_predictors[direction_position] > SNTAKEN){
		predict->direction_predictors[direction_position] = SNTAKEN;
	}
	if(predict->direction_predictors[direction_position] < STAKEN){
		predict->direction_predictors[direction_position] = STAKEN;
	}

	// Adjusting "choice_predictor" to the boundaries.	
	if(predict->choice_predictor[index_choice] > SNTAKEN){
		predict->choice_predictor[index_choice] = SNTAKEN;
	}
	if(predict->choice_predictor[index_choice] < STAKEN){
		predict->choice_predictor[index_choice] = STAKEN;
	}	

	if(hit)
		predict->global_history = ((predict->global_history << 1) + 1) & 1023;
	else
		predict->global_history = (predict->global_history << 1) & 1023;
}
int verification(opcode_package_t* actual, opcode_package_t* next, Predictor* predict, bool decision){
	int penalty;
	uint64_t NT_address;

	// Guarda destino da próxima instrução caso NÃO seja uma branch.
	NT_address = actual->opcode_address + actual->opcode_size;

	contador_predictions++;

	// ORCS_PRINTF("NT_address:\t\t\t\t%" PRIu64 "\n", NT_address);
	// ORCS_PRINTF("next->opcode_address:\t%" PRIu64 "\n", next->opcode_address);
	// ORCS_PRINTF("decision:\t\t\t\t%d\n", decision);


	// IF BRANCH = T
	if(NT_address != next->opcode_address){
		//IF BTH = T
		if(decision == true){
			// ORCS_PRINTF("HIT (T-T)\n");
			penalty = 0;		
			adjust(predict, -1, actual->opcode_address, true);
		}		
		//IF BTH = NT
		else{
			// ORCS_PRINTF("HIT (T-NT)\n");
			penalty = 14;
			// Counting number of misses in BHT.
			contador_misprediction ++;
			adjust(predict, -1, actual->opcode_address, false);
		}
	}
	// IF BRANCH = NT
	else{
		//IF BTH = T
		if(decision == true){
			// ORCS_PRINTF("MISS (NT-T)\n");
			penalty = 14;
			// Counting number of misses in BHT.
			contador_misprediction ++;
			adjust(predict, 1, actual->opcode_address, false);
		}
		//IF BTH = NT
		else{
			// ORCS_PRINTF("HIT (NT-NT)\n");
			penalty = 0;
			adjust(predict, 1, actual->opcode_address, true);
		}			
	}

	// ORCS_PRINTF("\n\n");
	return penalty;
}

// =====================================================================
processor_t::processor_t() {

};

// =====================================================================
void processor_t::allocate() {
	btb = (BTB_t *) malloc(1024 * sizeof(BTB_t));
	predictor = (Predictor *) malloc(sizeof(Predictor));

	memset(btb, 0, 1024 * sizeof(BTB_t));
	memset(predictor, 0, sizeof(Predictor));

	new_instruction = (opcode_package_t *) malloc(sizeof(opcode_package_t));
	next_instruction = (opcode_package_t *) malloc(sizeof(opcode_package_t));

	predictor->direction_predictors = ( char *)malloc(2048 * sizeof(char));
	predictor->choice_predictor = (char *)malloc(1024 * sizeof(char));

	memset(predictor->direction_predictors, 0, 1024 * sizeof(char));
	memset(predictor->direction_predictors + 1024, 3, 1024 * sizeof(char));
	memset(predictor->choice_predictor, 0, 1024 * sizeof(char));

	contador_branch = 0;
	contador_miss_BTB = 0;
	contador_misprediction = 0;
	contador_predictions = 0;
};

// =====================================================================
void processor_t::clock() {

	uint64_t  entry, older, address;
	ushort selected, i;
	opcode_package_t *aux;
	bool taken;
	
	//Apply penalty to the process.
	if(delay){
		delay--;
		return;	
	}
	
	//Pegando primeira instrução, no começo da execução do programa.
	if(orcs_engine.global_cycle == 0){
		next_instruction->end = orcs_engine.trace_reader->trace_fetch(next_instruction);
	}

	// Troca de ponteiros de next -> new;
	aux = new_instruction;
	new_instruction = next_instruction;
	next_instruction = aux;

	//Pega próxima instrunção.
	next_instruction->end = orcs_engine.trace_reader->trace_fetch(next_instruction);

	//Caso todas as instruções tenham sido executadas.
	if (!new_instruction->end ) {
		/// If EOF
		ORCS_PRINTF("CLOCKS = %" PRIu64 "\n", orcs_engine.global_cycle);
		orcs_engine.simulator_alive = false;
	}
	else{
		if(new_instruction->opcode_operation == INSTRUCTION_OPERATION_BRANCH){

			address = new_instruction->opcode_address;
			// Contando número de branches.
			contador_branch ++;

			entry = (address & 1023);
			older = 0xFFFFFFFFFFFFFFFF;
			selected = 0;

			//Verificando se branch está na BTB
			for(i = 0; i < WAYS; i++){
				if(btb[entry].group[i].pc == address){
					btb[entry].group[i].pc = address;
					// Se branch for do tipo condicional, tentar predizer direção.
					btb[entry].group[i].time = orcs_engine.global_cycle;
					if(new_instruction->branch_type == BRANCH_COND){
						taken = prediction(predictor, address);
						delay = verification(new_instruction, next_instruction, predictor, taken);
					}
					break;
				}
				if(older > btb[entry].group[i].time){
					older = btb[entry].group[i].time;
					selected = i;
				}
			}

			// If the branch instruction was not found into BTB.
			if(i == WAYS){
				contador_miss_BTB++;
				btb[entry].group[selected].pc = address;
				btb[entry].group[selected].time = orcs_engine.global_cycle;
					// if(new_instruction->branch_type == BRANCH_COND){
					// 	taken = prediction(predictor, address);
					// 	verification(new_instruction, next_instruction, predictor, taken);
					// }

				delay = 14;
			}
		}
	}
};

// =====================================================================
void processor_t::statistics() {
	ORCS_PRINTF("######################################################\n");
	ORCS_PRINTF("processor_t\n");
	ORCS_PRINTF("Total Branches:%" PRIu64 "\n", contador_branch);
	ORCS_PRINTF("Total BTB misses:%" PRIu64 "\n", contador_miss_BTB);
	ORCS_PRINTF("Total predictions:%" PRIu64 "\n", contador_predictions);
	ORCS_PRINTF("Total mispredictions:%" PRIu64 "\n", contador_misprediction);
	ORCS_PRINTF("branch conditinal HIT: %f\n", 1 - ((contador_misprediction * 1.0)/(contador_predictions * 1.0)));
};
