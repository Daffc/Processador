#include "simulator.hpp"

// =====================================================================

markov_prefetcher prefetcher;

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

	prefetcher.initialize(8, 4, 32);
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
	
	// if (!orcs_engine.trace_reader->trace_fetch(&new_instruction) || orcs_engine.global_cycle > 2000) {
	if (!orcs_engine.trace_reader->trace_fetch(&new_instruction)) {
		/// If EOF
		ORCS_PRINTF("CLOCKS = %" PRIu64 "\n",orcs_engine.global_cycle);
		orcs_engine.simulator_alive = false;
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

int processor_t::read(uint32_t endereco){
	uint32_t posicao_1, posicao_2;
	unsigned int delay_interno, delay_buffer;

	// L1->imprimeGrupo(endereco);
	// L2->imprimeGrupo(endereco);

	total_acesso_L1 += 1;
	delay_interno = L1->latencia;

	// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
	if(!L1->search(endereco, &posicao_1)){
		
		miss_L1 +=1;
		total_acesso_L2 += 1;
		delay_interno += L2->latencia;
		// Verifica se bloco está em cache e o atualiza, caso não esteja entra no if.
		if(!L2->search(endereco, &posicao_2)){			
			
			// Verifica se endereço consta em buffer.
			if(prefetcher.buscaNoBuffer(endereco, L2->offset_bits + L2->index_bits, &delay_buffer)){
				delay_interno += delay_buffer;
			}
			else{
				delay_interno += DELAY_PRINC_MEM;
				miss_L2 += 1;
			}
			
			prefetcher.train(endereco, L2->offset_bits + L2->index_bits);
			prefetcher.allocate(endereco, L2->offset_bits + L2->index_bits);
			prefetcher.prefetch(endereco, L2->offset_bits + L2->index_bits, DELAY_PRINC_MEM);
			prefetcher.atualizaAnterior(endereco, L2->offset_bits + L2->index_bits);
			// Tras bloco para cache L2.
			delay_interno += L2->allocate(endereco, posicao_2, &total_writeback);
		}

		// Tras bloco para cache L1.
		delay_interno += L1->allocate(endereco, posicao_1, &total_writeback);
	}
	// ORCS_PRINTF("DELAY:\t%d\n", delay_interno);
	// ORCS_PRINTF("--------------------------------------\n\n");	 
	
	return delay_interno;
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
	prefetcher.invalidaBlocoBuffer(endereco, L2->offset_bits + L2->index_bits);
}

/***********************************************/
/************* METODOS DA CACHE ****************/
/***********************************************/

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


/***********************************************/
/************* METODOS PREFETCHER **************/
/***********************************************/
void markov_prefetcher::initialize(unsigned char  quantidade_entradas, unsigned char  tamanho_grupo, unsigned int tamanho_buffer){

	int entrada;

	this->quantidade_entradas = quantidade_entradas;
	this->tamanho_grupo = tamanho_grupo;

	// Definindo tamanho da tabela de predição para markov.
	this->entradas = (entrada_markov *) malloc( quantidade_entradas * sizeof(entrada_markov));
	memset(this->entradas, 0, quantidade_entradas * sizeof(entrada_markov));

	// Allocando e zerando cara elemento de cada entrada da tabela de markov.
	for(entrada = 0; entrada < this->quantidade_entradas; entrada++){
		this->entradas[entrada].grupo = (proximo *) malloc( tamanho_grupo * sizeof(proximo));
		memset(this->entradas[entrada].grupo, 0, tamanho_grupo * sizeof(proximo));
	}
	this->endereco_anterior = 0;

	// Adiquirindo metricas do buffer que armazenará as prediçoes.
	this->tamanho_buffer = tamanho_buffer;
	this->buffer_prefetch = (entrada_buffer *) malloc( tamanho_buffer * sizeof(entrada_buffer));
	memset(this->buffer_prefetch, 0, tamanho_buffer * sizeof(entrada_buffer));
	this->cabeca_buffer = 0;
}

void markov_prefetcher::allocate(uint32_t mem_endereco, unsigned int shift_bits){
	unsigned entrada, selecionado, proximo, tag;	

	// Registra tag do endereço de memória (correspondente ao bloco).
	tag = mem_endereco >> shift_bits;

	// Define Primeira entrada do prefetcher como candidata a substituição.
	selecionado = 0;

	// Verifica se endereço procurado se encontra em alguma das entradas.
	for(entrada = 0; entrada < this->quantidade_entradas; entrada++){
		// Caso endereço seja achado em uma entrada válida. Parar de procura.
		if(this->entradas[entrada].tag == tag){
			break;
		}
		// Verifica se entrada "selecionado" não é "INVALIDO".
		if(this->entradas[selecionado].time > this->entradas[entrada].time ){
			selecionado = entrada;
		}
	}
	
	// Caso "op_endereco" não tenha sido encontrado em nenhuma das entradas. substituir entrada "selecionado" por nova entrada.
	if(entrada == this->quantidade_entradas){
		this->entradas[selecionado].tag = tag;
		for(proximo = 0; proximo < this->tamanho_grupo; proximo++){
			this->entradas[selecionado].grupo[proximo].endereco = 0;
			this->entradas[selecionado].grupo[proximo].counter = 0;
		}			
	}
	this->entradas[selecionado].time = orcs_engine.global_cycle;
}

void markov_prefetcher::train(uint32_t proximo_endereco, unsigned int shift_bits){

	unsigned  int entrada, proximo, selecionado = 0, tag;	

	// Registra tag do endereço de memória (correspondente ao bloco).
	tag = proximo_endereco >> shift_bits;

	// ORCS_PRINTF("treinamento: %" PRIu32 "\n", tag);	

	for(entrada = 0; entrada < this->quantidade_entradas; entrada++){
		// Caso endereço seja achado em uma entrada válida. Parar de procura.uint32_t 	endereco;
		if(this->entradas[entrada].tag == this->endereco_anterior){
			break;
		}
	}
	// this->imprimeTabela();
	
	// ORCS_PRINTF("entrada: %d\t", entrada);	

	for(proximo = 0; proximo < this->tamanho_grupo; proximo++){
		// Caso proximo_endereco conste em algum elemento do grupo.
		if(this->entradas[entrada].grupo[proximo].endereco == tag){	
			break;
		}
		// Seleciona a entrada do grupo que possui menor probabilidade de sofrer prefetch.
		if(this->entradas[entrada].grupo[selecionado].counter > this->entradas[entrada].grupo[proximo].counter){
			selecionado = proximo;
		}
	}

	// ORCS_PRINTF("selecionado: %d\n\n", selecionado);	

	// Caso não conste, substituir elemento do grupo "menos provavel"
	if(proximo == this->tamanho_grupo){
		this->entradas[entrada].grupo[selecionado].endereco = tag;
		this->entradas[entrada].grupo[selecionado].counter = 0;
	}
	else{
		// Ajusta selecionado para caso entrada de "ultimo_endereco" tenha sido encontrada.
		selecionado = proximo;
	}

	for(proximo = 0; proximo < this->tamanho_grupo; proximo++){
		// Se entrada possui o proximo, incrementar em 1.
		if(proximo == selecionado){
			this->entradas[entrada].grupo[proximo].counter += 1;
		}
		// Caso contreário decrementar.
		else{
			this->entradas[entrada].grupo[proximo].counter -= 1;
		}

		if(this->entradas[entrada].grupo[proximo].counter > 3){
			this->entradas[entrada].grupo[proximo].counter = 3;
		}
		if(this->entradas[entrada].grupo[proximo].counter < 0){
			this->entradas[entrada].grupo[proximo].counter = 0;
		}
	}	
}

void markov_prefetcher::prefetch(uint32_t mem_endereco, unsigned int shift_bits, unsigned int delay){
	unsigned  int entrada, proximo, selecionado = 0, tag;	

	// Registra tag do endereço de memória (correspondente ao bloco).
	tag = mem_endereco >> shift_bits;

	// Registra tag do endereço de memória (correspondente ao bloco).
	for(entrada = 0; entrada < this->quantidade_entradas; entrada++){
		// Caso endereço seja achado em uma entrada válida. Parar de procura.
		if(this->entradas[entrada].tag == tag){
			break;
		}
	}

	// Caso endereço que sofreu miss possua entrada em tabela de markov.
	if(entrada != this->quantidade_entradas){

		// Percorre todas as entradas.
		for(proximo = 0; proximo < this->tamanho_grupo; proximo++){
			// Seleciona a entrada dos elementos do grupo que possui maior probabilidade de seguir o endereco que sofreu miss.
			if(this->entradas[entrada].grupo[selecionado].counter < this->entradas[entrada].grupo[proximo].counter){
				selecionado = proximo;
			}
		}

		//Aloca em buffer endereco que derevá ser buscado.
		this->insereNoBuffer(this->entradas[entrada].grupo[selecionado].endereco, delay);
	}
}

void markov_prefetcher::insereNoBuffer(uint32_t tag, unsigned int delay){

	uint32_t entrada;
	// Verifica se já existe entrada que armazena bloco "tag" em buffer
	for(entrada = 0; entrada < this->tamanho_buffer; entrada++){
		if(this->buffer_prefetch[entrada].endereco == tag && this->buffer_prefetch[entrada].validade){
			break;
		}
	}

	// Caso nao exista nenhuma entrada armazenando o endereço anterior, solicitar e armazenar.
	if(entrada == this->tamanho_buffer){
		// Alocar novo bloco.
		this->buffer_prefetch[this->cabeca_buffer].endereco = tag;

		// Define quando bloco estará pronto.
		this->buffer_prefetch[this->cabeca_buffer].ready_cycle = orcs_engine.global_cycle + delay;

		// Define bloco como válido.
		this->buffer_prefetch[this->cabeca_buffer].validade = 1;

		// Atualiza cabeça do buffer.
		this->cabeca_buffer +=1;

		// Caso buffer tenha ultrapassado "tamanho_buffer", efetuar overflow.
		if(this->cabeca_buffer == this->tamanho_buffer)
			this->cabeca_buffer = 0;
	}
}

int markov_prefetcher::buscaNoBuffer(uint32_t mem_endereco,unsigned int shift_bits, unsigned int *delay){

	
	unsigned  int entrada, tag;	
	long int 	diferenca;

	// Registra tag do endereço de memória (correspondente ao bloco).
	tag = mem_endereco >> shift_bits;

	*delay = 0;
	
	// ORCS_PRINTF("mem_endereco: %" PRIu32 " \n",mem_endereco);
	// ORCS_PRINTF("tag: %" PRIu32 " \n", tag);

	// this->imprimeBuffer();

	// Verifica se já existe entrada que armazena bloco "tag" em buffer
	for(entrada = 0; entrada < this->tamanho_buffer; entrada++){
		if(this->buffer_prefetch[entrada].endereco == tag && this->buffer_prefetch[entrada].validade ){
			break;
		}
	}

	// Caso exista entrada armazenando o endereço anterior.
	if(entrada != this->tamanho_buffer){

		// Verifica qual a diferença entre o ciclo atual e o ciclo em que o bloco estará em buffer.
		diferenca = this->buffer_prefetch[entrada].ready_cycle - orcs_engine.global_cycle;

		// Caso diferença seja positiva, retornar como delay (mais rápido que pedir a cache inferior).
		if(diferenca > 0)
			*delay = diferenca;

		// ORCS_PRINTF("ENCONTRADO: %" PRIu32 " \n",mem_endereco);
		// ORCS_PRINTF("CICLO: %lu \n",orcs_engine.global_cycle);
		// ORCS_PRINTF("DELAY: %" PRIu32 " \n",*delay);
		// ORCS_PRINTF("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n\n");
		// Retorna que bloco foi encontrado.
		return 1;
	}
	else{
		// // Retorna que bloco não foi encontrado.
		// ORCS_PRINTF("MISS: %" PRIu32 " \n",mem_endereco);
		// ORCS_PRINTF("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n\n");
		return 0;
	}
}

void markov_prefetcher::invalidaBlocoBuffer(uint32_t mem_endereco, unsigned int shift_bits){

	
	unsigned  int entrada, tag;	

	// Registra tag do endereço de memória (correspondente ao bloco).
	tag = mem_endereco >> shift_bits;

	// Verifica se já existe entrada que armazena bloco "tag" em buffer
	for(entrada = 0; entrada < this->tamanho_buffer; entrada++){
		if(this->buffer_prefetch[entrada].endereco == tag){
			break;
		}
	}

	// Caso exista entrada armazenando o endereço anterior.
	if(entrada != this->tamanho_buffer){
		
		// Invalida bloco uma vez que ele se encontra la L2.
		this->buffer_prefetch[entrada].validade = 0;
	}
}

void markov_prefetcher::atualizaAnterior(uint32_t mem_endereco, unsigned int shift_bits){

	unsigned  int  tag;	

	// Registra tag do endereço de memória (correspondente ao bloco).
	tag = mem_endereco >> shift_bits;
	this->endereco_anterior = tag;
}




/*-------------------------------------------------*/
/*--------------------- DEBUG ---------------------*/
/*-------------------------------------------------*/
void markov_prefetcher::imprimeTabela(){

	uint32_t entrada, grupo;

	for(entrada = 0; entrada < this->quantidade_entradas; entrada++){
		ORCS_PRINTF("tag: %" PRIu32 "\t", this->entradas[entrada].tag);	
		
		for(grupo = 0; grupo < this->tamanho_grupo; grupo++){
			ORCS_PRINTF("next: %" PRIu32 " \t", 	this->entradas[entrada].grupo[grupo].endereco);
			ORCS_PRINTF("counter: %d \t", 	this->entradas[entrada].grupo[grupo].counter);
		}
		ORCS_PRINTF("time: %" PRIu64 "\n", this->entradas[entrada].time);
	}
}

void markov_prefetcher::imprimeBuffer(){

	uint32_t entrada;

	for(entrada = 0; entrada < this->tamanho_buffer; entrada++){	
		ORCS_PRINTF("endereco: %" PRIu32 " \t",this->buffer_prefetch[entrada].endereco);
		ORCS_PRINTF("validade: %d \t",this->buffer_prefetch[entrada].validade);
		ORCS_PRINTF("time: %" PRIu64 "\n", 	this->buffer_prefetch[entrada].ready_cycle);
	}
}