#include "cache.h"
#include <vector>
using namespace std;

memConfig_t memConfig;

block createBlock(unsigned tag)
{
    struct block b;
    b.tag = tag;
    b.valid = 0;
    b.dirty = 0;
    b.counter = 0;
    return b;
}

void destroyBlock(block &b) // after findBlockToReplace
{
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
            L1.Ways[i][j].counter = i; // init LRU
        }
    }
    L2.Size = L2Size;
    L2.NumWays = L2NumWays;
    L2.Cyc = L2Cyc;
    L2.NumCalls = 0;
    L2.NumMisses = 0;
    L2.Ways.resize(1 << L2NumWays);
    for (int i = 0; i < L2.Ways.size(); i++)
    {
        L2.Ways[i].resize(1 << (L2Size - BSize - L2NumWays));
        for (int j = 0; j < L2.Ways[i].size(); j++)
        {
            L2.Ways[i][j] = createBlock(0);
            L2.Ways[i][j].counter = i;
        }
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
    return (address >> memConfig.BSize) & set_mask;
}

unsigned getTag(unsigned address, cache &c)
{
    return address >> (c.Size - c.NumWays); // shift right by num bits in set + offset
}

void updateLRU(cache &c, unsigned set_index, unsigned way_index)
{
    int tmp = c.Ways[way_index][set_index].counter;
    c.Ways[way_index][set_index].counter = c.Ways.size() - 1;
    for (int i = 0; i < c.Ways.size(); i++)
    {
        if (i != way_index && c.Ways[i][set_index].counter > tmp)
        {
            c.Ways[i][set_index].counter--;
        }
    }
}

