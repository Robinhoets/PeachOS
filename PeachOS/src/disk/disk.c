/*******************************************************************************
 * @file: disk.c
 * @brief: Read memory from the disk.
 * @author: Robert Smith
 * @date: July 2026
 ******************************************************************************/

 #include "io/io.h"                     // Access to outb and inb instructions.
 #include "disk.h"
 #include "memory/memory.h"
 #include "config.h"
 #include "status.h"

struct disk disk;                       // Represents primary hard disk.

 /*
    Purpose:
    Parameter lba: Logical Block Address we want to read from.
    Parameter total: Total number of blocks to read from lba.
    Parameter buf: Points to buffer where we load data to.
*/
int disk_read_sector(int lba, int total, void* buf)
{
    outb(0x1F6, (lba >> 24) | 0xE0);    // Select master drive and pass part of the LBA 
    outb(0x1F2, total);                 // Send the total number of sectors we want to read.
    outb(0x1F3, (unsigned char)(lba & 0xff));   // Send more of LBA
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x20);

    unsigned short* ptr = (unsigned short*) buf;    // reading two bytes at a time
    // Listen & Wait for the buffer to be ready
    for(int b=0; b<total; b++)
    {
        // read on purt 0x1F7
        char c = insb(0x1F7);
        // Check for flag
        while(!(c & 0x08))
        {
            c = insb(0x1F7);
        }

        // Copy from hard disk to memory
        for(int i=0; i<256; i++)
        {
            // Read two bytes and put in buffer
            *ptr = insw(0x1F0);
            ptr++;
        }
    }

    return 0;
}

/*
    Purpose: Search for disks and initializing them.
    *** not searching currently - abstracted out for later implementation ***
*/
void disk_search_and_init()
{
    memset(&disk, 0, sizeof(disk));
    disk.type = PEACHOS_DISK_TYPE_REAL;
    disk.sector_size = PEACHOS_SECTOR_SIZE;
    disk.id = 0;    // better system would search an array for a free index.
    disk.filesystem = fs_resolve(&disk);
}

/*
    Purpose: Get the disk.
    Parameter index: Index of disk to get.
    Return: Address of disk desired.
    *** not implemented to be dynamic. Just an abstract at the moment. ***
*/
struct disk* disk_get(int index)
{
    if(index != 0)
        return 0;

    return &disk;
}

/*
    Purpose: Read the block for the disk.
    Parameter idisk: 
    Parameter lba: Logical Block Address we want to read from.
    Parameter total: Total number of blocks to read from lba.
    Parameter buf: Points to buffer where we load data to.
*/
int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf)
{
    if(idisk != &disk)
    {
        return -EIO;
    }
    return disk_read_sector(lba, total, buf);
}