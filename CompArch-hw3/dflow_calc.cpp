/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <vector>

using namespace std;

struct node{
    int id;
    int latency;
    int dep_1_id;
    int dep_2_id;
    int depth;
};

typedef vector<node> t_graph;

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    // allocate graph dynamically so the returned ProgCtx remains valid
    t_graph* graph_ptr = new vector<node>(numOfInsts);

    int regs_in_use[32];
    for (int i = 0; i < 32; ++i) regs_in_use[i] = -1;
    // resize graph (already sized via constructor)
    // dependencies by src
    // update depth by max{dependency depths} + opcode latency

    return (ProgCtx)(graph_ptr);
}

void freeProgCtx(ProgCtx ctx) {
    if (ctx == PROG_CTX_NULL) return;
    t_graph* graph = (t_graph*)ctx;
    delete graph;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    t_graph graph = *(t_graph*)ctx;
    return graph[theInst].depth;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    t_graph graph = *(t_graph*)ctx;
    if(theInst >= graph.size()) {
        return -1;
    }
    node inst = graph[theInst];
    *src1DepInst = inst.dep_1_id;
    *src2DepInst = inst.dep_2_id;
    return 0;
}

int getProgDepth(ProgCtx ctx) {
    // find max depth in linear pass on graph vector
    t_graph graph = *(t_graph*)ctx;
    int max_depth = 0;
    for(int i = 0; i < graph.size(); i++) {
        if(graph[i].depth > max_depth) {
            max_depth = graph[i].depth;
        }
    }
    return max_depth;
}


