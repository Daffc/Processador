#include "simulator.hpp"

BTB_t *btb;
char  delay;

/// Get the next instruction from the trace
opcode_package_t *new_instruction, *next_instruction;

// =====================================================================
processor_t::processor_t() {

};

// =====================================================================
void processor_t::allocate() {
	btb = (BTB_t *) malloc(1024 * sizeof(BTB_t));
	memset(btb, 0, 1024 * sizeof(BTB_t));

	new_instruction = (opcode_package_t *) malloc(sizeof(opcode_package_t));
	next_instruction = (opcode_package_t *) malloc(sizeof(opcode_package_t));
};

// =====================================================================
void processor_t::clock() {

	uint64_t  entry, older;
	ushort selected;
	opcode_package_t *aux;
	
	if(delay){
		delay--;
		return;	
	}
	
	//Pegando primeira instrução, no começo da execução do programa.
	if(orcs_engine.global_cycle == 0){
		next_instruction->end = orcs_engine.trace_reader->trace_fetch(next_instruction);
	}

	// Troca de onteiros de next -> new;
	aux = new_instruction;
	new_instruction = next_instruction;
	next_instruction = aux;

	//Pega próxima instrunção.
	next_instruction->end = orcs_engine.trace_reader->trace_fetch(next_instruction);

	//Caso todas as instruções tenham sido executadas.
	if (!new_instruction->end) {
		/// If EOF
		ORCS_PRINTF("CLOCKS = %" PRIu64 "\n", orcs_engine.global_cycle);
		orcs_engine.simulator_alive = false;
	}
	else{
		if(new_instruction->opcode_operation == INSTRUCTION_OPERATION_BRANCH){
			entry = (new_instruction->opcode_address & 1023);
			
			if(btb[entry].group[0].pc == new_instruction->opcode_address){
				btb[entry].group[0].pc = new_instruction->opcode_address;
				btb[entry].group[0].time = orcs_engine.global_cycle;
				
			}
			else{
				older = btb[entry].group[0].time;
				selected = 0;
				if(btb[entry].group[1].pc == new_instruction->opcode_address){
					btb[entry].group[1].pc = new_instruction->opcode_address;
					btb[entry].group[1].time = orcs_engine.global_cycle;
					
				}
				else{
					if(older > btb[entry].group[1].time){
						older = btb[entry].group[1].time;
						selected = 1;
					}
					if(btb[entry].group[2].pc == new_instruction->opcode_address){
						btb[entry].group[2].pc = new_instruction->opcode_address;
						btb[entry].group[2].time = orcs_engine.global_cycle;
						
					}	
					else{
						if(older > btb[entry].group[2].time){
							older = btb[entry].group[2].time;
							selected = 2;
						}
						if(btb[entry].group[3].pc == new_instruction->opcode_address){
							btb[entry].group[3].pc = new_instruction->opcode_address;
							btb[entry].group[3].time = orcs_engine.global_cycle;
							
						}	
						else{
							delay = 14;
							if(older > btb[entry].group[3].time){
								older = btb[entry].group[3].time;
								selected = 3;
							}

							btb[entry].group[selected].pc = new_instruction->opcode_address;
							btb[entry].group[selected].time = orcs_engine.global_cycle;

							ORCS_PRINTF("GRUPO:\t%" PRIu64 "\n", entry);
							ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[entry].group[0].pc, btb[entry].group[0].time);
							ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[entry].group[1].pc, btb[entry].group[1].time);
							ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[entry].group[2].pc, btb[entry].group[2].time);
							ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[entry].group[3].pc, btb[entry].group[3].time);
							ORCS_PRINTF("\n\n\n");
						}
					}
				}
			}	
		}
	}
};

// =====================================================================
void processor_t::statistics() {
	ORCS_PRINTF("######################################################\n");
	ORCS_PRINTF("processor_t\n");

};
