/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <cmath>

using namespace std;

// #########################################
// Structs, Globals and Macros
// ##########################################
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
	vector<int> fsm;
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

#define NOT_USING_SHARE 0
#define USING_SHARE_LSB 1
#define USING_SHARE_MID 2

// #########################################
// Aux functions
// ##########################################

uint32_t get_relevant_tag(uint32_t pc)
{
	// pc has 2 bits of 0, then log2(btbSize) bits for entry, then tagSize bits for tag
	int tag_start = 2 + (int)log2(BTB_context.btbSize);
	uint32_t mask = (1 << BTB_context.tagSize) - 1; // creates a mask with tagSize bits of 1 (rest are 0)
	return (pc >> tag_start) & mask;
}

int get_btb_index(uint32_t pc)
{
	// return btb index from pc (y in {zzzzzzzzzz yyyy 00})
	uint32_t mask = BTB_context.btbSize - 1;
	return (pc >> 2) & mask;
}

void add_to_history(int history_index, bool taken)
{
	// shift history 1  bit left  and add taken bit to history register
	uint32_t history_size_mask = ((1 << BTB_context.historySize) - 1);
	BTB_context.history[history_index] = ((BTB_context.history[history_index] << 1) | (taken ? 1 : 0)) & history_size_mask;
}

int fsm_index(uint32_t pc, int history_index)
{
	// return the index of relevant row in FSM table
	int hist_val = BTB_context.history[history_index];
	int fsm_ind = hist_val;
	if (BTB_context.isGlobalTable) // L\Gshare relevant
	{
		uint32_t mask = (1 << BTB_context.historySize) - 1;
		switch (BTB_context.Shared)
		{
		case NOT_USING_SHARE:
			break;
		case USING_SHARE_LSB:
			fsm_ind = (mask & (pc >> 2)) ^ hist_val;
			break;
		case USING_SHARE_MID:
			fsm_ind = (mask & (pc >> 16)) ^ hist_val;
			break;
		default:
			break;
		}
	}
	return fsm_ind;
}

void update_fsm(uint32_t pc, int fsm_table_index, int history_index, bool taken)
{
	uint32_t fsm_row_index = fsm_index(pc, history_index);
	if (taken)
	{
		BTB_context.fsm_tables[fsm_table_index].fsm[fsm_row_index] = min(BTB_context.fsm_tables[fsm_table_index].fsm[fsm_row_index] + 1, 3);
	}
	else
	{
		BTB_context.fsm_tables[fsm_table_index].fsm[fsm_row_index] = max(BTB_context.fsm_tables[fsm_table_index].fsm[fsm_row_index] - 1, 0);
	}
}

void reset_fsm(int fsm_table_index)
{
	for (size_t j = 0; j < BTB_context.fsm_tables[fsm_table_index].fsm.size(); j++)
	{
		// init fsms state
		BTB_context.fsm_tables[fsm_table_index].fsm[j] = BTB_context.fsmState;
	}
}

void reset_history(int history_index)
{
	BTB_context.history[history_index] = 0;
}

