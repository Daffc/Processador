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
#define WAYS 4
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
};



class BTB_t {
	private:

	public:
		block 	group[WAYS];
};

class Predictor{
	private:

	public:
		uint16_t		global_history;
		unsigned char 	*direction_predictors[2];
		unsigned char	*choice_predictor;
};