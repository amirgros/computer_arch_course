// ############ IMPORTS ############
#include <vector>

using namespace std;

// ############ Structs ############

struct block
{
    unsigned tag;
    bool valid;
    bool dirty;
    unsigned lastUsed;
};

typedef vector<block> way;

struct cache
{
    vector<way> Ways;
    unsigned Size;
    unsigned NumWays;
    unsigned Cyc;
    unsigned NumOps;
    unsigned NumMisses
};

struct memConfig_t
{
    cache L1;
    cache L2;
    unsigned MemCyc;
    unsigned BSize;
    unsigned WrAlloc;
    int numOperations;
    double totalCycles;
};

// ############ Globals ############

memConfig_t memConfig;

// ############ Functions ############

void memConfigInit(unsigned MemCyc, unsigned BSize, unsigned L1Size,
                   unsigned L2Size, unsigned L1num_ways, unsigned L2num_ways,
                   unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc);

block createBlock(unsigned tag);

void destroyBlock(block b); // delete content from block

//////////////////////////////////////////////////

// get to block location by address
block getBlockLocationInCache(unsigned address, cache &c);

// check if block matches address tag
bool checkHit(unsigned address, block b);

// write block to cache
void writeToCache(unsigned address, cache &c);

// LRU
block findBlockToReplace(unsigned address, cache &c);

//////////////////////////////////////////////////

void read(unsigned address);

void write(unsigned address);

double getL1MissRate();
double getL2MissRate();
double getAvgAccTime();