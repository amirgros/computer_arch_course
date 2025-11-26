/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <cmath>

using namespace std;

struct line
{
	uint32_t tag;
	uint32_t target;
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
	for (int i = 0; i < BTB_context.BTB.size(); i++)
	{
		if (BTB_context.BTB[i].tag == get_relevant_tag(pc)) // assuming not relevant bits of "tag" are also 0
		{
			// command in btb:
			// go to history -> go to relevant state machine -> get prediction
			// predicted Taken: return true, target
			// predicted NotTaken: return false

			int const not_using_share = 0;
			int const using_share_lsb = 1;
			int const using_share_mid = 2;

			switch (BTB_context.Shared) // history -> do xors accordingly -> relevant state machine
			{
			case not_using_share:
				break;
			case using_share_lsb:
				break;
			case using_share_mid:
				break;
			default:
				break;
			}
		}
		
		// command not in btb: return false

		return false;
	}
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

// can use get_relevant_tag when entering new line to BTB (to get the new tag)
uint32_t get_relevant_tag(uint32_t pc)
{
	// pc has 2 bits of 0, then log2(btbSize) bits for entry, then tagSize bits for tag
	int tag_start = 2 + log2(BTB_context.btbSize);
	int tag_finish = tag_start + BTB_context.tagSize - 1;
	// create a mask with 0 where pc bits are not for tag,
	// and 1 on bits for the tag:
	// (0u = 32 bits of 0, ~0u = 32 bits of 1)
	uint32_t mask = ((~0u) >> tag_start) & ((~0u) << tag_finish);
	return pc & mask;
}
