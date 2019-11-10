#include "simulator.hpp"

// =====================================================================
processor_t::processor_t() {

	//definindo delay inicial.
	delay = 0;

	// Allocando caches.
	L1 = (cache *) malloc(sizeof(cache));
	L2 = (cache *) malloc(sizeof(cache));
	
	//Inicializando caches.
	L1->initialize("L1", 63, 255, 4, 1024, 1);
	L2->initialize("L2", 63, 1023, 8, 16384, 4);

	// Inicializando contadores.
	miss_L1 = 0;
	miss_L2 = 0;
	total_acesso_L1 = 0;
	total_acesso_L2 = 0;
	total_writeback = 0;

	prefetcher.initialize(16, 4, 1);

	prefetcher.allocate(20, 40);
	prefetcher.imprime(20);

	prefetcher.train(20, 30);
	prefetcher.imprime(20);

	prefetcher.train(20, 50);
	prefetcher.imprime(20);

	prefetcher.train(20, 60);
	prefetcher.imprime(20);

	prefetcher.train(20, 70);
	prefetcher.imprime(20);

	prefetcher.allocate(30, 20);
	prefetcher.imprime(30);

	prefetcher.train(30, 100);
	prefetcher.imprime(30);

	prefetcher.train(20, 80);
	prefetcher.imprime(20);

	prefetcher.train(30, 200);
	prefetcher.imprime(30);

	prefetcher.train(20, 10);
	prefetcher.imprime(20);

	prefetcher.train(30, 300);
	prefetcher.imprime(30);
	
	for(int i = 0; i <= 100; i+=10)
		if(prefetcher.search(i) != prefetcher.quantidade_entradas)
			ORCS_PRINTF("PARA %d\t resultado = %d\n", i, prefetcher.search(i));


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
	
	if (!orcs_engine.trace_reader->trace_fetch(&new_instruction) || orcs_engine.global_cycle > 2000000) {
	// if (!orcs_engine.trace_reader->trace_fetch(&new_instruction)) {
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
			delay += read(new_instruction.opcode_address, new_instruction.read_address);
		}
		if(new_instruction.is_read2){
			// ORCS_PRINTF("READ2\n");		
			delay += read(new_instruction.opcode_address, new_instruction.read2_address);
		}
		if(new_instruction.is_write){
			// ORCS_PRINTF("WRITE\n");			
			delay += read(new_instruction.opcode_address, new_instruction.write_address);
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

int processor_t::read(uint32_t op_endereco, uint32_t mem_endereco){
	uint32_t posicao_1, posicao_2;
	int delay;

	// ORCS_PRINTF("Buscando:\t%" PRIu32 "\n\n", mem_endereco);
	// L1->imprimeGrupo(mem_endereco);
	// L2->imprimeGrupo(mem_endereco);

	total_acesso_L1 += 1;
	delay = L1->latencia;

	// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
	if(!L1->search(mem_endereco, &posicao_1)){
		
		miss_L1 +=1;
		total_acesso_L2 += 1;
		delay += L2->latencia;

		this->prefetcher.train(op_endereco, mem_endereco);
		
		// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
		if(!L2->search(mem_endereco, &posicao_2)){

			// Aloca entrada com "op_endereco" e com "mem_endereco"
			this->prefetcher.allocate(op_endereco, mem_endereco);

			delay += DELAY_PRINC_MEM;
			
			miss_L2 += 1;
			// Tras bloco para cache L2.
			delay += L2->allocate(mem_endereco, posicao_2, &total_writeback);
		}

		// Tras bloco para cache L1.
		delay += L1->allocate(mem_endereco, posicao_1, &total_writeback);
	}
	// ORCS_PRINTF("DELAY:\t%d\n", delay);
	// ORCS_PRINTF("--------------------------------------\n\n");	 
	
	return delay;
}

void processor_t::write(uint32_t endereco){
	uint32_t index, grupo;
	uint32_t i;

	// Prepavra variáveis para busca em L1.
	index = (endereco >> BLOCK_BIT_SIZE) & L1->index_operator;
	grupo = index * L1->quantidade_vias;

	// Procura bloco referente ao endereço em L1.
	for( i = 0; i < L1->quantidade_vias; i++){
		// marca bloco como sujo.
		if((L1->blocos[grupo + i].endereco == endereco)){

			L1->blocos[grupo + i].dirty = 1;
			break;
		}
	}

	// Prepara variáveis para busca em L2
	index = (endereco >> BLOCK_BIT_SIZE) & L2->index_operator;
	grupo = index * L2->quantidade_vias;

	// Procura bloco referente ao endereço em L2
	for( i = 0; i < L2->quantidade_vias; i++){
		// Marca bloco como invalido.
		if((L2->blocos[grupo + i].endereco == endereco)){

			L2->blocos[grupo + i].validade = 0;
			break;
		}
	}
}


/***********************************************/
/************* METODOS DA CACHE ****************/
/***********************************************/
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

	// Deifine primeiro elemento do conjunto associativo como "selected".
	selected = grupo;


	for( i = 0; i < this->quantidade_vias; i++){
		// Caso bloco procurado seja encontrado e esteja válido, atualize o seu tempo de acesso e retorne 1.
		if((this->blocos[grupo + i].endereco == endereco) && this->blocos[grupo + i].validade ){
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

	int delay;

	delay = 0;
	
	// Caso bloco substituido esteja "sujo", 
	// aplicar delay de escrita em memória principal.
	if(this->blocos[posicao].dirty){

		delay = DELAY_PRINC_MEM;
		*total_writeback = *total_writeback + 1;
	}

	// Atualizando o novo bloco da cache.
	this->blocos[posicao].endereco = endereco;
	this->blocos[posicao].time = orcs_engine.global_cycle;
	this->blocos[posicao].validade = 1;
	this->blocos[posicao].dirty = 0;

	//Definindo quando bloco estará pronto (somente para L2)
	this->blocos[posicao].ready_cycle = orcs_engine.global_cycle + DELAY_PRINC_MEM;

	return delay;
}

void cache::free_cache(){
	free(this->blocos);
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
		ORCS_PRINTF("dirty: %d \n", this->blocos[grupo + i].dirty);
	}
	ORCS_PRINTF("\n");
}



/***********************************************/
/************* METODOS PREFETCHER **************/
/***********************************************/
void stride_prefetcher::initialize(unsigned char  quantidade_entradas, unsigned char  distance,unsigned char  degree){
	this->quantidade_entradas = quantidade_entradas;
	this->distance = distance;
	this->degree = degree;

	this->entradas = (entrada_stride *) malloc( quantidade_entradas * sizeof(entrada_stride));
	memset(this->entradas, 0, quantidade_entradas * sizeof(entrada_stride));	
}

void stride_prefetcher::allocate(uint32_t op_endereco, uint32_t mem_endereco){
	int entrada, selecionado;	

	// Define Primeira entrada do prefetcher como candidata a substituição.
	selecionado = 0;

	// Verifica se endereço procurado se encontra em alguma das entradas.
	for(entrada = 0; entrada < this->quantidade_entradas; entrada++){
		// Caso endereço seja achado em uma entrada válida. Parar de procura.
		if(this->entradas[entrada].tag == op_endereco && this->entradas[entrada].status != INVALIDO){
			break;
		}
		// Verifica se entrada "selecionado" não é "INVALIDO".
		if(this->entradas[selecionado].status != INVALIDO){
			// Verifica se entrada "selecionado" não é "TREINAMENTO" e entrada ENTRADA  é INVALIDO. Se verdadeiro, atualizar "selecionado".
			if(this->entradas[selecionado].status == TREINAMENTO  && this->entradas[entrada].status == INVALIDO){
				selecionado = entrada;
			}
			else{
				// Verifica se entrada "selecionado" não é "ATIVO" e entrada ENTRADA  não é ATIVO. Se verdadeiro, atualizar "selecionado".
				if(this->entradas[selecionado].status == ATIVO && this->entradas[entrada].status != ATIVO){
					selecionado = entrada;
				}
			}
		}
	}
	
	// Caso "op_endereco" não tenha sido encontrado em nenhuma das entradas. substituir entrada "selecionado" por nova entrada.
	if(entrada == this->quantidade_entradas){
		this->entradas[selecionado].tag = op_endereco;
		this->entradas[selecionado].last_address = mem_endereco;
		this->entradas[selecionado].stride = 0;
		this->entradas[selecionado].status = TREINAMENTO;
	}
}

void stride_prefetcher::train(uint32_t op_endereco, uint32_t mem_endereco){

	int verdadeiro_stride, entrada;

	// Verifica se endereço procurado se encontra em alguma das entradas.
	for(entrada = 0; entrada < this->quantidade_entradas; entrada++){
		// Caso endereço seja achado. Parar procura.
		if(this->entradas[entrada].tag == op_endereco){
			break;
		}
	}

	// Caso entrada para "op_endereco" tenha sido encontrada
	if(entrada != this->quantidade_entradas){
		// Verifica se entrada é válida.
		if(this->entradas[entrada].status != INVALIDO){
			
			verdadeiro_stride = mem_endereco - this->entradas[entrada].last_address; 
			
			// Verifica se "stride" armazenado é igual ao "verdadeiro_stride".
			if(this->entradas[entrada].stride == verdadeiro_stride){
				this->entradas[entrada].status = ATIVO;
				this->entradas[entrada].last_address = mem_endereco;
			}
			// Caso "stride" não seja condizente.
			else{
				// Verifica se "status" é de treinamento. caso seja atualiza o "last_address" e "stride".
				if(this->entradas[entrada].status  == TREINAMENTO){
					this->entradas[entrada].last_address = mem_endereco;
					this->entradas[entrada].stride = verdadeiro_stride;
				}
				// Caso "status" seja "ATIVO", torná-lo INVALIDO.
				else{
					this->entradas[entrada].status = INVALIDO;
				}
			}
		}
	}	
}

int stride_prefetcher::search(uint32_t op_endereco){
	uint32_t entrada;

	// Verifica se endereço procurado se encontra em alguma das entradas.
	for(entrada = 0; entrada < this->quantidade_entradas; entrada++){
		// Caso endereço seja achado em uma entrada válida. Parar de procura.
		if(this->entradas[entrada].tag == op_endereco && this->entradas[entrada].status == ATIVO){
			return entrada;
		}
	}
	return entrada;
}
/*-------------------------------------------------*/
/*--------------------- DEBUG ---------------------*/
/*-------------------------------------------------*/
void stride_prefetcher::imprime(uint32_t endereco){

	uint32_t entrada;
	
	ORCS_PRINTF("endereco:\t%" PRIu32 "\n", endereco);

	for(entrada = 0; entrada < this->quantidade_entradas; entrada++){
		ORCS_PRINTF("tag: %" PRIu32 "          \t", this->entradas[entrada].tag);	
		ORCS_PRINTF("last_address: %" PRIu32 " \t", this->entradas[entrada].last_address);
		ORCS_PRINTF("stride: %" PRId32 "       \t", this->entradas[entrada].stride);
		ORCS_PRINTF("status: %d                \n", this->entradas[entrada].status);
	}
	ORCS_PRINTF("------------------------------\n\n");
}