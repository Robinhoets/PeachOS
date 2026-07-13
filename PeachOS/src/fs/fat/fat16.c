#include "fat16.h"
#include "string/string.h"
#include "status.h"

int fat16_resolve(struct disk* disk);
void * fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);

struct filesystem fat16_fs = 
{
    // storing the address of the function
    .resolve = fat16_resolve,
    .open = fat16_open
};

/*
    Purpose: Initialize the fat16 filesystem.
    Return: Address of fat16_fs
*/
struct filesystem* fat16_init()
{
    // accessing file.h struct name to set the kind of file system.
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

/*
    Purpose: Resolve function
    Parameter disk:
*/
int fat16_resolve(struct disk* disk)
{
    return 0;
}

/*
    Purpose: Open a file.
    Parameter disk:
    Parameter path:
    Parameter mode:
    Return: 0 if failed to open file.
*/
void * fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode)
{
    return 0;
}