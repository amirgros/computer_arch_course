/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>

// cpu context
typedef struct{
	thread* threads;
	int num_threads;
	int current_thread; 
	int threads_left;
	int num_cycles; // for CPI
} cpu;

cpu b_cpu;
cpu fg_cpu;

// threads context
typedef struct{
	int regs[8];
	int pc;	// threads command line. check if line num or addr (aligned to 4 bytes)
	int when_ready; // cycle when the thread will be ready
	bool is_waiting;
	bool finished;
} thread;




void CORE_BlockedMT() {
	// init mem
	// init cpu and threads
	// while threads not finished
		// choose thread - current/ready
		// execute
			// check opcode
			// update regs - arithmetics / load store
			// update pc
		// update thread context - if opcode have latency
	// free structs and memory
}

void CORE_FinegrainedMT() {
	// init mem
	// init cpu and threads
	// while threads not finished
		// automatically switch to next thread
		// execute
			// check opcode
			// update regs - arithmetics / load store
			// update pc
		// update thread context - if opcode have latency
	// free structs and memory
}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
