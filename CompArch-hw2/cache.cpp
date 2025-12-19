#include "cache.h"
#include <vector>
using namespace std;

block createBlock(unsigned tag)
{
    struct block b;
    b.tag = tag;
    b.valid = 0;
    b.dirty = 0;
    return b;
}

void destroyBlock(block b)  // after findBlockToReplace
{
    // ALL - is this function needed if we only use stack copies of blocks (without 'new' heap-allocation)?
    b.valid = 0;
    b.dirty = 0;
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
    L1.NumCalls = 0;
    L1.NumMisses = 0;
    L1.Ways.resize(1 << L1NumWays);
    for (int i = 0; i < L1.Ways.size(); i++) {
        L1.Ways[i].resize(1 << (L1Size - BSize - L1NumWays));
        for (int j = 0; j < L1.Ways[i].size(); j++) {
            L1.Ways[i][j] = createBlock(0);
        }
    }
    L1.LRU.resize(1 << (L1Size - BSize - L1NumWays));
    for (int i = 0; i < L1.LRU.size(); i++) {
        L1.LRU[i].resize((1 << L1NumWays) - 1 , 0); // k-1 bits for PLRU tree
    }
    L2.Size = L2Size;
    L2.NumWays = L2NumWays;
    L2.Cyc = L2Cyc;
    L2.NumCalls = 0;
    L2.NumMisses = 0;
    L2.Ways.resize(L2NumWays);
    for (int i = 0; i < L2.Ways.size(); i++) {
        L2.Ways[i].resize(1 << (L2Size - BSize - L2NumWays));
        for (int j = 0; j < L2.Ways[i].size(); j++) {
            L2.Ways[i][j] = createBlock(0);
        }
    }
    L2.LRU.resize(1 << (L2Size - BSize - L2NumWays));
    for (int i = 0; i < L2.LRU.size(); i++) {
        L2.LRU[i].resize((1 << L2NumWays) - 1, 0); // k-1 bits for PLRU tree
    }

    memConfig.L1 = L1;
    memConfig.L2 = L2;

    memConfig.BSize = BSize;
    memConfig.MemCyc = MemCyc;
    memConfig.WrAlloc = WrAlloc;
}

unsigned getOffset(unsigned address)
{
    unsigned offset_mask = (1 << memConfig.BSize) - 1;
    return address & offset_mask;
}

unsigned getSet(unsigned address, cache &c)
{
    unsigned set_mask = ((1 << (c.Size - memConfig.BSize - c.NumWays)) - 1);
    return (address & set_mask) >> memConfig.BSize;
}

unsigned getTag(unsigned address, cache &c)
{
    return address >> (c.Size - c.NumWays); // shift right by num bits in set + offset
}

void updateLRU(cache &c, unsigned set, unsigned way)
{
    vector<bool> &lru = c.LRU[set];
    unsigned index = 0;
    for (unsigned i = 0; i < c.NumWays - 1; i++) { // go over way bits
        if ((way >> i) & 1 ) { // set 1 and go right
            lru[index] = 1;
            index = 2 * index + 2;
        } else { // set 0 and go left
            lru[index] = 0;
            index = 2 * index + 1;
        }
    }
}

// LRU - return way index of block to replace
unsigned findWayToReplace(cache &c, unsigned set)
{
    vector<bool> &lru = c.LRU[set];
    unsigned wayToReplace = 0;
    unsigned index = 0;
    for (unsigned i = 0; i < c.NumWays - 1; i++) { // go against the flow
        if (lru[index]) { // 1 - go left
            index = 2 * index + 1;
            wayToReplace += 1 << i;
        } else { // 0 - go right
            index = 2 * index + 2;
        }
    }
    return wayToReplace;
}

// check if block matches address tag - 1 for hit, 0 for miss
int checkHit(unsigned address, cache &c)
{
    // devide address into tag, set, offset
    unsigned offset = getOffset(address);
    unsigned set = getSet(address, c);
    unsigned tag = getTag(address, c);

    for (unsigned way = 0; way < c.NumWays; way++) {
        block b = c.Ways[way][set];
        if (b.valid && b.tag == tag) {
            updateLRU(c, set, way);
            return 1;
        }    
    }
    // block not found - read miss
    return 0;
}    

// checks hit and set dirty - 1 for WriteHit, 0 for WriteMiss (then should check write allocate policy)
int writeToCache(unsigned address, cache &c)
{
    // devide address into tag, set, offset
    unsigned offset = getOffset(address);
    unsigned set = getSet(address, c);
    unsigned tag = getTag(address, c);

    for (unsigned way = 0; way < c.NumWays; way++) {
        block b = c.Ways[way][set];
        if (b.valid && b.tag == tag) {
            b.dirty = 1;
            updateLRU(c, set, way);
            return 1;
        }    
    }    
    // block not found - write miss
    return 0;
}    

// call on miss to add block to cache, returns address of replaced block when replacing dirty, else return 0
unsigned addBlockToCache(unsigned address, cache &c)
{
    // devide address into tag, set, offset
    unsigned offset = getOffset(address);
    unsigned set = getSet(address, c);
    unsigned tag = getTag(address, c);

    // if there is an empty slot, add block there
    for (unsigned way = 0; way < c.NumWays; way++) {
        block b = c.Ways[way][set];
        if (!b.valid) {
            // found an empty slot
            c.Ways[way][set] = createBlock(tag);
            c.Ways[way][set].valid = 1;
            updateLRU(c, set, way);
            return 0;
        }
    }

    // all slots are occupied - need to replace one
    unsigned replaceWay = findWayToReplace(c, set);
    block replacedBlock = c.Ways[replaceWay][set];
    c.Ways[replaceWay][set] = createBlock(tag);
    c.Ways[replaceWay][set].valid = 1;
    updateLRU(c, set, replaceWay);

    // check if replaced block was dirty
    if (replacedBlock.dirty) {
        return (replacedBlock.tag << (c.Size - c.NumWays)) | (set << memConfig.BSize) | offset;
    }
    // replaced block was not dirty and can be discarded
    return 0;
}


void read(unsigned address)
{
    // check hit in each cache until hit or miss in L2
    // add blocks to needed caches on miss
        // addBlockToCache, returns removed block's address if replaced a dirty block - need to write back (meaning set dirty in L2)
        // if miss in L1 and L2, add block to L1 first, then to L2, or write back might run over block in L2
    // update stats: misses and calls for caches, cycles and ops for memConfig
}

void write(unsigned address)
{
    // check write hit in each cache until hit or miss in L2
    // if write alloc, add blocks to needed caches on miss
        // addBlockToCache, returns removed block's address if replaced a dirty block - need to write back (meaning set dirty in L2)
        // if miss in L1 and L2, add block to L1 first, then to L2, or write back might run over block in L2
    // update stats: misses and calls for caches, cycles and ops for memConfig
}