// LRU - return way index of block to replace
unsigned findWayToReplace(cache &c, unsigned set_index)
{
    unsigned wayToReplace = 0;
    int count = c.Ways.size();
    for (int i = 0; i < c.Ways.size(); i++)
    {
        if (c.Ways[i][set_index].counter < count)
        {
            count = c.Ways[i][set_index].counter;
            wayToReplace = i;
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

    unsigned actualNumWays = 1 << c.NumWays;
    for (unsigned way = 0; way < actualNumWays; way++)
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

// returns 1 if block found & deleted, else 0
int deleteBlock(unsigned address, cache &c)
{
    // devide address into tag, set, offset
    unsigned set = getSet(address, c);
    unsigned tag = getTag(address, c);

    unsigned actualNumWays = 1 << c.NumWays;
    for (unsigned way = 0; way < actualNumWays; way++)
    {
        block b = c.Ways[way][set];
        if (b.valid && b.tag == tag)
        {
            destroyBlock(c.Ways[way][set]);
            return 1;
        }
    }
    // block not found
    return 0;
}

// checks hit and set dirty - 1 for WriteHit, 0 for WriteMiss (then should check write allocate policy)
int writeToCache(unsigned address, cache &c) // relevant for L1
{
    // devide address into tag, set, offset
    unsigned set = getSet(address, c);
    unsigned tag = getTag(address, c);

    unsigned actualNumWays = 1 << c.NumWays;
    for (unsigned way = 0; way < actualNumWays; way++)
    {
        block b = c.Ways[way][set];
        if (b.valid && b.tag == tag)
        {
            c.Ways[way][set].dirty = 1;
            updateLRU(c, set, way);
            return 1;
        }
    }
    // block not found - write miss
    return 0;
}

// call on miss to add block to cache, returns address of replaced block when replacing dirty, else return 0
addrDirty addBlockToCache(unsigned address, cache &c)
{
    addrDirty addr_dirty;
    // devide address into tag, set, offset
    unsigned set = getSet(address, c);
    unsigned tag = getTag(address, c);

    // if there is an empty slot, add block there
    unsigned actualNumWays = 1 << c.NumWays;
    for (unsigned way = 0; way < actualNumWays; way++)
    {
        block b = c.Ways[way][set];
        if (!b.valid)
        {
            // found an empty slot
            c.Ways[way][set].tag = tag;
            c.Ways[way][set].valid = 1;
            c.Ways[way][set].dirty = 0;
            updateLRU(c, set, way);
            addr_dirty.deleted = 0;
            return addr_dirty;
        }
    }

    // all slots are occupied - need to replace one
    unsigned replaceWay = findWayToReplace(c, set);
    block replacedBlock = c.Ways[replaceWay][set];
    c.Ways[replaceWay][set].tag = tag;
    c.Ways[replaceWay][set].valid = 1;
    c.Ways[replaceWay][set].dirty = 0;
    updateLRU(c, set, replaceWay);

    addr_dirty.address = (replacedBlock.tag << (c.Size - c.NumWays)) | (set << memConfig.BSize);
    addr_dirty.deleted = 1;
    // check if replaced block was dirty
    if (replacedBlock.dirty)
    {
        addr_dirty.dirty = 1;
        return addr_dirty;
    }
    // replaced block was not dirty and can be discarded
    addr_dirty.dirty = 0;
    return addr_dirty;
}

void read(unsigned address)
{
    memConfig.numOperations++;

    // SEARCH IN L1
    memConfig.totalCycles += memConfig.L1.Cyc;
    memConfig.L1.NumCalls++;
    int hit1 = checkHit(address, memConfig.L1);
    if (hit1) // FOUND IN L1
    {
        return;
    }
    // MISS IN L1 -> SEARCH IN L2
    memConfig.L1.NumMisses++;

    memConfig.totalCycles += memConfig.L2.Cyc;
    memConfig.L2.NumCalls++;
    int hit2 = checkHit(address, memConfig.L2);
    addrDirty dirty_addr1, dirty_addr2;
    if (hit2) // FOUND IN L2
    {
        dirty_addr1 = addBlockToCache(address, memConfig.L1);
        if (dirty_addr1.deleted && dirty_addr1.dirty)
        {
            writeToCache(dirty_addr1.address, memConfig.L2);
        }
        return;
    }
    // MISS IN L2 -> BRING FROM MEMORY
    memConfig.L2.NumMisses++;
    memConfig.totalCycles += memConfig.MemCyc; // get info from memory

    dirty_addr2 = addBlockToCache(address, memConfig.L2);
    if (dirty_addr2.deleted)
    {
        deleteBlock(dirty_addr2.address, memConfig.L1);
    }
    
    dirty_addr1 = addBlockToCache(address, memConfig.L1);
    if (dirty_addr1.deleted && dirty_addr1.dirty)
    { // dirty
        writeToCache(dirty_addr1.address, memConfig.L2);
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

    // SEARCH IN L1
    memConfig.totalCycles += memConfig.L1.Cyc;
    memConfig.L1.NumCalls++;
    int hit1 = writeToCache(address, memConfig.L1); // marks dirty
    if (hit1) // FOUND IN L1
    {
        return;
    }
    // MISS IN L1 -> SEARCH IN L2
    memConfig.L1.NumMisses++;

    memConfig.totalCycles += memConfig.L2.Cyc;
    memConfig.L2.NumCalls++;
    int hit2 = checkHit(address, memConfig.L2);
    if (hit2) // FOUND IN L2
    {
        if (memConfig.WrAlloc == 0) // no write allocate
        {
            writeToCache(address, memConfig.L2); // mark dirty
            return;
        }
        // write allocate:
        addrDirty dirty_addr1 = addBlockToCache(address, memConfig.L1);
        if (dirty_addr1.deleted && dirty_addr1.dirty)
        {
            writeToCache(dirty_addr1.address, memConfig.L2);
        }
        writeToCache(address, memConfig.L1); // update to dirty
        return;
    }
    // MISS IN L2 -> BRING FROM MEMORY
    memConfig.L2.NumMisses++;

    memConfig.totalCycles += memConfig.MemCyc;
    if (memConfig.WrAlloc == 0) // no write allocate
    {
        return;
    }
    // write allocate:
    // bring to L2:
    addrDirty dirty_addr2 = addBlockToCache(address, memConfig.L2);
    if (dirty_addr2.deleted)
    {
        deleteBlock(dirty_addr2.address, memConfig.L1);
    }
    // bring to L1:
    addrDirty dirty_addr1 = addBlockToCache(address, memConfig.L1);
    if (dirty_addr1.deleted && dirty_addr1.dirty)
    {
        writeToCache(dirty_addr1.address, memConfig.L2);
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


 /* 
 LRU explanation:
 each set saves a counter that says when it was recently used (biggest = most recently)
 the set's counter is in relation to the same sets (lines) in the other ways
 
 way1      way2         way3
|  0 |    |  1  |    |    2   |
|  2 |    |  0  |    |    1   |
|  1 |    |  2  |    |    0   |
|  1 |    |  2  |    |    0   |

if we went now to set0 in way2 the counters will change to:

 way1      way2         way3
|  0 |    |  2  |    |    1   |
|  2 |    |  0  |    |    1   |
|  1 |    |  2  |    |    0   |
|  1 |    |  2  |    |    0   |
*/