/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <vector>
#include <algorithm>

using namespace std;

struct node
{
    int id;
    int latency;
    int dep_1_id;
    int dep_2_id;
    int depth;
};

typedef vector<node> t_graph;

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts)
{
    // allocate graph dynamically so the returned ProgCtx remains valid
    t_graph *graph_ptr = new t_graph(numOfInsts);

    int regs_in_use[32];
    for (int i = 0; i < 32; ++i)
        regs_in_use[i] = -1;
    // resize graph (already sized via constructor)
    // dependencies by src
    // update depth by max{dependency depths} + opcode latency
    for (int i = 0; i < numOfInsts; i++)
    {
        node &curr = (*graph_ptr)[i];
        InstInfo curr_trace = progTrace[i];
        curr.id = i;
        curr.latency = opsLatency[curr_trace.opcode];
        curr.dep_1_id = regs_in_use[curr_trace.src1Idx];
        curr.dep_2_id = regs_in_use[curr_trace.src2Idx];
        int depth1, depth2;
        if (curr.dep_1_id == -1)
            depth1 = 0;
        else
            depth1 = (*graph_ptr)[curr.dep_1_id].depth + (*graph_ptr)[curr.dep_1_id].latency;

        if (curr.dep_2_id == -1)
            depth2 = 0;
        else
            depth2 = (*graph_ptr)[curr.dep_2_id].depth + (*graph_ptr)[curr.dep_2_id].latency;

        int max_depth = max(depth1 > 0 ? depth1 : 0, depth2 > 0 ? depth2 : 0);
        curr.depth = max_depth;
        regs_in_use[curr_trace.dstIdx] = i;
    }

    return (ProgCtx)(graph_ptr);
}

void freeProgCtx(ProgCtx ctx)
{
    if (ctx == PROG_CTX_NULL)
        return;
    t_graph *graph = (t_graph *)ctx;
    delete graph;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst)
{
    t_graph graph = *(t_graph *)ctx;
    return graph[theInst].depth;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst)
{
    t_graph graph = *(t_graph *)ctx;
    if (theInst >= graph.size())
    {
        return -1;
    }
    node inst = graph[theInst];
    *src1DepInst = inst.dep_1_id;
    *src2DepInst = inst.dep_2_id;
    return 0;
}

int getProgDepth(ProgCtx ctx)
{
    // find max depth in linear pass on graph vector
    t_graph graph = *(t_graph *)ctx;
    int max_depth = 0;
    for (int i = 0; i < graph.size(); i++)
    {
        if (graph[i].depth + graph[i].latency > max_depth)
        {
            max_depth = graph[i].depth + graph[i].latency;
        }
    }
    return max_depth;
}
