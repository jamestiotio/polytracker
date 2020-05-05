#ifndef DFSAN_LOG_TAINT
#define DFSAN_LOG_TAINT

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <list>
#include <mutex>
#include <iostream>
#include <stdint.h>
#include "dfsan/dfsan.h"
#include "polyclang/polytracker.h"
#include "json.hpp"
#include <thread>

//TODO rename file

#define TAINT_GRANULARITY 1
#define MAX_NONCE 128
using json = nlohmann::json; 

typedef std::unordered_map<std::thread::id, std::vector<std::string>> thread_id_map; 
typedef std::unordered_map<std::string, std::unordered_set<taint_node_t*>> string_node_map; 
typedef std::unordered_map<std::string, Roaring> string_roaring_map; 

class targetInfo {
public:
	std::string target_name;
	int byte_start;
	int byte_end;
	bool is_open;
	targetInfo(std::string fname,
			int start,
			int end);
	~targetInfo();
};


class taintSourceManager {
private:
	//For ease of use we have a mono lock on all things here
	std::mutex taint_info_mutex;
	std::map<std::string, targetInfo*> name_target_map;
	std::unordered_map<int, targetInfo*> fd_target_map;
	std::unordered_map<FILE*, targetInfo*> file_target_map;
	//TODO use these
	std::unordered_map<targetInfo*, json> taint_metadata;
	//std::unordered_map<int, std::string> fd_metadata;
	//std::unordered_map<FILE*, std::string> file_metadata;

public:
	taintSourceManager();
	//TODO Free memory here
	~taintSourceManager();
	void createNewTargetInfo(std::string fname,
			int start, int end);
	bool createNewTaintInfo(std::string name, int fd);
	bool createNewTaintInfo(std::string name, FILE * ffd);
	targetInfo * getTargetInfo(std::string name);
	targetInfo * getTargetInfo(int fd);
	targetInfo * getTargetInfo(FILE* fd);
	bool isTracking(std::string name);
	bool isTracking(int fd);
	bool isTracking(FILE* fd);
	void closeSource(FILE * fd);
	void closeSource(int fd);
	targetInfo* findTargetInfo(std::string name);
	std::map<std::string, targetInfo*> getTargets();
	json getMetadata(targetInfo * targ_info);
};

/*
 * This manages the mapping between taint_label <--> taint_node 
 * NOTE an instance of this class is shared between the log manager and prop manager 
 * It has its own lock to prevent concurrency issues
 */
class taintMappingManager {
	public:
		taintMappingManager(char * shad_mem_ptr, char * taint_forest_ptr);
		~taintMappingManager(); 
		inline taint_node_t * getTaintNode(dfsan_label label); 
		inline dfsan_label getTaintLabel(taint_node_t * node);
	private: 
		std::mutex taint_mapping_lock;
	 	char * shad_mem; 
		char * forest_mem; 	
}; 


/*
 * Create labels and union labels together 
 */
class taintManager : public taintMappingManager, public taintSourceManager {
	public: 
		taintManager(decay_val init_decay_val, char* shad_mem, char* forest_ptr);
		~taintManager();
		void logCompare(dfsan_label some_label);
		void logOperation(dfsan_label some_label);
		int logFunctionEntry(char* fname);
		void logFunctionExit();
		void resetFrame(int* index);
		void output();
		dfsan_label getLastLabel();
		bool taintData(FILE * fd, char * mem, int offset, int len);
		bool taintData(int fd, char * mem, int offset, int len);
	 	dfsan_label createUnionLabel(dfsan_label l1, dfsan_label l2);
	 	dfsan_label createReturnLabel(int file_byte_offset);
	private:
		void checkMaxLabel(dfsan_label label);
		void outputRawTaintForest();
		void outputRawTaintSets();
		void addJsonVersion();
		void addTaintSources();
		void addJsonRuntimeCFG();
		void addCanonicalMapping();
		void addTaintedBlocks();
	 	dfsan_label createCanonicalLabel(int file_byte_offset);
		void taintTargetRange(char * mem, int offset, int len, int byte_start, int byte_end);

		dfsan_label _unionLabel(dfsan_label l1, dfsan_label l2, decay_val init_decay);
		std::unordered_map<dfsan_label, std::unordered_map<dfsan_label, dfsan_label>> union_table; 	
		decay_val taint_node_ttl;
		std::mutex taint_prop_lock;
		dfsan_label next_label;
		//TODO map LABEL to <offset, std::string (source)>
		std::map<dfsan_label, dfsan_label> canonical_mapping;
		std::list<std::pair<int, int>> taint_bytes_processed;
		thread_id_map thread_stack_map;
		string_node_map function_to_bytes;
		string_node_map function_to_cmp_bytes;
		std::unordered_map<std::string, std::unordered_set<std::string>> runtime_cfg;
		std::string outfile;
		json output_json;
};

#endif 

