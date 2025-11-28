/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <cmath>

using namespace std;


// #########################################
// Structs and Globals
// ##########################################
struct line
{
	uint32_t tag;
	uint32_t target;	 // sava only 30 bits, because 2 lsbits are always 0
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
	unsigned tagSize;
	unsigned fsmState;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared;
	unsigned cmds_counter;
	unsigned flushes_counter;
};

struct BTB_context_t BTB_context;


// #########################################
// Aux functions
// ##########################################

// can use get_relevant_tag when entering new line to BTB (to get the new tag)
uint32_t get_relevant_tag(uint32_t pc)
{
	// pc has 2 bits of 0, then log2(btbSize) bits for entry, then tagSize bits for tag
	int tag_start = 2 + log2(BTB_context.btbSize);
	int tag_finish = 32 - tag_start - BTB_context.tagSize;
	// create a mask with 0 where pc bits are not for tag,
	// and 1 on bits for the tag:
	// (0u = 32 bits of 0, ~0u = 32 bits of 1)
	uint32_t mask = (((~0u) >> tag_start) << tag_start) & (((~0u) >> tag_finish) << tag_finish);
	return pc & mask; // AYA - (>> tag_start)? to take only the relevant bits for the tag
}

uint32_t get_btb_index(uint32_t pc)
{
	// return btb index from pc (y in {zzzzzzzzzz yyyy 00})
	uint32_t bits_in_index = log2(BTB_context.btbSize);
	uint32_t mask = ((1 << bits_in_index) - 1) << 2;
	return (pc & mask) >> 2;
}

void add_to_history(int history_index, bool taken)
{
	// shift history 1  bit left  and add taken bit to history register
	uint32_t history_size_mask = ((1 << BTB_context.historySize) - 1);
	BTB_context.history[history_index] = ((BTB_context.history[history_index] << 1) | (taken ? 1 : 0)) & history_size_mask;
}

void update_fsm(int fsm_table_index, int history_index, bool taken)
{
	uint32_t fsm_row_index = (int)(BTB_context.history[history_index]);
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
	for (int j = 0; j < BTB_context.fsm_tables[fsm_table_index].fsm.size(); j++)
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
	BTB_context.btbSize = btbSize;		   // number of entries in BTB
	BTB_context.historySize = historySize; // size of history register
	BTB_context.tagSize = tagSize;
	BTB_context.fsmState = fsmState;
	BTB_context.isGlobalHist = isGlobalHist;
	BTB_context.isGlobalTable = isGlobalTable;
	BTB_context.Shared = Shared;

	BTB_context.BTB.resize(btbSize);

	BTB_context.history.resize(isGlobalHist ? 1 : btbSize);
	for (int i = 0; i < BTB_context.history.size(); i++)	// AYA - I added history init to 0 to avoid garbage values
	{
		reset_history(i);
	}
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
		BTB_context.fsm_tables[i].fsm.resize(pow(2, historySize));
		reset_fsm(i);	// AYA - I moved your loop to a function and replaced it with a call
	}

	BTB_context.flushes_counter = 0;
	BTB_context.cmds_counter = 0;

	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst)
{
	// search tag in BTB
	for (int i = 0; i < BTB_context.BTB.size(); i++) // AYA - use get_btb_index(pc) instead of loop, since it's direct mapping
	{
		if (BTB_context.BTB[i].valid && BTB_context.BTB[i].tag == get_relevant_tag(pc)) // assuming not relevant bits of "tag" are also 0
		{
			// command in btb:
			// go to history -> go to relevant state machine -> get prediction
			// predicted Taken: return true, target
			// predicted NotTaken: return false

			int const not_using_share = 0;
			int const using_share_lsb = 1;
			int const using_share_mid = 2;
			bool predicted_state;
			switch (BTB_context.Shared) // history -> do xors accordingly -> relevant state machine
			{
			case not_using_share: // local
				if (BTB_context.isGlobalHist)
				{
				}
				else
				{
					int hist = BTB_context.BTB[i].history_index;
					int hist_val = BTB_context.history[hist]; // value of relevant history register
					int fsm_tab = BTB_context.BTB[i].fsm_table_index;
					predicted_state = BTB_context.fsm_tables[fsm_tab].fsm[hist_val];
				}
				break;
			case using_share_lsb:
				break;
			case using_share_mid:
				break;
			default:
				break;
			}
			// predicted_state -> taken \ not taken

		}

		// command not in btb: return false and update BTB

		return false; // AYA -  wrong placing? will return after first iteration always
	}
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) // ALL - not sure what to do with pred_dst
{
	int btb_index = get_btb_index(pc);
	if (BTB_context.BTB[btb_index].valid == 0)
	{
		// command not in BTB and line is empty: add to BTB, update fsm and history
		BTB_context.BTB[btb_index].tag = get_relevant_tag(pc);
		BTB_context.BTB[btb_index].target = targetPc;
		BTB_context.BTB[btb_index].valid = 1;
		update_fsm(BTB_context.BTB[btb_index].fsm_table_index, BTB_context.BTB[btb_index].history_index, taken);
		add_to_history(BTB_context.BTB[btb_index].history_index, taken); // make sure to update fsm first!
	}
	else
	{
		if (BTB_context.BTB[btb_index].tag == get_relevant_tag(pc))
		{
			// command found: update fsm, history, target
			BTB_context.BTB[btb_index].target = targetPc;
			update_fsm(BTB_context.BTB[btb_index].fsm_table_index, BTB_context.BTB[btb_index].history_index, taken);
			add_to_history(BTB_context.BTB[btb_index].history_index, taken); // make sure to update fsm first!
		} 
		else
		{
			// line full, different tag: replace line, reset and update fsm and history
			BTB_context.BTB[btb_index].tag = get_relevant_tag(pc);
			BTB_context.BTB[btb_index].target = targetPc;
			// reset fsm and history if local
			if (BTB_context.isGlobalTable == 0) reset_fsm(BTB_context.BTB[btb_index].fsm_table_index);
			if (BTB_context.isGlobalHist == 0) reset_history(BTB_context.BTB[btb_index].history_index);
			update_fsm(BTB_context.BTB[btb_index].fsm_table_index, BTB_context.BTB[btb_index].history_index, taken);
			add_to_history(BTB_context.BTB[btb_index].history_index, taken); // make sure to update fsm first!
		}
	}
	return;
}

void BP_GetStats(SIM_stats *curStats)
{
	// delete BTB_context
	return;
}


