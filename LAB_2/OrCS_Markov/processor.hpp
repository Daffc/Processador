
#define BLOCK_BIT_SIZE 6
#define DELAY_PRINC_MEM 200

class block {
	private:

	public:
		uint32_t tag;
		uint64_t time;
		char	 validade;
		char	 dirty;
};

class cache{
	private:


	public:
		char     nome[5] ;
		block 	 *blocos;
		uint32_t offset_operator;
		uint32_t offset_bits;
		uint32_t index_operator;
		uint32_t index_bits;
		uint32_t quantidade_vias;
		uint32_t tamanho;
		unsigned char latencia;

		// Criar e instanciar cache.
		void initialize(const char* nome, uint32_t offset_operator, uint32_t Offset_bits, uint32_t index_operator, uint32_t index_bits, uint32_t vias, uint32_t tamanho, unsigned char delay);
		
		// Procurar por bloco de informado na cache, caso ache returna 1, 
		// caso não ache, retorna 0 e melhor posição em *melhor_posicao
		int search(uint32_t endereco, uint32_t *melhor_posicao);
		
		// Aloca bloco de "endereço" solicitado no espaço indicado por "posicao" em cache e retorna delay 
		// provocado pela operação (substituição de dirty block).
		int allocate(uint32_t endereco, uint32_t posicao,uint32_t *total_writeback);

		//Limpar memória usada por cache.
		void free_cache();

		/*-------------------------------------------------*/
		/*--------------------- DEBUG ---------------------*/
		/*-------------------------------------------------*/

		// Imprime grupo de cache de acordo com o index provido pelo endereco.
		void imprimeGrupo(uint32_t endereco);

};

/***********************************************/
/******** ELEMENTOS DO PREFETCHER **************/
/***********************************************/

class proximo{
	public:
		uint32_t 		endereco;
		char			counter;
};
class entrada_markov{
	public:
		uint32_t 	tag;
		proximo 	*grupo;
		uint64_t	time;
};

class markov_prefetcher{
	public:
		entrada_markov 	*entradas;
		unsigned char	quantidade_entradas;
		unsigned char 	tamanho_grupo;
		uint32_t		endereco_anterior;

		
		// Inicializa Dimenções e entradas do prefecher.
		void initialize(unsigned char  quantidade_entradas, unsigned char  tamanho_grupo);
		// // Armazena endereço da instrução e da requisição em entrada de prefetcher.
		void allocate(uint32_t mem_endereco);
		// // Recebe endereço de instrução "op_endereco" e endereço de memória "mem_endereco" 
		// // e atualiza (caso exista entrada para instrução "op_endereco") "status" do prefetcher 
		// // de acordo com a politica indicada .
		void train(uint32_t proximo_endereco);

		// // Função que verifica se existe entrata ATIVA referente a operação "op_endereco". 
		// // Retornará valor da posição do elemento se encontrado, caso contrário retorna "quantidade_entradas".
		// int search(uint64_t op_endereco);

		// // Função que, de acordo com o endereco da operação atual + prefetcher.distance, 
		// // verificará e efetuará prefetch se necessário.
		// void prefetch(uint64_t op_endereco, cache * cache);
		// /*-------------------------------------------------*/
		// /*--------------------- DEBUG ---------------------*/
		// /*-------------------------------------------------*/

		// // Imprime todas as entradas do prefetcher e o endereço informado.
		void imprime();
		
};
// ============================================================================
// ============================================================================


// ============================================================================
// ============================================================================
class processor_t {
    private:    
    
    
    public:
		unsigned int delay;
		cache	*L1;
		cache	*L2;

		markov_prefetcher prefetcher;

		uint32_t miss_L1;
		uint32_t miss_L2;

		uint32_t total_acesso_L1;
		uint32_t total_acesso_L2;
		
		uint32_t total_writeback;
		
		// ====================================================================
		/// Methods
		// ====================================================================
		processor_t();
	    void allocate();
	    void clock();
	    void statistics();
		

		// Efetuará todo os processos relacionados a leitura do "endereço", retornando o delay gerado prlo processo.
		int read(uint32_t endereco);

		// Efetuará todo os processos relacionados a escrita do "endereço" uma vez que este já esteja na L1 (após read).
		void write(uint32_t endereco);
};

