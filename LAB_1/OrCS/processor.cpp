#include "simulator.hpp"

BTB_t *btb;
char  delay;

// =====================================================================
processor_t::processor_t() {

};

// =====================================================================
void processor_t::allocate() {
	btb = (BTB_t *) malloc(4096 * sizeof(BTB_t));
	memset(btb, 0, 4096 * sizeof(BTB_t));
};

// =====================================================================
void processor_t::clock() {

	/// Get the next instruction from the trace
	opcode_package_t new_instruction;
	uint64_t  group, older;
	char selected;
	
	if(delay){
		delay--;
		return;	
	}
	if (!orcs_engine.trace_reader->trace_fetch(&new_instruction)) {
		/// If EOF
		ORCS_PRINTF("CLOCKS = %" PRIu64 "\n", orcs_engine.global_cycle);
		orcs_engine.simulator_alive = false;
	}
	else{
		if(new_instruction.opcode_operation == INSTRUCTION_OPERATION_BRANCH ){
			group = (new_instruction.opcode_address & 1023) * 4;
			
			if(btb[group].pc == new_instruction.opcode_address){
				btb[group].pc = new_instruction.opcode_address;
				btb[group].time = orcs_engine.global_cycle;
				
			}
			else{
				older = btb[group].time;
				selected = 0;
				if(btb[group + 1].pc == new_instruction.opcode_address){
					btb[group + 1].pc = new_instruction.opcode_address;
					btb[group + 1].time = orcs_engine.global_cycle;
					
				}
				else{
					if(older > btb[group + 1].time){
						older = btb[group + 1].time;
						selected = 1;
					}
					if(btb[group + 2].pc == new_instruction.opcode_address){
						btb[group + 2].pc = new_instruction.opcode_address;
						btb[group + 2].time = orcs_engine.global_cycle;
						
					}	
					else{
						if(older > btb[group + 2].time){
							older = btb[group + 2].time;
							selected = 2;
						}
						if(btb[group + 3].pc == new_instruction.opcode_address){
							btb[group + 3].pc = new_instruction.opcode_address;
							btb[group + 3].time = orcs_engine.global_cycle;
							
						}	
						else{
							delay = 14;
							if(older > btb[group + 3].time){
								older = btb[group + 3].time;
								selected = 3;
							}

							btb[group + selected].pc = new_instruction.opcode_address;
							btb[group + selected].time = orcs_engine.global_cycle;

							ORCS_PRINTF("GRUPO:\t%" PRIu64 "\n", group);
							ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[group].pc, btb[group].time);
							ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[group + 1].pc, btb[group + 1].time);
							ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[group + 2].pc, btb[group + 2].time);
							ORCS_PRINTF("%" PRIu64 " \t %" PRIu64 "\n", btb[group + 3].pc, btb[group + 3].time);
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
