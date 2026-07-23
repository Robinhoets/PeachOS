#ifndef DISK_H
#define DISK_H

#include "fs/file.h"

typedef unsigned int PEACHOS_DISK_TYPE;

// Represents a real physical hard disk
#define PEACHOS_DISK_TYPE_REAL 0

struct disk
{
    PEACHOS_DISK_TYPE type;
    int sector_size;

    // The id of the disk
    int id;

    struct filesystem* filesystem;

    // private data of the filesystem
    // What kind of filesystem? fat16? fat16, or any other, will have a specific signature that can be 'AND-ED' together with
    // a FAT_FILE_SUBDIRECTORY for example to determine if it is indeed a fat16 type system
    void* fs_private;
};

void disk_search_and_init();
struct disk* disk_get(int index);
int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf);

#endif