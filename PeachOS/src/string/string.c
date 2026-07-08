#include "string.h"

/*
    Purpose: Determine the length of a string.
    Parameter ptr: Pointer to location of string.
    Return: Return length of string.
*/
int strlen(const char* ptr)
{
    int i=0;
    while(*ptr != 0)
    {
        i++;
        ptr += 1;
    }

    return i;
}

/*
    Purpose: Determine the length of a string but don't go past max length.
    Parameter ptr: Pointer to location of string.
    Parameter max: Max location of end of string.
    Return: Return length of string.
*/
int strnlen(const char* ptr, int max)
{
    int i = 0;
    for(i=0; i<max; i++)
    {
        if(ptr[i] == 0)
            break;
    }

    return i;
}

/*
    Purpose: Check if character is a digit.
    Parameter c: Character to check.
    Return: True if digit passed (decimal 48-57 maps 0-9), false if out of bounds.
*/
bool isdigit(char c)
{
    return c>= 48 && c <= 57;
}

/*
    Purpose: Compute integer value from ascii character.
    Parameter c: Character to convert.
    Return: Integer value.
*/
int tonumericdigit(char c)
{
    return c - 48;
}