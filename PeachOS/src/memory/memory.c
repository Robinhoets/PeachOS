#include "memory.h"
/*
    Purpose: Fills a contiguous block of memory with a specific byte value.
    Parameter ptr: Address to fill.
    Parateter c: Byte value.
    Parameter size: Size of to fill (in bytes).
    Return: ptr to the address.
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