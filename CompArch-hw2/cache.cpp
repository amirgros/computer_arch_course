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

void destroyBlock(block b) // after findBlockToReplace
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
    for (int i = 0; i < L1.Ways.size(); i++)
    {
        L1.Ways[i].resize(1 << (L1Size - BSize - L1NumWays));
        for (int j = 0; j < L1.Ways[i].size(); j++)
        {
            L1.Ways[i][j] = createBlock(0);
        }
    }
    L1.LRU.resize(1 << (L1Size - BSize - L1NumWays));
    for (int i = 0; i < L1.LRU.size(); i++)
    {
        L1.LRU[i].resize((1 << L1NumWays) - 1, 0); // k-1 bits for PLRU tree
    }
    L2.Size = L2Size;
    L2.NumWays = L2NumWays;
    L2.Cyc = L2Cyc;
    L2.NumCalls = 0;
    L2.NumMisses = 0;
    L2.Ways.resize(L2NumWays);
    for (int i = 0; i < L2.Ways.size(); i++)
    {
        L2.Ways[i].resize(1 << (L2Size - BSize - L2NumWays));
        for (int j = 0; j < L2.Ways[i].size(); j++)
        {
            L2.Ways[i][j] = createBlock(0);
        }
    }
    L2.LRU.resize(1 << (L2Size - BSize - L2NumWays));
    for (int i = 0; i < L2.LRU.size(); i++)
    {
        L2.LRU[i].resize((1 << L2NumWays) - 1, 0); // k-1 bits for PLRU tree
    }

    memConfig.L1 = L1;
    memConfig.L2 = L2;

    memConfig.BSize = BSize;
    memConfig.MemCyc = MemCyc;
    memConfig.WrAlloc = WrAlloc;
    memConfig.numOperations = 0;
    memConfig.totalCycles = 0;
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
    for (unsigned i = 0; i < c.NumWays - 1; i++)
    { // go over way bits
        if ((way >> i) & 1)
        { // set 1 and go right
            lru[index] = 1;
            index = 2 * index + 2;
        }
        else
        { // set 0 and go left
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
    for (unsigned i = 0; i < c.NumWays - 1; i++)
    { // go against the flow
        if (lru[index])
        { // 1 - go left
            index = 2 * index + 1;
            wayToReplace += 1 << i;
        }
        else
        { // 0 - go right
            index = 2 * index + 2;
        }
    }
    return wayToReplace;
}

// check if block matches address tag - 1 for hit, 0 for miss
int checkHit(unsigned address, cache &c)
{
    // devide address into tag, set, offset
    unsigned set = getSet(address, c);
    unsigned tag = getTag(address, c);

    for (unsigned way = 0; way < c.NumWays; way++)
    {
        block b = c.Ways[way][set];
        if (b.valid && b.tag == tag)
        {
            updateLRU(c, set, way);
            return 1;
        }
    }
    // block not found - read miss
    return 0;
}

// checks hit and set dirty - 1 for WriteHit, 0 for WriteMiss (then should check write allocate policy)
int writeToCache(unsigned address, cache &c) // relevant for L1
{
    // devide address into tag, set, offset
    unsigned set = getSet(address, c);
    unsigned tag = getTag(address, c);

    for (unsigned way = 0; way < c.NumWays; way++)
    {
        block b = c.Ways[way][set];
        if (b.valid && b.tag == tag)
        {
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
    unsigned set = getSet(address, c);
    unsigned tag = getTag(address, c);

    // if there is an empty slot, add block there
    for (unsigned way = 0; way < c.NumWays; way++)
    {
        block b = c.Ways[way][set];
        if (!b.valid)
        {
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
    if (replacedBlock.dirty)
    {
        return (replacedBlock.tag << (c.Size - c.NumWays)) | (set << memConfig.BSize);
    }
    // replaced block was not dirty and can be discarded
    return 0;
}

void read(unsigned address)
{
    memConfig.numOperations++;
    memConfig.totalCycles += memConfig.L1.Cyc;
    memConfig.L1.NumCalls++;
    int hit1 = checkHit(address, memConfig.L1);
    if (hit1)
    {
        return;
    }
    // miss in L1, check L2
    memConfig.L1.NumMisses++;
    memConfig.totalCycles += memConfig.L2.Cyc;
    memConfig.L2.NumCalls++;
    int hit2 = checkHit(address, memConfig.L2);
    int dirty_addr1, dirty_addr2;
    if (hit2)
    {
        dirty_addr1 = addBlockToCache(address, memConfig.L1);
        if (dirty_addr1 != 0)
        {
            writeToCache(dirty_addr1, memConfig.L2);
        }
        return;
    }
    // miss in L2
    memConfig.L2.NumMisses++;
    memConfig.totalCycles += memConfig.MemCyc; // get info from memory

    dirty_addr1 = addBlockToCache(address, memConfig.L1);
    if (dirty_addr1 != 0)
    { // dirty
        dirty_addr2 = addBlockToCache(dirty_addr1, memConfig.L2);
        if (dirty_addr2 != 0)
        { // dirty
        }
    }

    dirty_addr2 = addBlockToCache(address, memConfig.L2);
    if (dirty_addr2 != 0)
    { // dirty
    }

    // check hit in each cache until hit or miss in L2
    // add blocks to needed caches on miss
    // addBlockToCache, returns removed block's address if replaced a dirty block - need to write back (meaning set dirty in L2)
    // if miss in L1 and L2, add block to L1 first, then to L2, or write back might run over block in L2
    // update stats: misses and calls for caches, cycles and ops for memConfig
}

void write(unsigned address)
{
    memConfig.numOperations++;

    memConfig.totalCycles += memConfig.L1.Cyc;
    memConfig.L1.NumCalls++;
    int hit1 = writeToCache(address, memConfig.L1); // also marks dirty
    if (hit1)
    {
        return;
    }
    // miss in L1
    memConfig.L1.NumMisses++;

    memConfig.totalCycles += memConfig.L2.Cyc;
    memConfig.L2.NumCalls++;
    int hit2 = checkHit(address, memConfig.L2);
    if (hit2)
    {
        if (memConfig.WrAlloc == 0) // no write allocate
        {
            writeToCache(address, memConfig.L2); // mark dirty
            return;
        }
        // write allocate:
        int dirty_addr1 = addBlockToCache(address, memConfig.L1);
        if (dirty_addr1 != 0)
        {
            writeToCache(dirty_addr1, memConfig.L2);
        }
        writeToCache(address, memConfig.L1); // update to dirty
        return;
    }
    // miss in L2
    memConfig.L2.NumMisses++;

    memConfig.totalCycles += memConfig.MemCyc;
    if (memConfig.WrAlloc == 0) // no write allocate
    {
        return;
    }
    // write allocate:
    // bring to L2:
    int dirty_addr2 = addBlockToCache(address, memConfig.L2);
    if (dirty_addr2 != 0)
    {
    }
    // bring to L1:
    int dirty_addr1 = addBlockToCache(address, memConfig.L1);
    if (dirty_addr1 != 0)
    {
        writeToCache(dirty_addr1, memConfig.L2);
    }
    // update to dirty in L1:
    writeToCache(address, memConfig.L1);

    // check write hit in each cache until hit or miss in L2
    // if write alloc, add blocks to needed caches on miss
    // addBlockToCache, returns removed block's address if replaced a dirty block - need to write back (meaning set dirty in L2)
    // if miss in L1 and L2, add block to L1 first, then to L2, or write back might run over block in L2
    // update stats: misses and calls for caches, cycles and ops for memConfig
}

/* on write allocate - check in L1. if miss - check in L2. if miss - bring from memory to L2, but don't update. bring to L1 and update.
*/
