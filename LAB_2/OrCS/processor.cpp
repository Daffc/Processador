#include "simulator.hpp"

// =====================================================================
processor_t::processor_t() {
	// Allocando caches.
	L1 = (cache *) malloc(sizeof(cache));
	L2 = (cache *) malloc(sizeof(cache));
	
	//Inicializando caches.
	L1->initialize("L1", 63, 255, 4, 1024, 1);
	L2->initialize("L2", 63, 1023, 8, 16384, 4);

	/*----------------------------------------------------------*/
	/*--------------------- TESTES LEITURA ---------------------*/
	/*----------------------------------------------------------*/
	
	uint32_t posicao_1, posicao_2, endereco;
	endereco = 2579;
	// delay = L1->latencia.

	// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
	L1->imprimeGrupo(endereco);
	L2->imprimeGrupo(endereco);

	if(!L1->search(endereco, &posicao_1)){
		// delay += L2->latencia.

		// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
		if(!L2->search(endereco, &posicao_2)){
			// delay += DELAY_PRINC_MEM.

			L2->allocate(endereco, posicao_2);
		}

		L1->allocate(endereco, posicao_1);
	}
	
	L1->imprimeGrupo(endereco);
	L2->imprimeGrupo(endereco);

	endereco = 2583;

	if(!L1->search(endereco, &posicao_1)){
		// delay += L2->latencia.

		// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
		if(!L2->search(endereco, &posicao_2)){
			// delay += DELAY_PRINC_MEM.

			L2->allocate(endereco, posicao_2);
		}

		L1->allocate(endereco, posicao_1);
	}
	
	L1->imprimeGrupo(endereco);
	L2->imprimeGrupo(endereco);

	endereco = 2591;
	if(!L1->search(endereco, &posicao_1)){
		// delay += L2->latencia.

		// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
		if(!L2->search(endereco, &posicao_2)){
			// delay += DELAY_PRINC_MEM.

			L2->allocate(endereco, posicao_2);
		}

		L1->allocate(endereco, posicao_1);
	}
	
	L1->imprimeGrupo(endereco);
	L2->imprimeGrupo(endereco);

	ORCS_PRINTF("----------------------------\n");

	endereco = 2607;
	if(!L1->search(endereco, &posicao_1)){
		// delay += L2->latencia.

		// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
		if(!L2->search(endereco, &posicao_2)){
			// delay += DELAY_PRINC_MEM.

			L2->allocate(endereco, posicao_2);
		}

		L1->allocate(endereco, posicao_1);
	}
	
	L1->imprimeGrupo(endereco);
	L2->imprimeGrupo(endereco);

	ORCS_PRINTF("----------------------------\n");
	
	endereco = 2603;
	if(!L1->search(endereco, &posicao_1)){
		// delay += L2->latencia.

		// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
		if(!L2->search(endereco, &posicao_2)){
			// delay += DELAY_PRINC_MEM.

			L2->allocate(endereco, posicao_2);
		}

		L1->allocate(endereco, posicao_1);
	}
	
	L1->imprimeGrupo(endereco);
	L2->imprimeGrupo(endereco);
	ORCS_PRINTF("----------------------------\n");

	

	/*----------------------------------------------------------*/
	/*------------------- FIM TESTES LEITURA -------------------*/
	/*----------------------------------------------------------*/

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
		ORCS_PRINTF("CLOCKS = %" PRIu64 "\n",orcs_engine.global_cycle);
		orcs_engine.simulator_alive = false;
	}
	else{

	}
};

// =====================================================================
void processor_t::statistics() {
	ORCS_PRINTF("######################################################\n");
	ORCS_PRINTF("processor_t\n");

};


void cache::initialize(const char* nome,uint32_t offset, uint32_t index, uint32_t vias, uint32_t tamanho, unsigned char delay){
	memcpy(this->nome, nome, 4);
	this->offset_operator = offset;
	this->index_operator = index;
	this->quantidade_vias = vias;
	this->tamanho = tamanho;
	this->latencia = delay;

	this->blocos = (block *) malloc(tamanho * sizeof(block));
	memset(this->blocos, 0, tamanho * sizeof(block));
}

int cache::search(uint32_t endereco, uint32_t* melhor_posicao){
	uint32_t index, grupo, older, selected;
	uint32_t i;

	index = (endereco >> BLOCK_BIT_SIZE) & this->index_operator;
	grupo = index * this->quantidade_vias;

	older = UINT32_MAX;
	selected = grupo;
	
	ORCS_PRINTF("%s->search:\n", this->nome);
	ORCS_PRINTF("endereco:\t%" PRIu32 "\n", endereco);

	for( i = 0; i < this->quantidade_vias; i++){
		// Caso bloco procurado seja encontrado e esteja válido, atualize o seu tempo de acesso e retorne 1.
		if((this->blocos[grupo + i].endereco == endereco) && this->blocos[grupo + i].validade ){
			ORCS_PRINTF("ENCONTRADO!\n");
			ORCS_PRINTF("posicao:\t%" PRIu32 "\n\n\n", grupo + i);
			this->blocos[grupo + i].time = orcs_engine.global_cycle;
			return 1;
		}
		// Verifica se o bloco "selected" para substituição é um bloco válido. 
		// Caso não seja valido, o bloco "selected" deverá ser o substituido.
		if(this->blocos[selected].validade){
			// Caso bloco "selected" seja válido, verifica se próximo é mais 
			// antigo que o já selecionado ou se é um bloco inválido.
			if(older > this->blocos[grupo + i].time || !this->blocos[grupo + i].validade){
				selected = grupo + i;
			}
		}
	}

	ORCS_PRINTF("NÂO ENCONTRADO!\n\n");
	ORCS_PRINTF("posicao:\t%" PRIu32 "\n\n\n", selected);

	*melhor_posicao = selected;

	return 0;
}

int cache::allocate(uint32_t endereco, uint32_t posicao){

	int delay;

	delay = 0;
	
	// Caso bloco substituido esteja "sujo", 
	// aplicar delay de escrita em memória principal.
	if(this->blocos[posicao].dirty){
		delay = DELAY_PRINC_MEM;
	}

	// Atualizando o novo bloco da cache.
	this->blocos[posicao].endereco = endereco;
	this->blocos[posicao].time = orcs_engine.global_cycle;
	this->blocos[posicao].validade = 1;
	this->blocos[posicao].dirty = 0;

	return delay;
}


/*-------------------------------------------------*/
/*--------------------- DEBUG ---------------------*/
/*-------------------------------------------------*/
void cache::imprimeGrupo(uint32_t endereco){

	uint32_t index, grupo;
	uint32_t i;

	index = (endereco >> BLOCK_BIT_SIZE) & this->index_operator;
	grupo = index * this->quantidade_vias;
	
	ORCS_PRINTF("%s->search:\n", this->nome);
	ORCS_PRINTF("endereco:\t%" PRIu32 "\n", endereco);
	ORCS_PRINTF("index:   \t%" PRIu32 "\n", index);

	for( i = 0; i < this->quantidade_vias; i++){
		ORCS_PRINTF("endereco: %" PRIu32 " \t", this->blocos[grupo + i].endereco);	
		ORCS_PRINTF("time: %" PRIu64 " \t", this->blocos[grupo + i].time);
		ORCS_PRINTF("validade: %d \t", this->blocos[grupo + i].validade);
		ORCS_PRINTF("dirty: %d \t", this->blocos[grupo + i].dirty);
		ORCS_PRINTF("\n\n");
	}

}