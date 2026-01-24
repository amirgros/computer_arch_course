/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <vector>

using namespace std;

// threads context
typedef struct
{
	int regs[8];
	int pc;			// threads command line
	int when_ready; // cycle when the thread will be ready (used for load/store)
	bool finished;
} thread;

// cpu context
typedef struct
{
	vector<thread> threads;
	int num_threads;
	int current_thread;
	int threads_left;
	int num_cycles; // for CPI
	int count_inst; // for CPI
} cpu;

cpu b_cpu;
cpu fg_cpu;

void CORE_BlockedMT()
{
	printf("Starting Blocked Multithreading Simulation...\n");
	// init cpu and threads
	b_cpu.num_threads = SIM_GetThreadsNum();
	b_cpu.current_thread = 0;
	b_cpu.threads_left = b_cpu.num_threads;
	b_cpu.num_cycles = 0;
	b_cpu.count_inst = 0;
	b_cpu.threads.resize(b_cpu.num_threads);
	for (int i = 0; i < b_cpu.num_threads; i++)
	{
		for (int j = 0; j < REGS_COUNT; j++)
			b_cpu.threads[i].regs[j] = 0;
		b_cpu.threads[i].pc = 0;
		b_cpu.threads[i].when_ready = 0;
		b_cpu.threads[i].finished = 0;
	}

	while (b_cpu.threads_left > 0)
	{
		// choose thread:
		thread *curr_thread = &b_cpu.threads[b_cpu.current_thread];
		if (curr_thread->finished || curr_thread->when_ready > b_cpu.num_cycles)
		{
			// find next ready thread:
			bool found = false;
			for (int i = 1; i < b_cpu.num_threads && !found; i++)
			{
				int curr_num = (i + b_cpu.current_thread) % b_cpu.num_threads;
				if (!b_cpu.threads[curr_num].finished && b_cpu.threads[curr_num].when_ready <= b_cpu.num_cycles)
				{
					curr_thread = &b_cpu.threads[curr_num];
					b_cpu.current_thread = curr_num;
					b_cpu.num_cycles += SIM_GetSwitchCycles();
					found = true;
				}
			}
			if (!found) // idle - must wait
			{
				b_cpu.num_cycles++;
				continue;
			}
		}

		// execute:
		b_cpu.count_inst++;
		b_cpu.num_cycles++;
		Instruction curr_inst;
		SIM_MemInstRead(curr_thread->pc, &curr_inst, b_cpu.current_thread);
		int src1_val = curr_thread->regs[curr_inst.src1_index];
		int src2_val = (curr_inst.isSrc2Imm) ? curr_inst.src2_index_imm : curr_thread->regs[curr_inst.src2_index_imm];
		int latency = 0;
		switch (curr_inst.opcode)
		{
		case CMD_NOP:
			break;
		case CMD_ADD:
			curr_thread->regs[curr_inst.dst_index] = src1_val + src2_val;
			break;
		case CMD_ADDI:
			curr_thread->regs[curr_inst.dst_index] = src1_val + src2_val;
			break;
		case CMD_SUB:
			curr_thread->regs[curr_inst.dst_index] = src1_val - src2_val;
			break;
		case CMD_SUBI:
			curr_thread->regs[curr_inst.dst_index] = src1_val - src2_val;
			break;
		case CMD_LOAD:
		{
			uint32_t addr = src1_val + src2_val;
			int32_t val;
			SIM_MemDataRead(addr, &val);
			curr_thread->regs[curr_inst.dst_index] = val;
			latency = SIM_GetLoadLat();
			curr_thread->when_ready = b_cpu.num_cycles + latency; // add plus 1?
			break;
		}
		case CMD_STORE:
		{
			uint32_t addr = curr_thread->regs[curr_inst.dst_index] + src2_val;
			SIM_MemDataWrite(addr, src1_val);
			latency = SIM_GetStoreLat();
			curr_thread->when_ready = b_cpu.num_cycles + latency;
			break;
		}
		case CMD_HALT:
			curr_thread->finished = true;
			b_cpu.threads_left--;
			break;
		default:
			break;
		}
		curr_thread->pc++; // I think starts from 0
	}

	// while threads not finished
	// choose thread - current/ready
	// execute
	// check opcode
	// update regs - arithmetics / load store
	// update pc
	// update thread context - if opcode have latency
	// free structs and memory
}

