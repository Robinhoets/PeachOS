#include "kernel.h"

uint16_t* video_mem = 0; // point to this address in memory

uint16_t terminal_make_char(char c, char color)
{
    return (color << 8) | c;
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
            video_mem[(y*VGA_WIDTH) + x] =  terminal_make_char(' ', 0);
        }
    }
}



void kernel_main()
{
    terminal_initialize();
    video_mem[0] = terminal_make_char('B',15);
}