#include "kernel.h"
#include <stddef.h>
#include <stdint.h>
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "string/string.h"
#include "fs/file.h"
#include "disk/disk.h"
#include "fs/pparser.h"
#include "disk/streamer.h"

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


void print(const char* str)
{
    size_t len = strlen(str);
    for(int i=0; i<len; i++)
    {
        terminal_write_char(str[i], 15);
    }
}

static struct paging_4gb_chunk* kernel_chunk = 0;

void kernel_main()
{
    terminal_initialize();
    print("Hello \nWorld!");

    // Initialize the heap
    kheap_init();

    // Initialize the file systems
    fs_init();

    // Search and initialize the disks
    disk_search_and_init();

    // Initialize the interrupt descriptor table
    idt_init();

    // Setup paging
    // Create entire 4gb, writeable, present and acces for anyone
    // Any ring level can access
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);

    // Switch to kernel paging chunk
    paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));

    // Enable Paging
    enable_paging();

    // Enable the system interrupts
    enable_interrupts();

    int fd = fopen("0:/hello.txt", "r");
    if(fd>0)
    {
        print("\nWe opened hello.txt");
        char buf[14];
        fseek(fd, 2, SEEK_SET);
        fread(buf, 11, 1, fd);
        buf[13] = 0x00;
        print(buf);
    }
    while(1){}
}