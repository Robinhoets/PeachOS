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

/*
    Purpose: Compares the first n bytes of the memory areas s1 and s2.
    Paramter s1: First memory area to be compared.
    Parameter s2: Second memory area to be compared. 
    Parameter count: size of the bytes to compare
    Return: The  memcmp() function returns an integer less than, equal
       to, or greater than zero if the first n  bytes  of  s1  is
       found,  respectively,  to  be  less  than, to match, or be
       greater than the first n bytes of s2.

       For a nonzero return value, the sign is determined by  the
       sign  of  the  difference  between the first pair of bytes
       (interpreted as unsigned char) that differ in s1 and s2.

       If n is zero, the return value is zero.
*/
int memcmp(void* s1, void* s2, int count)
{
    char* c1 = s1;
    char* c2 = s2;
    while(count-- > 0)
    {
        if(*c1++ != *c2++)
        {
            // if c1 less than c2, return -1, else 1
            return c1[-1] < c2[-1] ? -1 : 1;
        }
    }

    return 0;
}