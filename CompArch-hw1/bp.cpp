/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <cmath>

using namespace std;

struct line
{
	int tag;
	int target;
	int history_index;
};

struct fsm_table
{
	vector<unsigned> fsm;
};

struct BTB_context_t
{
	vector<line> BTB;
	vector<unsigned> history;
	vector<fsm_table> fsm_tables;
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmState;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared;
	unsigned cmds_counter;
	unsigned flushes_counter;
};

struct BTB_context_t BTB_context;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared)
{
	BTB_context.btbSize = btbSize;		   // number of entries in BTB
	BTB_context.historySize = historySize; // size of history register
	BTB_context.tagSize = tagSize;
	BTB_context.fsmState = fsmState;
	BTB_context.isGlobalHist = isGlobalHist;
	BTB_context.isGlobalTable = isGlobalTable;
	BTB_context.Shared = Shared;

	BTB_context.BTB.resize(btbSize);

	BTB_context.history.resize(isGlobalHist ? 1 : btbSize);
	// if global history: all BTB entries go to the same history_index
	// else: each BTB entry has its own history_index
	for (int i = 0; i < BTB_context.BTB.size(); i++)
	{
		BTB_context.BTB[i].history_index = isGlobalHist ? 0 : i;
	}

	BTB_context.fsm_tables.resize(isGlobalTable ? 1 : btbSize);
	for (int i = 0; i < BTB_context.fsm_tables.size(); i++)
	{
		// size of each fsm table: 2^history_size
		BTB_context.fsm_tables[i].fsm.resize(pow(2, historySize));
		for (int j = 0; j < BTB_context.fsm_tables[i].fsm.size(); j++)
		{
			// init fsms state
			BTB_context.fsm_tables[i].fsm[j] = fsmState;
		}
	}

	BTB_context.flushes_counter = 0;
	BTB_context.cmds_counter = 0;

	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst)
{
	// search tag in BTB
		// command in btb:
			// go to history -> go to relevant state machine -> get prediction
				// predicted Taken: return true, target
				// predicted NotTaken: return false

	// command not in btb: return false
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	return;
}

void BP_GetStats(SIM_stats *curStats)
{
	// delete BTB_context
	return;
}
