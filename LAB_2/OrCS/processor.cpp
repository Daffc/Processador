#include "simulator.hpp"

// =====================================================================
processor_t::processor_t() {

	//definindo delay inicial.
	delay = 0;

	// Allocando caches.
	L1 = (cache *) malloc(sizeof(cache));
	L2 = (cache *) malloc(sizeof(cache));
	
	//Inicializando caches.
	L1->initialize("L1", 63, 6, 255, 8, 4, 1024, 1);
	L2->initialize("L2", 63, 6, 1023, 10, 8, 16384, 4);

	// Inicializando contadores.
	miss_L1 = 0;
	miss_L2 = 0;
	total_acesso_L1 = 0;
	total_acesso_L2 = 0;
	total_writeback = 0;
};

// =====================================================================
void processor_t::allocate() {
};

// =====================================================================
void processor_t::clock() {

	/// Get the next instruction from the trace
	opcode_package_t new_instruction;

	if(delay){
		delay--;
		return;	
	}
	
	// if (!orcs_engine.trace_reader->trace_fetch(&new_instruction) || orcs_engine.global_cycle > 200000) {
	if (!orcs_engine.trace_reader->trace_fetch(&new_instruction)) {
		/// If EOF
		ORCS_PRINTF("CLOCKS = %" PRIu64 "\n",orcs_engine.global_cycle);
		orcs_engine.simulator_alive = false;

		L1->free_cache();
		L2->free_cache();

		free(L1);
		free(L2);
	}
	else{
		if(new_instruction.is_read){
			// ORCS_PRINTF("READ\n");		
			delay += read(new_instruction.read_address);
		}
		if(new_instruction.is_read2){
			// ORCS_PRINTF("READ2\n");		
			delay += read(new_instruction.read2_address);
		}
		if(new_instruction.is_write){
			// ORCS_PRINTF("WRITE\n");			
			delay += read(new_instruction.write_address);
			write(new_instruction.write_address);
		}
	}
};

// =====================================================================
void processor_t::statistics() {

	ORCS_PRINTF("total_acesso_L1:\t%" PRIu32 "\n", total_acesso_L1);
	ORCS_PRINTF("miss_L1:        \t%" PRIu32 "\n", miss_L1);
	ORCS_PRINTF("total_acesso_L2:\t%" PRIu32 "\n", total_acesso_L2);
	ORCS_PRINTF("miss_L2:        \t%" PRIu32 "\n", miss_L2);
	ORCS_PRINTF("total_writeback:\t%" PRIu32 "\n", total_writeback);
	ORCS_PRINTF("######################################################\n");
	ORCS_PRINTF("processor_t\n");

};


void cache::initialize(const char* nome, uint32_t offset_operator, uint32_t offset_bits, uint32_t index_operator, uint32_t index_bits, uint32_t vias, uint32_t tamanho, unsigned char delay){
	memcpy(this->nome, nome, 4);
	this->offset_operator = offset_operator;
	this->offset_bits = offset_bits;
	this->index_operator = index_operator;
	this->index_bits = index_bits;
	this->quantidade_vias = vias;
	this->tamanho = tamanho;
	this->latencia = delay;

	this->blocos = (block *) malloc(tamanho * sizeof(block));
	memset(this->blocos, 0, tamanho * sizeof(block));
}

int cache::search(uint32_t endereco, uint32_t* melhor_posicao){
	uint32_t index, grupo, older, selected, tag;
	uint32_t i;

	index = (endereco >> this->offset_bits) & this->index_operator;
	grupo = index * this->quantidade_vias;
	tag = endereco >> (this->offset_bits + this->index_bits);
	older = UINT32_MAX;

	// Deifine primeiro elemento do conjunto associativo como "selected".
	selected = grupo;

	for( i = 0; i < this->quantidade_vias; i++){
		// Caso bloco procurado seja encontrado e esteja válido, atualize o seu tempo de acesso e retorne 1.
		if((this->blocos[grupo + i].tag == tag) && this->blocos[grupo + i].validade ){
			this->blocos[grupo + i].time = orcs_engine.global_cycle;
			return 1;
		}
		// Verifica se o bloco "selected" para substituição é um bloco válido. 
		// Caso não seja valido, o bloco "selected" deverá ser o substituido.
		if(this->blocos[selected].validade){
			// Caso bloco "selected" seja válido, verifica se próximo é mais 
			// antigo que o já selecionado ou se é um bloco inválido.
			if((older > this->blocos[grupo + i].time) || !(this->blocos[grupo + i].validade)){
				selected = grupo + i;
				older = this->blocos[grupo + i].time;
			}
		}
	}

	*melhor_posicao = selected;

	return 0;
}

