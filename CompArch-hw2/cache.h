// ############ IMPORTS ############
#include <vector>

using namespace std;

// ############ Structs ############

struct block
{
    unsigned tag;
    bool valid;
    bool dirty;
};

typedef vector<block> way;
typedef vector<bool> wayLRU;

struct cache
{
    vector<way> Ways;
    vector<wayLRU> LRU; // PLRU - binary tree LRU per set
    unsigned Size;
    unsigned NumWays;
    unsigned Cyc;
    unsigned NumCalls;
    unsigned NumMisses;
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

unsigned getSet(unsigned address, cache &c);
unsigned getTag(unsigned address, cache &c);

// update LRU info after hit
void updateLRU(cache &c, unsigned set, unsigned way);

// LRU
unsigned findWayToReplace(cache &c, unsigned set);

// check if block matches address tag - return 1 if hit, 0 if miss
int checkHit(unsigned address, cache &c);

// write block to cache
int writeToCache(unsigned address, cache &c);

unsigned addBlockToCache(unsigned address, cache &c);

//////////////////////////////////////////////////

void read(unsigned address);

void write(unsigned address);

double getL1MissRate();
double getL2MissRate();
double getAvgAccTime();