/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <cmath>

using namespace std;

struct line
{
	uint32_t tag;
	uint32_t target;	 // 2 lsbits are always 0
	int valid;			 // 0 = line empty, 1 = line full
	int history_index;	 // BTB line - history vector
	int fsm_table_index; // BTB line - fsm table
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
	unsigned tableSize;
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
	// check inputs:
	if ((btbSize != 1 && btbSize != 2 && btbSize != 4 &&
		 btbSize != 8 && btbSize != 16 && btbSize != 32) ||
		(historySize < 1 || historySize > 8) ||
		(tagSize < 0 || tagSize > (30 - (int)log2(BTB_context.btbSize))) ||
		(fsmState < 0 || fsmState > 3))
	{
		return -1;
	}

	// init inputs:
	BTB_context.btbSize = btbSize;		   // number of entries in BTB
	BTB_context.historySize = historySize; // size of history register
	BTB_context.tableSize = pow(2, historySize);
	BTB_context.tagSize = tagSize;
	BTB_context.fsmState = fsmState;
	BTB_context.isGlobalHist = isGlobalHist;
	BTB_context.isGlobalTable = isGlobalTable;
	BTB_context.Shared = Shared;
	try // resize memory allocation may fail
	{
		BTB_context.BTB.resize(btbSize);
		BTB_context.history.resize(isGlobalHist ? 1 : btbSize);
		for (int i = 0; i < BTB_context.BTB.size(); i++)
		{
			BTB_context.BTB[i].valid = 0; // all lines are empty
			// if global history: all BTB entries go to the same history_index
			// else: each BTB entry has its own history_index
			BTB_context.BTB[i].history_index = isGlobalHist ? 0 : i;
			BTB_context.BTB[i].fsm_table_index = isGlobalTable ? 0 : i;
		}

		BTB_context.fsm_tables.resize(isGlobalTable ? 1 : btbSize);
		for (int i = 0; i < BTB_context.fsm_tables.size(); i++)
		{
			// size of each fsm table: 2^history_size
			BTB_context.fsm_tables[i].fsm.resize(BTB_context.tableSize);
			for (int j = 0; j < BTB_context.fsm_tables[i].fsm.size(); j++)
			{
				// init fsms state
				BTB_context.fsm_tables[i].fsm[j] = fsmState;
			}
		}
	}
	catch (const bad_alloc &e)
	{
		return -1; // resize memory allocation failed
	}

	BTB_context.flushes_counter = 0;
	BTB_context.cmds_counter = 0;

	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst)
{
	// search tag in BTB
	for (int i = 0; i < BTB_context.BTB.size(); i++)
	{
		if (BTB_context.BTB[i].valid && BTB_context.BTB[i].tag == get_relevant_tag(pc))
		{ // command in btb:
			int const not_using_share = 0;
			int const using_share_lsb = 1;
			int const using_share_mid = 2;

			bool predicted_state;
			int hist_ind = BTB_context.BTB[i].history_index;
			int hist_val = BTB_context.history[hist_ind]; // value of relevant history register
			int fsm_tab_ind;
			if (BTB_context.isGlobalTable) // L\Gshare relevant
			{
				switch (BTB_context.Shared)
				{
				case not_using_share:
					fsm_tab_ind = BTB_context.BTB[i].fsm_table_index;
					break;
				case using_share_lsb:
					uint32_t mask = (1 << BTB_context.historySize) - 1;
					fsm_tab_ind = mask ^ (pc >> 2);
					break;
				case using_share_mid:
					uint32_t mask = (1 << BTB_context.historySize) - 1;
					fsm_tab_ind = mask ^ (pc >> 16);
					break;
				default:
					break;
				}
			}
			else
			{
				fsm_tab_ind = BTB_context.BTB[i].fsm_table_index;
			}
			predicted_state = BTB_context.fsm_tables[fsm_tab_ind].fsm[hist_val];
			if (predicted_state == 2 || predicted_state == 3)
			{ // taken
				*dst = BTB_context.BTB[i].target;
				return true;
			}
			else
			{ // not taken
				*dst = pc + 4;
				return false;
			}
		}

		// command not in btb
		*dst = pc + 4;
		return false;
	}
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	return;
}

void BP_GetStats(SIM_stats *curStats)
{
	curStats->br_num = BTB_context.cmds_counter;
	curStats->flush_num = BTB_context.flushes_counter;
	// size = BTBsize*(tagSize + targetsSize without 00 + valid bit) +
	// historySize*number of histories + tableSize * number of tables * 2 bits
	unsigned theoretic_size = BTB_context.btbSize * (BTB_context.tagSize + 30 + 1) +
							  BTB_context.historySize * (BTB_context.isGlobalHist ? 1 : BTB_context.btbSize) +
							  BTB_context.tableSize * 2 * (BTB_context.isGlobalTable ? 1 : BTB_context.btbSize);
	curStats->size = theoretic_size;

	// clear memory:
	BTB_context.BTB.clear();
	BTB_context.history.clear();
	for (int i = 0; i < BTB_context.fsm_tables.size(); i++)
	{
		BTB_context.fsm_tables[i].fsm.clear();
	}
	BTB_context.fsm_tables.clear();

	return;
}

// can use get_relevant_tag when entering new line to BTB (to get the new tag)
uint32_t get_relevant_tag(uint32_t pc)
{
	// pc has 2 bits of 0, then log2(btbSize) bits for entry, then tagSize bits for tag
	int tag_start = 2 + (int)log2(BTB_context.btbSize);
	uint32_t mask = (1 << BTB_context.tagSize) - 1; // creates a mask with tagSize bits of 1 (rest are 0)
	return (pc >> tag_start) & mask;
}
