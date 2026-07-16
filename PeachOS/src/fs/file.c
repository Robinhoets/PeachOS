#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "fat/fat16.h"
#include "kernel.h"

struct filesystem* filesystems[PEACHOS_MAX_FILESYSTEMS];
struct file_descriptor* file_descriptors[PEACHOS_MAX_FILE_DESCRIPTORS];

/*
    Purpose:
    Return: Address of empty slot in array. 0 if no free filesystem.
*/
static struct filesystem** fs_get_free_filesystem()
{
    int i=0;
    for(i=0; i<PEACHOS_MAX_FILESYSTEMS; i++)
    {
        if(filesystems[i] == 0)
        {
            return &filesystems[i];
        }
    }
    
    return 0;
}

/*
    Purpose: To insert a filesystem.
    Parameter filesystem: The filesystem to be inserted. 
    Return: void.
*/
void fs_insert_filesystem(struct filesystem* filesystem)
{
    struct filesystem** fs;
    fs = fs_get_free_filesystem();
    if(!fs)
    {
        print("Problem inserting filesysem");
        while(1){}
    }
    // Return element in array of free filesystem to address (*fs) provided.
    *fs = filesystem;
}

/*
    Purpose: Build filesystems into core kernel. Not dynamically loaded.
*/
static void fs_static_load()
{
    fs_insert_filesystem(fat16_init());
}

/*
    Purpose: Load all available filesystems.
*/
void fs_load()
{
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

/*
    Purpose: Initialize the filesystem.
*/
void fs_init()
{
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

/*
    Purpose: Create file descriptors.
    Parameter desc_out: Location of descriptor in memory. Set to desc if free descriptor.
    Return: -ENOMEM if no discriptors free. 0 if found descriptor.
*/
static int file_new_descriptor(struct file_descriptor** desc_out)
{
    int res = -ENOMEM;
    for(int i=0; i<PEACHOS_MAX_FILE_DESCRIPTORS; i++)
    {
        // if file descriptor is free
        if(file_descriptors[i] == 0)
        {
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));
            // Descriptors start at 1
            desc->index = i + 1;
            file_descriptors[i] = desc;
            // pass descriptor back to caller
            *desc_out = desc;
            res = 0;
            break;
        }
    }

    return res;
}

/*
    Purpose: Get file descriptor based on id.
    Parameter fd:
    Return:
*/
static struct file_descriptor* file_get_descriptor(int fd)
{
    if(fd <= 0 || fd >= PEACHOS_MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }

    // Descriptors start at 1
    int index = fd - 1;
    return file_descriptors[index];
}

/*
    Purpose: Get the memory location of the filesystem.
    Parameter disk: Which disk filesystem on.
    Return: Pointer to filesystem found.
*/
struct filesystem* fs_resolve(struct disk* disk)
{
    struct filesystem* fs = 0;
    // Loop through all filesystems that were loaded using fs_insert_filesystem.
    for(int i=0; i< PEACHOS_MAX_FILESYSTEMS; i++)
    {
        // Ensure filesystem there and not null pointer
        // Then call internal resolve function - fat16 driver.
        if(filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0)
        {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
}

/*
    Purpose: 
    Parameter filename:
    Parameter mode:
    Return
*/
int fopen(const char* filename, const char* mode)
{
    return -EIO;
}