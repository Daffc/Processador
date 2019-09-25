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
#define OFFSET 1024

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
		uint16_t	global_history;
		char 		*direction_predictors;
		char		*choice_predictor;
};