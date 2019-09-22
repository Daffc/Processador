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

enum decision{
	STAKEN,
	WTAKEN,
	WNTAKEN,
	SNTAKEN
};

class block{
	public:
		uint64_t 	pc;
		uint64_t 	time;
		char		validade;
		char 		BHT;
};



class BTB_t {
	private:

	public:
		block 	group[4];
};