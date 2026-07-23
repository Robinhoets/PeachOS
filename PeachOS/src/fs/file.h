#ifndef FILE_H
#define FILE_H

#include "pparser.h"
#include <stdint.h>

// Seeking through a file
typedef unsigned int FILE_SEEK_MODE;
enum
{
    SEEK_SET,
    SEEK_CUR,
    SEEK_END
}; 

typedef unsigned int FILE_MODE;
enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

struct disk;
// function pointer
typedef void*(*FS_OPEN_FUNCTION)(struct disk* disk, struct path_part* path, FILE_MODE mode);
// function pointer for read
typedef int (*FS_READ_FUNCTION)(struct disk* disk, void* private, uint32_t size, uint32_t nmemb, char* out);
// function pointer for resolve pointer - is this valid disk
typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk);

struct filesystem
{
    // Filesystem should return zero from resolve if the provided disk is using its filesystem.
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;

    char name[20];
};

struct file_descriptor
{
    // The descriptor index
    int index;
    struct filesystem* filesystem;

    // Private data for internal file descriptor
    void* private;

    // The disk that the file descriptor should be used on
    struct disk* disk;
};

void fs_init();
int fopen(const char* filename, const char* mode_str);
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd);
void f_insert_filesystem(struct filesystem* filesystem);
struct filesystem* fs_resolve(struct disk* disk);

#endif