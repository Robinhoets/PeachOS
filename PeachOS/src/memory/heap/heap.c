#include "heap.h"
#include "kernel.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>

/*
    Validate that the pointer for table and end pointer is 
    correct for the table provided.
    (has this person miscalculated)
*/
static int heap_validate_table(void* ptr, void* end, struct heap_table* table)
{
    int res = 0;

    size_t table_size = (size_t)(end - ptr);
    size_t total_blocks = table_size / PEACHOS_HEAP_BLOCK_SIZE;
    if(table->total != total_blocks)
    {
        res = -EINVARG;
        goto out;
    }

out:
    return res;    
}

/*
    Return true if pointer points to the heap segment in memory
*/
static bool heap_validate_alignment(void* ptr)
{
    return((unsigned int)ptr % PEACHOS_HEAP_BLOCK_SIZE) == 0;
}

/*
    Create heap.
    If return 0, okay.
    If return below 0, error.
        - The negative value represents error code.

    Args: Accept pointer to heap.
          Accept pointer to heap data pool.
          Accept pointer to end of heap.
          Accept heap table.
*/
int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
    int res = 0;
    // check if block aligned with start and end of heap
    if(!heap_validate_alignment(ptr) || !heap_validate_alignment(end))
    {
        return -EINVARG;
        goto out;
    }

    // initialize heap to 0
    memset(heap, 0, sizeof(struct heap));
    // set heap start address to pointer provided
    heap->saddr = ptr;  
    // set heap table to address provided
    heap->table = table;

    res = heap_validate_table(ptr, end, table);
    if(res < 0)
    {
        goto out;
    }

    // initialize hep to empty
    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);
    
out:
    return res;
    
}

/*
    Determine size
*/
static uint32_t heap_align_value_to_upper(uint32_t val)
{
    // check for alignments
    if((val % PEACHOS_HEAP_BLOCK_SIZE) == 0)
    {
        return val;
    }
    // ex) 5000 - (5000 % 4096)
    val = (val - (val % PEACHOS_HEAP_BLOCK_SIZE));
    // ex) 4096 + 4096
    val += PEACHOS_HEAP_BLOCK_SIZE;
    return val;
}

/*
    test entry type
    extract entry type
*/
static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    // return first 4 bits
    return entry & 0x0f;
}

int heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
    // get access to the entry table
    struct heap_table* table = heap->table;
    int bc = 0; // current block
    int bs = -1;// block start

    // go through entry block table
    // look for free blocks
    // simple implementation - would possibly point to start of block
    for(size_t i=0; i<table->total; i++)
    {
        // if there aren't enough sequential blocks, reset the indexes
        if(heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
            bc = 0;
            bs = -1;
            continue;
        }

        // if this is the first block
        // bs equals start block
        if(bs == -1)
        {
            bs =i;
        }
        bc++;
        // if we find the start block that works (has enough blocks in series)
        if(bc == total_blocks)
        {
            break;
        }
    }

    if(bs == -1)
    {
        return -ENOMEM;  // out of memory
    }

    return bs;
}


void* heap_block_to_address(struct heap* heap, int block)
{
    return heap->saddr + (block * PEACHOS_HEAP_BLOCK_SIZE);
}


void heap_mark_blocks_taken(struct heap* heap, int start_block, int total_blocks)
{
    int end_block = (start_block + total_blocks) - 1;

    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;

    // Show that next block is taken
    if(total_blocks > 1)
    {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for(int i=start_block; i<=end_block; i++)
    {
        heap->table->entries[i] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
        if(i != end_block -1)
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

/*
    find blocks needed and return address
*/
void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
    void* address = 0;

    // look through heap entry table and see if it can find enough room
    // for these total blocks
    // if found, return first block. From the entry table.
    int start_block = heap_get_start_block(heap, total_blocks);
    if(start_block < 0)
    {
        goto out;
    }

    // convert block to address
    address = heap_block_to_address(heap, start_block);

    // mark the blocks as taken
    heap_mark_blocks_taken(heap, start_block, total_blocks);

out:
    return address;
}

void* heap_malloc(struct heap* heap, size_t size)
{
    size_t aligned_size = heap_align_value_to_upper(size);
    // ex) 8192 / 4096 = 2
    uint32_t total_blocks = aligned_size / PEACHOS_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}

void heap_free(struct heap* heap, void* ptr)
{
    return 0;
}