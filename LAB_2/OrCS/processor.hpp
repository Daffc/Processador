class block {
	private:

	public:
		uint32_t pc;
		uint64_t time;
};

class cache{
	private:


	public:
		block 	 *blocos;
		uint32_t offset_operator;
		uint32_t index_operator;
		uint32_t quantidade_vias;
		uint32_t tamanho;
		unsigned char latencia;

		// Procurar por bloco de informado na cache.
		int search(uint32_t address);
		// Criar e instanciar cache.
		void initialize(uint32_t offset, uint32_t index, uint32_t vias, uint32_t tamanho, unsigned char delay);
		//Limpar memória usada por cache.
		void free_cache();

};

// ============================================================================
// ============================================================================
class processor_t {
    private:    
    
    
    public:
		unsigned char delay;
		cache	*L1;
		cache	*L2;

		// ====================================================================
		/// Methods
		// ====================================================================
		processor_t();
	    void allocate();
	    void clock();
	    void statistics();
};

