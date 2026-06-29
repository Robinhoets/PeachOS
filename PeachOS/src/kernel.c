#include "kernel.h"
#include <stddef.h>
#include <stdint.h>

uint16_t* video_mem = 0; // point to this address in memory

uint16_t terminal_make_char(char c, char color)
{
    return (color << 8) | c;
}

void terminal_put_char(int x, int y, char c, char color)
{
    video_mem[(y*VGA_WIDTH) + x] = terminal_make_char(c, color);
}

// Loop through terminal and clear it
void terminal_initialize()
{
    video_mem = (uint16_t*)(0xB8000);
    for(int y=0; y<VGA_HEIGHT; y++)
    {
        for(int x=0; x<VGA_WIDTH; x++)
        {
            /*
                Convert x & y into an index for one dimensional arrayf
                Input space - black
            */ 
            terminal_put_char(x, y, ' ', 0);
        }
    }
}

/*
    Count the length of a string
*/
size_t strlen(const char* str)
{
    size_t len = 0;
    while(str[len])
    {
        len++;
    }
    return len;
}

void kernel_main()
{
    terminal_initialize();
    video_mem[0] = terminal_make_char('B',15);
}