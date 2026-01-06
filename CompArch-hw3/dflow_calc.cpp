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
    int depth; //?
};

vector<node> graph;

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    int regs_in_use[32] = {-1};
    // resize graph
    // dependencies by src
    // update depth by max{dependency depths} + opcode latency

    return PROG_CTX_NULL;
}

void freeProgCtx(ProgCtx ctx) {
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    return graph[theInst].depth;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    
    return -1;
}

int getProgDepth(ProgCtx ctx) {
    // find max depth in linear pass on graph vector
    return 0;
}