void CORE_FinegrainedMT()
{
	// init cpu and threads
	fg_cpu.num_threads = SIM_GetThreadsNum();
	fg_cpu.current_thread = 0;
	fg_cpu.threads_left = fg_cpu.num_threads;
	fg_cpu.num_cycles = 0;
	fg_cpu.count_inst = 0;
	fg_cpu.threads.resize(fg_cpu.num_threads);
	for (int i = 0; i < fg_cpu.num_threads; i++)
	{
		for (int j = 0; j < REGS_COUNT; j++)
			fg_cpu.threads[i].regs[j] = 0;
		fg_cpu.threads[i].pc = 0;
		fg_cpu.threads[i].when_ready = 0;
		fg_cpu.threads[i].finished = 0;
	}

	// while not all threads are finished
	while(fg_cpu.threads_left > 0){
		thread *curr_thread = &fg_cpu.threads[fg_cpu.current_thread];
		// check if this thread is unavailable, find another if yes
		if (curr_thread->finished || curr_thread->when_ready > fg_cpu.num_cycles)
		{
			bool found = false; 
			for (int i = 1; i < fg_cpu.num_threads && !found; i++)
			{
				int curr_num = (i + fg_cpu.current_thread) % fg_cpu.num_threads;
				// check if curr_num thread is available
				if (!fg_cpu.threads[curr_num].finished && fg_cpu.threads[curr_num].when_ready <= fg_cpu.num_cycles)
				{
					curr_thread = &fg_cpu.threads[curr_num];
					fg_cpu.current_thread = curr_num;
					found = true;
				}
			}
			if (!found) // idle - must wait
			{
				fg_cpu.num_cycles++;
				continue;
			}
		}
	
		// execute
		fg_cpu.count_inst++;
		fg_cpu.num_cycles++;
		Instruction curr_inst;
		SIM_MemInstRead(curr_thread->pc, &curr_inst, fg_cpu.current_thread);
		int src1_val = curr_thread->regs[curr_inst.src1_index];
		int src2_val = (curr_inst.isSrc2Imm) ? curr_inst.src2_index_imm : curr_thread->regs[curr_inst.src2_index_imm];
		int latency = 0;
		
		// check opcode
		switch (curr_inst.opcode)
		{
		case CMD_NOP:
			break;
		case CMD_ADD:
			curr_thread->regs[curr_inst.dst_index] = src1_val + src2_val;
			break;
		case CMD_ADDI:
			curr_thread->regs[curr_inst.dst_index] = src1_val + src2_val;
			break;
		case CMD_SUB:
			curr_thread->regs[curr_inst.dst_index] = src1_val - src2_val;
			break;
		case CMD_SUBI:
			curr_thread->regs[curr_inst.dst_index] = src1_val - src2_val;
			break;
		case CMD_LOAD:
		{
			uint32_t addr = src1_val + src2_val;
			int32_t val;
			SIM_MemDataRead(addr, &val);
			curr_thread->regs[curr_inst.dst_index] = val;
			latency = SIM_GetLoadLat();
			curr_thread->when_ready = fg_cpu.num_cycles + latency; // add plus 1? - no, num_cycles already counted this cycle
			break;
		}
		case CMD_STORE:
		{
			uint32_t addr = curr_thread->regs[curr_inst.dst_index] + src2_val;
			SIM_MemDataWrite(addr, src1_val);
			latency = SIM_GetStoreLat();
			curr_thread->when_ready = fg_cpu.num_cycles + latency;
			break;
		}
		case CMD_HALT:
			curr_thread->finished = true;
			fg_cpu.threads_left--;
			break;
		default:
			break;
		}
		// update pc
		curr_thread->pc++;	
		// automatically switch to next available thread
		fg_cpu.current_thread = (fg_cpu.current_thread + 1) % fg_cpu.num_threads;
	}	
}

double CORE_BlockedMT_CPI()
{
	return (double)b_cpu.num_cycles / b_cpu.count_inst;
}

double CORE_FinegrainedMT_CPI()
{
	return (double)fg_cpu.num_cycles / fg_cpu.count_inst;
}

void CORE_BlockedMT_CTX(tcontext *context, int threadid)
{
	for (int i = 0; i < REGS_COUNT; i++)
	{
		context[threadid].reg[i] = b_cpu.threads[threadid].regs[i];
	}
}

void CORE_FinegrainedMT_CTX(tcontext *context, int threadid)
{
	for (int i = 0; i < REGS_COUNT; i++)
	{
		context[threadid].reg[i] = fg_cpu.threads[threadid].regs[i];
	}
}
