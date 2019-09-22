// ============================================================================
// ============================================================================
class processor_t {
    private:    
    
    
    public:

		// ====================================================================
		/// Methods
		// ====================================================================
		processor_t();
	    void allocate();
	    void clock();
	    void statistics();
};

class block{
	public:

		uint64_t 	pc;
		uint64_t 	time;
		char		validade;
};

class BTB_t {
	private:

	public:
		block 	group[4];
		char 	BHT;
};