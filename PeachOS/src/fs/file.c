#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "string/string.h"
#include "disk/disk.h"
#include "status.h"
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
    Purpose: Determine what kind of open we are performing. ex) read, write, append.
    Parameter str: The char passed to determine the mode.
    Return: the mode. Invalid if not a mode.
*/
FILE_MODE file_get_mode_by_string(const char* str)
{
    FILE_MODE mode = FILE_MODE_INVALID;
    if(strncmp(str, "r", 1) == 0)
    {
        mode = FILE_MODE_READ;
    }
    else if(strncmp(str, "w", 1) == 0)
    {
        mode = FILE_MODE_WRITE;
    }
    else if(strncmp(str, "a", 1) == 0)
    {
        mode = FILE_MODE_APPEND;
    }

    return mode;
}

/*
    Purpose: Open a file. VFS implementation.
    Parameter filename: The file path to look for.
    Parameter mode_str: How the file should be opened.
    Return: Result - the index. Null if fails.
*/
int fopen(const char* filename, const char* mode_str)
{
    int res = 0;
    // Parse path and return path root
    struct path_root* root_path = pathparser_parse(filename, NULL);
    if(!root_path)
    {
        res = -EINVARG;
        goto out;
    }
    
    // check if we opened just the root with not path after
    if(!root_path->first)
    {
        res = -EINVARG;
        goto out;
    } 

    // Get drive number
    struct disk* disk = disk_get(root_path->drive_no);
    if(!disk)
    {
        res = -EIO;
        goto out;
    }

    // Check if no filesystem associated with disk (ex - FAT16)
    if(!disk->filesystem)
    {
        res = -EIO;
        goto out;
    }

    FILE_MODE mode = file_get_mode_by_string(mode_str);
    // If r, w, a not passed
    if(mode == FILE_MODE_INVALID)
    {
        res = -EINVARG;
        goto out;
    }

    // filesystem->open is our function pointer to fat16_open
    // disk with drive number of 0 points to fat16 filesystem
    void* descriptor_private_data = disk->filesystem->open(disk, root_path->first, mode);
    if(ISERR(descriptor_private_data))
    {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor* desc = 0;
    res = file_new_descriptor(&desc);
    if(res < 0)
    {
        goto out;
    }
    // set filesystem we just created to filesystem we opened the file from
    desc->filesystem = disk->filesystem;
    desc->private = descriptor_private_data;
    desc->disk = disk;
    res = desc->index;

out:
    // fopen shouldn't return negative values.
    if(res < 0)
        res = 0;

    return res;
}

/*
    Purpose: Seek to any point in a file. Changes where file pointer is in that file.
    Parameter fd: Struct that holds the file item and position.
    Parameter offset: Where to start seeking from.
    Parameter whence:
    Return:
*/
int fseek(int fd, int offset, FILE_SEEK_MODE whence)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if(!desc)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->seek(desc->private, offset, whence);
out:
    return res;
}

/*
    Purpose: 
    Parameter ptr:
    Parameter size:
    Parameter nmemb:
    Parameter fd:
    Return:
*/
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd)
{
    int res = 0;
    if(size == 0 || nmemb == 0 || fd < 1)
    {
        res = -EINVARG;
        goto out;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if(!desc)
    {
        res = -EINVARG;
        goto out;
    }

    // pass private data to lower filesystem. Should have all information to resolve that file.
    res = desc->filesystem->read(desc->disk, desc->private, size, nmemb, (char*) ptr);

out:
    return res;
}