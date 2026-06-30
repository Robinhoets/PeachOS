#include "memory.h"
/*
    The memset() function in C fills a contiguous block of memory with a specific 
    byte value.
*/
void* memset(void* ptr, int c, size_t size)
{
    char* c_ptr = (char*) ptr;
    for(int i=0; i<size; i++)
    {
        c_ptr[i] = (char) c;
    }
    return ptr;
}