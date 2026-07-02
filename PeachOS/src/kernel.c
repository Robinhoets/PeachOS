#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"

uint16_t* video_mem = 0; // point to this address in memory
uint16_t terminal_row = 0;
uint16_t terminal_col = 0;

uint16_t terminal_make_char(char c, char color)
{
    return (color << 8) | c;
}

void terminal_put_char(int x, int y, char c, char color)
{
    video_mem[(y*VGA_WIDTH) + x] = terminal_make_char(c, color);
}

/*
    Write's a character in sequence
*/
void terminal_write_char(char c, char color)
{
    if(c == '\n')
    {
        terminal_row += 1;
        terminal_col = 0;
        return;
    }

    terminal_put_char(terminal_col, terminal_row, c, color);
    terminal_col += 1;

    /* If end of row, loop back around*/
    if(terminal_col >= VGA_WIDTH)
    {
        terminal_col = 0;
        terminal_row += 1;
    }
}

// Loop through terminal and clear it
void terminal_initialize()
{
    video_mem = (uint16_t*)(0xB8000);
    terminal_row = 0;
    terminal_col = 0;
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

void print(const char* str)
{
    size_t len = strlen(str);
    for(int i=0; i<len; i++)
    {
        terminal_write_char(str[i], 15);
    }
}

void kernel_main()
{
    terminal_initialize();
    print("Hello \nWorld!");

    // Initialize the heap
    kheap_init();

    // Initialize the interrupt descriptor table
    idt_init();

    void* ptr = kmalloc(50);
    void* ptr2 = kmalloc(5000);
    if(ptr || ptr2){}
    
}