int cache::allocate(uint32_t endereco, uint32_t posicao, uint32_t *total_writeback){

	int delay, tag;

	tag = endereco >> (this->offset_bits + this->index_bits);

	delay = 0;
	
	// Caso bloco substituido esteja "sujo", 
	// aplicar delay de escrita em memória principal.
	

	if(this->blocos[posicao].dirty){
		delay = DELAY_PRINC_MEM;
		*total_writeback = *total_writeback + 1;
	}

	// Atualizando o novo bloco da cache.
	this->blocos[posicao].tag = tag;
	this->blocos[posicao].time = orcs_engine.global_cycle;
	this->blocos[posicao].validade = 1;
	this->blocos[posicao].dirty = 0;


	return delay;
}

void cache::free_cache(){
	free(this->blocos);
}

int processor_t::read(uint32_t endereco){
	uint32_t posicao_1, posicao_2;
	int delay;

	// L1->imprimeGrupo(endereco);
	// L2->imprimeGrupo(endereco);

	total_acesso_L1 += 1;
	delay = L1->latencia;

	// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
	if(!L1->search(endereco, &posicao_1)){
		
		miss_L1 +=1;
		total_acesso_L2 += 1;
		delay += L2->latencia;
		// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
		if(!L2->search(endereco, &posicao_2)){			// delay += DELAY_PRINC_MEM;

			miss_L2 += 1;
			// Tras bloco para cache L2.
			delay += L2->allocate(endereco, posicao_2, &total_writeback);
		}

		// Tras bloco para cache L1.
		delay += L1->allocate(endereco, posicao_1, &total_writeback);
	}
	// ORCS_PRINTF("DELAY:\t%d\n", delay);
	// ORCS_PRINTF("--------------------------------------\n\n");	 
	
	return delay;
}

void processor_t::write(uint32_t endereco){
	uint32_t index, grupo, tag;
	uint32_t i;

	// Prepavra variáveis para busca em L1.
	index = (endereco >> L1->offset_bits) & L1->index_operator;
	grupo = index * L1->quantidade_vias;
	tag = endereco >> (L1->offset_bits + L1->index_bits);
	
	// Procura bloco referente ao endereço em L1.
	for( i = 0; i < L1->quantidade_vias; i++){
		// marca bloco como sujo.
		if((L1->blocos[grupo + i].tag == tag)){

			L1->blocos[grupo + i].dirty = 1;
			break;
		}
	}
	
	// Prepara variáveis para busca em L2
	index = (endereco >> L2->offset_bits) & L2->index_operator;
	grupo = index * L2->quantidade_vias;
	tag = endereco >> (L2->offset_bits + L2->index_bits);

	// Procura bloco referente ao endereço em L2
	for( i = 0; i < L2->quantidade_vias; i++){
		// Marca bloco como invalido.
		if((L2->blocos[grupo + i].tag == tag)){

			L2->blocos[grupo + i].validade = 0;
			break;
		}
	}
}

/*-------------------------------------------------*/
/*--------------------- DEBUG ---------------------*/
/*-------------------------------------------------*/
void cache::imprimeGrupo(uint32_t endereco){

	uint32_t index, grupo;
	uint32_t i;

	index = (endereco >> this->offset_bits) & this->index_operator;
	grupo = index * this->quantidade_vias;
	
	ORCS_PRINTF("%s->search:\n", this->nome);
	ORCS_PRINTF("endereco:\t%" PRIu32 "\n", endereco);
	ORCS_PRINTF("index:   \t%" PRIu32 "\n", index);

	for( i = 0; i < this->quantidade_vias; i++){
		ORCS_PRINTF("tag: %" PRIu32 " \t", this->blocos[grupo + i].tag);	
		ORCS_PRINTF("time: %" PRIu64 " \t", this->blocos[grupo + i].time);
		ORCS_PRINTF("validade: %d \t", this->blocos[grupo + i].validade);
		ORCS_PRINTF("dirty: %d \n", this->blocos[grupo + i].dirty);
	}
	ORCS_PRINTF("\n");
}



