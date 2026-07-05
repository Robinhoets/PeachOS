#include "paging.h"
#include "memory/heap/kheap.h"
#include "status.h"

void paging_load_directory(uint32_t* directory);

static uint32_t* current_directory = 0;

/*
    Map out the memory linearly.
    Creates a page directory with a bunch of page tables that cover the entire
    4gb of RAM.
    Note: We can then manipulate these values anyway we like to point virtual 
    addresses anywhere else.
*/
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags)
{
    uint32_t* directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    int offset = 0;
    for(int i=0; i<PAGING_TOTAL_ENTRIES_PER_TABLE; i++)
    {
        uint32_t* entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
        for(int b=0; b<PAGING_TOTAL_ENTRIES_PER_TABLE; b++)
        {
            // Individual pages not necessarily writable
            entry[b] = (offset + (b * PAGING_PAGE_SIZE)) | flags;
        }
        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
        // Make directory writeable
        directory[i] = (uint32_t)entry | flags | PAGING_IS_WRITEABLE;
    }

    struct paging_4gb_chunk* chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;
    return chunk_4gb;
}

/*
    Paging switch function.
    In charge of switching betweent the directories.
*/
void paging_switch(uint32_t* directory)
{
    paging_load_directory(directory);
    current_directory = directory;
}

/*
    Get directory entry
*/
uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk)
{
    return chunk->directory_entry;
}

/*
    Purpose: Make sure paging is aligned. Heap sizes are 4096 which will work directly
                if we make sure blocks aligned.
    Parameter addr:
    Return: True if page is aligned. Else, fasle
*/
bool paging_is_aligned(void* addr)
{
    return ((uint32_t) addr % PAGING_PAGE_SIZE) == 0;
}

/*
    Purpose: Take a virtual address, calculate which directory index in the directory
                entry table and which in the page table are responsible for this
                virtual address.
    Parameter virtual_address: The adress given to determine directory and page table.
    Parameter directory_index_out: The directory.
    Parameter table_index_out: The page table.
    Return: 
*/
int paging_get_indexes(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out)
{
    // response variable
    int res = 0;
    if(!paging_is_aligned(virtual_address))
    {
        res = -EINVARG;
        goto out;
    }

    *directory_index_out = ((uint32_t)virtual_address / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE));
    *table_index_out = ((uint32_t) virtual_address % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE) / PAGING_PAGE_SIZE);

out:
    return res;
}

/*
    Purpose: Set a page.
    Parameter directory: Directory of page to be added.
    Parameter virt: Virtual address of page to be added.
    Parameter val: Value to be added.
    Return: 
*/
int paging_set(uint32_t* directory, void* virt, uint32_t val)
{
    if(!paging_is_aligned(virt))
    {
        return -EINVARG;
    }
    uint32_t directory_index = 0;
    uint32_t table_index = 0;
    int res = paging_get_indexes(virt, &directory_index, &table_index);
    if(res < 0)
    {
        return res; // error and return
    }

    // get page directory entry - pointer to page table
    uint32_t entry = directory[directory_index];
    // extract only the address (remove flags)
    uint32_t* table = (uint32_t*)(entry & 0xfffff000);
    table[table_index] = val;

    return 0;

}