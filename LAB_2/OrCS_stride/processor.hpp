
#define BLOCK_BIT_SIZE 6
#define DELAY_PRINC_MEM 200


/***********************************************/
/*********** ELEMENTOS DA CACHE ****************/
/***********************************************/

class block {
	private:

	public:
		uint32_t endereco;
		uint64_t time;
		char	validade;
		char	dirty;
		uint64_t ready_cycle;
};

class cache{
	private:


	public:
		char     nome[5] ;
		block 	 *blocos;
		uint32_t offset_operator;
		uint32_t index_operator;
		uint32_t quantidade_vias;
		uint32_t tamanho;
		unsigned char latencia;

		// Criar e instanciar cache.
		void initialize(const char* nome, uint32_t offset, uint32_t index, uint32_t vias, uint32_t tamanho, unsigned char delay);
		
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

enum estados{
	INVALIDO,
	TREINAMENTO,
	ATIVO
};
class entrada_stride{
	public:
		uint32_t 	tag;
		uint32_t 	last_address;
		int32_t		stride;
		estados		status;
};

class stride_prefetcher{
	public:
		entrada_stride 	*entradas;
		unsigned char	quantidade_entradas;
		unsigned char 	distance;
		unsigned char 	degree;

		
		// Inicializa Dimenções e entradas do prefecher.
		void initialize(unsigned char  size, unsigned char  distance, unsigned char  degree);
		// Armazena endereço da instrução e da requisição em entrada de prefetcher.
		void allocate(uint32_t op_endereco, uint32_t mem_endereco);
		// Recebe endereço de instrução "op_endereco" e endereço de memória "mem_endereco" 
		// e atualiza (caso exista entrada para instrução "op_endereco") "status" do prefetcher 
		// de acordo com a politica indicada .
		void train(uint32_t op_endereco, uint32_t mem_endereco);

		// Função que verifica se existe entrata ATIVA referente a operação "op_endereco"
		int search(uint32_t op_endereco);

		/*-------------------------------------------------*/
		/*--------------------- DEBUG ---------------------*/
		/*-------------------------------------------------*/

		// Imprime todas as entradas do prefetcher e o endereço informado.
		void imprime(uint32_t endereco);
};
// ============================================================================
// ============================================================================
class processor_t {
    private:    
    
    
    public:
		unsigned int delay;
		cache	*L1;
		cache	*L2;
		stride_prefetcher prefetcher;

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
		

		// Efetuará todo os processos relacionados a leitura do "endereço", retornando o delay gerado pelo processo.
		int read(uint32_t op_endereco, uint32_t endereco_mem);

		// Efetuará todo os processos relacionados a escrita do "endereço" uma vez que este já esteja na L1 (após read).
		void write(uint32_t endereco);
};

