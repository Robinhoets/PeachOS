#include "string.h"

/*
    Purpose: To make a char lower case.
    Parameter s1: char to make lower case
    Return:
*/
char tolower(char s1)
{
    // ex 65 + 32 = 97 --- A to a --- in ascii
    if(s1 >= 65 && s1 <= 90)
    {
        s1 += 32;
    }

    return s1;
}


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
    Purpose: Read the string until it finds a null terminator or it finds the terminator from variable.
                Can make use of spaces.
    Parameter str: String to parse.
    Parameter max: Max length of the str.
    Parameter terminator: Other terminator supplied that signifies the end.
    Return: i, length of str. 
*/
int strnlen_terminator(const char* str, int max, char terminator)
{
    int i = 0;
    for(i=0; i<max; i++)
    {
        if(str[i] == '\0' || str[i] == terminator)
            break;
    }

    return i;
}

/*
    Purpose: FAT16 is not case sensitive. This function strings without caring about the case.
    Parameter str1: Address of first string to compare.
    Parameter str2: Address of second string to compare.
    Parameter n: Maximum number of bytes to compare
    Return: 0 if same strings.
*/
int istrncmp(const char* s1, const char* s2, int n)
{
    unsigned char u1, u2;

    while(n-- > 0)
    {
        u1 = (unsigned char)*s1++;
        u2 = (unsigned char)*s2++;
        if(u1 != u2 && tolower(u1) != tolower(u2))
            return u1 - u2;
        if(u1 == '\0')
            return 0;
    }

    return 0;
}

/*
    Purpose: Compare to strings.
    Parameter str1: Address of first string to compare.
    Parameter str2: Address of second string to compare.
    Parameter n: Maximum number of bytes to compare
    Return: 0 if same strings.
*/
int strncmp(const char* str1, const char* str2, int n)
{
    unsigned char u1, u2;

    while(n-- > 0)
    {
        u1 = (unsigned char)*str1++;
        u2 = (unsigned char)*str2++;
        if(u1 != u2)
            return u1 - u2;
        if(u1 == '\0')
            return 0;
    }

    return 0;
}

/*
    Purpose: Copy one addresses string to another address.
    Parameter dest: Destination address of string.
    Parameter src: Source address of string.
    Return:
*/
char* strcpy(char* dest, const char* src)
{
    char* res = dest;
    while(*src != 0)
    {
        *dest = *src;
        src += 1;
        dest += 1;
    }

    // append null terminator
    *dest = 0x00;

    return res;
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