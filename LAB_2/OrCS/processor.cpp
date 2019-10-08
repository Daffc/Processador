#include "simulator.hpp"

// =====================================================================
processor_t::processor_t() {
	L1 = (cache *) malloc(sizeof(cache));
	L2 = (cache *) malloc(sizeof(cache));
	
	L1->initialize(31, 255, 4, 1024, 1);
	L2->initialize(31, 1023, 8, 16384, 4);
};

// =====================================================================
void processor_t::allocate() {
};

// =====================================================================
void processor_t::clock() {

	/// Get the next instruction from the trace
	opcode_package_t new_instruction;
	// uint64_t  group, older;
	// char selected;
	
	if(delay){
		delay--;
		return;	
	}
	if (!orcs_engine.trace_reader->trace_fetch(&new_instruction)) {
		/// If EOF
		ORCS_PRINTF("CLOCKS = %" PRIu64 "\n", orcs_engine.global_cycle);
		orcs_engine.simulator_alive = false;
	}
	// else{
	// 	for(i = 0; i < WAYS; i++){
	// 		if(btb[entry].group[i].pc == address){
	// 			btb[entry].group[i].pc = address;
	// 			// Se branch for do tipo condicional, tentar predizer direção.
	// 			btb[entry].group[i].time = orcs_engine.global_cycle;
	// 			if(new_instruction->branch_type == BRANCH_COND){
	// 				taken = prediction(predictor, address);
	// 				delay = verification(new_instruction, next_instruction, predictor, taken & btb[entry].group[selected].bias);
	// 				if(new_instruction->opcode_address + new_instruction->opcode_size != next_instruction->opcode_address)
	// 					btb[entry].group[selected].bias = 1;
	// 			}
	// 			break;
	// 		}
	// 		if(older > btb[entry].group[i].time){
	// 			older = btb[entry].group[i].time;
	// 			selected = i;
	// 		}
	// 	}
	// 		if(i == 4){
	// 			contador_miss_BTB++;

	// 			btb[entry].group[selected].pc = new_instruction->opcode_address;
	// 			btb[entry].group[selected].time = orcs_engine.global_cycle;
	// 			if(new_instruction->branch_type == BRANCH_COND){
	// 				// taken = prediction(btb[entry].group[selected].BHT);
	// 				// verification(new_instruction, next_instruction, &btb[entry].group[selected], taken);
	// 			}

	// 			delay = 14;

	// 			// ORCS_PRINTF("GRUPO:\t%" PRIu64 "\n", entry);
	// 			// ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[entry].group[0].pc, btb[entry].group[0].time);
	// 			// ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[entry].group[1].pc, btb[entry].group[1].time);
	// 			// ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[entry].group[2].pc, btb[entry].group[2].time);
	// 			// ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[entry].group[3].pc, btb[entry].group[3].time);
	// 			// ORCS_PRINTF("\n\n\n");
	// 		}
	// }
};

// =====================================================================
void processor_t::statistics() {
	ORCS_PRINTF("######################################################\n");
	ORCS_PRINTF("processor_t\n");

};


void cache::initialize(uint32_t offset, uint32_t index, uint32_t vias, uint32_t tamanho, unsigned char delay){
	


}
