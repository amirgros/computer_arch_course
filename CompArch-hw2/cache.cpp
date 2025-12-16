#include "cache.h"
#include <vector>
using namespace std;

block createBlock(unsigned tag)
{
    struct block b;
    b.tag = tag;
    b.valid = 0;
    b.dirty = 0;
    b.lastUsed = 0;
    return b;
}

void destroyBlock(block b)  // after findBlockToReplace
{
    b.valid = 0;
    b.dirty = 0;
    b.lastUsed = 0;
    b.tag = 0;
}

void memConfigInit(unsigned MemCyc, unsigned BSize, unsigned L1Size,
                   unsigned L2Size, unsigned L1NumWays, unsigned L2NumWays,
                   unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc)
{
    struct cache L1, L2;
    L1.Size = L1Size;
    L1.NumWays = L1NumWays;
    L1.Cyc = L1Cyc;
    L2.Size = L2Size;
    L2.NumWays = L2NumWays;
    L2.Cyc = L2Cyc;

    memConfig.L1 = L1;
    memConfig.L2 = L2;

    memConfig.BSize = BSize;
    memConfig.MemCyc = MemCyc;
    memConfig.WrAlloc = WrAlloc;
}