// #########################################
// BP functions
// ##########################################

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared)
{
	// check inputs:
	if ((btbSize != 1 && btbSize != 2 && btbSize != 4 &&
		 btbSize != 8 && btbSize != 16 && btbSize != 32) ||
		(historySize < 1 || historySize > 8) ||
		(fsmState < 0 || fsmState > 3))
	{
		return -1;
	}
	if (tagSize < 0 || tagSize > (30 - (unsigned)log2(BTB_context.btbSize)))
		return -1;

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
		for (size_t i = 0; i < BTB_context.history.size(); i++)
		{
			reset_history(i);
		}
		for (size_t i = 0; i < BTB_context.BTB.size(); i++)
		{
			BTB_context.BTB[i].valid = 0; // all lines are empty
			// if global history: all BTB entries go to the same history_index
			// else: each BTB entry has its own history_index
			BTB_context.BTB[i].history_index = isGlobalHist ? 0 : i;
			BTB_context.BTB[i].fsm_table_index = isGlobalTable ? 0 : i;
		}

		BTB_context.fsm_tables.resize(isGlobalTable ? 1 : btbSize);
		for (size_t i = 0; i < BTB_context.fsm_tables.size(); i++)
		{
			// size of each fsm table: 2^history_size
			BTB_context.fsm_tables[i].fsm.resize(BTB_context.tableSize);
			reset_fsm(i);
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
	// check if pc in btb:
	uint32_t pc_tag = get_relevant_tag(pc);
	int BTB_entry = get_btb_index(pc);
	if (BTB_context.BTB[BTB_entry].valid && BTB_context.BTB[BTB_entry].tag == pc_tag)
	{ // command in btb:
		unsigned predicted_state;
		int hist_ind = BTB_context.BTB[BTB_entry].history_index;
		int fsm_tab_ind = BTB_context.BTB[BTB_entry].fsm_table_index;
		int fsm_ind = fsm_index(pc, hist_ind);
		predicted_state = BTB_context.fsm_tables[fsm_tab_ind].fsm[fsm_ind];
		if (predicted_state == 2 || predicted_state == 3)
		{ // taken
			*dst = BTB_context.BTB[BTB_entry].target;
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

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	// statistics update
	BTB_context.cmds_counter++;
	if ((taken && targetPc != pred_dst) || (!taken && pc + 4 != pred_dst))
		BTB_context.flushes_counter++; // mispredicted

	int btb_index = get_btb_index(pc);
	if (BTB_context.BTB[btb_index].valid == 0)
	{
		// command not in BTB and line is empty: add to BTB, update fsm and history
		BTB_context.BTB[btb_index].tag = get_relevant_tag(pc);
		BTB_context.BTB[btb_index].target = targetPc;
		BTB_context.BTB[btb_index].valid = 1;
		update_fsm(pc, BTB_context.BTB[btb_index].fsm_table_index, BTB_context.BTB[btb_index].history_index, taken);
		add_to_history(BTB_context.BTB[btb_index].history_index, taken); // make sure to update fsm first!
	}
	else
	{
		if (BTB_context.BTB[btb_index].tag == get_relevant_tag(pc))
		{
			// command found: update fsm, history, target, and check for flush
			BTB_context.BTB[btb_index].target = targetPc;
			update_fsm(pc, BTB_context.BTB[btb_index].fsm_table_index, BTB_context.BTB[btb_index].history_index, taken);
			add_to_history(BTB_context.BTB[btb_index].history_index, taken); // make sure to update fsm first!
		}
		else
		{
			// line full, different tag: replace line, reset and update fsm and history
			BTB_context.BTB[btb_index].tag = get_relevant_tag(pc);
			BTB_context.BTB[btb_index].target = targetPc;
			// reset fsm and history if local
			if (BTB_context.isGlobalTable == 0)
				reset_fsm(BTB_context.BTB[btb_index].fsm_table_index);
			if (BTB_context.isGlobalHist == 0)
				reset_history(BTB_context.BTB[btb_index].history_index);
			update_fsm(pc, BTB_context.BTB[btb_index].fsm_table_index, BTB_context.BTB[btb_index].history_index, taken);
			add_to_history(BTB_context.BTB[btb_index].history_index, taken); // make sure to update fsm first!
		}
	}
	return;
}

void BP_GetStats(SIM_stats *curStats)
{
	curStats->br_num = BTB_context.cmds_counter;
	curStats->flush_num = BTB_context.flushes_counter;
	// size = BTBsize*(tagSize + target size without 00 + valid bit) +
	// historySize*number of histories + tableSize * number of tables * 2 bits
	unsigned theoretic_size = BTB_context.btbSize * (BTB_context.tagSize + 30 + 1) +
							  BTB_context.historySize * (BTB_context.isGlobalHist ? 1 : BTB_context.btbSize) +
							  BTB_context.tableSize * 2 * (BTB_context.isGlobalTable ? 1 : BTB_context.btbSize);
	curStats->size = theoretic_size;

	// clear memory:
	BTB_context.BTB.clear();
	BTB_context.history.clear();
	for (size_t i = 0; i < BTB_context.fsm_tables.size(); i++)
	{
		BTB_context.fsm_tables[i].fsm.clear();
	}
	BTB_context.fsm_tables.clear();

	return;
}