#include "fat16.h"
#include "string/string.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"
#include "kernel.h"
#include <stdint.h>

#define PEACHOS_FAT16_SIGNATURE         0x29
#define PEACHOS_FAT16_FAT_ENTRY_SIZE    0x02
#define PEACHOS_FAT16_BAD_SECTOR        0xFF7
#define PEACHOS_FAT16_UNUSED            0x00

// Internal use while loading into memory
typedef unsigned int FAT_ITEM_TYPE;
#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1

// Fat directory entry attributes bitmask
#define FAT_FILE_READ_ONLY              0x01
#define FAT_FILE_HIDDEN                 0x02
#define FAT_FILE_SYSTEM                 0x04
#define FAT_FILE_VOLUME_LABEL           0x08
#define FAT_FILE_SUBDIRECTORY           0x10
#define FAT_FILE_ARCHIVED               0x20
#define FAT_FILE_DEVICE                 0x40
#define FAT_FILE_RESERVED               0x80

// Packed to ensure compiler keeps this structure.
// Writing to and reading from disk.
struct fat_header_extended
{
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t system_id_string[8];
} __attribute__((packed));

/*
    bytes_per_sector: ignored
    reserved_sectors: how many sectors reserved before file allocation table
*/
struct fat_header
{
    uint8_t short_jmp_ins[3];   
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector; 
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors; 
    uint8_t fat_copies;
    uint16_t root_dir_entries;
    uint16_t number_of_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t sectors_big;

} __attribute__((packed));

struct fat_h
{
    struct fat_header primary_header;
    union fat_h_e
    {
        struct fat_header_extended extended_header;
    } shared;
};

/*
    Represents either a file or subdirectory.
    attribute:  represent Fat directory entry attributes bitmask.
                do bitwise op to see if flags are set - tells us about directory.
*/
struct fat_directory_item
{
    uint8_t filename[8];
    uint8_t ext[3];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t creation_time_tenths_of_a_sec;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access;
    uint16_t high_16_bits_first_cluster;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t low_16_bits_first_cluster;
    uint32_t filesize;
} __attribute__((packed));

/*
    Represents only a directory.
    item: points to first item.
    sector_pos: first sector where data is.
*/
struct fat_directory
{
    struct fat_directory_item* item;
    int total;
    int sector_pos;
    int ending_sector_pos;
};

/*
    Represents either a normal file or a directory.
*/
struct fat_item
{
    union 
    {
        struct fat_directory_item* item;
        struct fat_directory* directory;
    };
    FAT_ITEM_TYPE type;
};

/*

*/
struct fat_file_descriptor
{
    struct fat_item* item;
    uint32_t pos;
};

/*

*/
struct fat_private
{
    struct fat_h header;
    struct fat_directory root_directory;

    // Used to stream data clusters
    struct disk_stream* cluster_read_stream;
    // Used to stream th efile allocation table
    struct disk_stream* fat_read_stream;

    // Used in situations where we stream the directory
    struct disk_stream* directory_stream;
};

/*
    FAT16 implemetations of function pointers.
*/
int fat16_resolve(struct disk* disk);
void * fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);
int fat16_read(struct disk* disk, void* descriptor, uint32_t size, uint32_t nmemb, char* out_ptr);
int fat16_seek(void* private, uint32_t offset, FILE_SEEK_MODE seek_mode);

struct filesystem fat16_fs = 
{
    // storing the address of the function
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read,
    .seek = fat16_seek
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
    Purpose: Allocate memory size of fat_private structure
    Parameter disk: 
    Parameter private: 
    Return: void
*/
static void fat16_init_private(struct disk* disk, struct fat_private* private)
{
    memset(private, 0, sizeof(struct fat_private));
    private->cluster_read_stream = diskstreamer_new((disk->id));
    private->fat_read_stream = diskstreamer_new(disk->id);
    private->directory_stream = diskstreamer_new(disk->id);
}

/*
    Purpose: Physical location of the sector in memory.
    Parameter disk: what disk the sector is in
    Parameter sector: the sector to locate
    Return: Location in memory of sector.
*/
int fat16_sector_to_absolute(struct disk* disk, int sector)
{
    return sector * disk->sector_size;
}

/*
    Purpose:
    Parameter disk:
    Parameter directory_start_sector: 
    Return: 
*/
int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector)
{
    // Create empty item and initialize
    struct fat_directory_item item;
    struct fat_directory_item empty_item;
    memset(&empty_item, 0, sizeof(empty_item));

    struct fat_private* fat_private = disk->fs_private;
    int res = 0;
    int i = 0;
    // seek to directory start sector
    int directory_start_pos = directory_start_sector * disk->sector_size;
    struct disk_stream* stream = fat_private->directory_stream;
    if(diskstreamer_seek(stream, directory_start_pos) != PEACHOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }
    // Then loop through directory and increment item each time
    while(1)
    {
        if(diskstreamer_read(stream, &item, sizeof(item)) != PEACHOS_ALL_OK)
        {
            res = -EIO;
            goto out;
        }

        if(item.filename[0] == 0x00)
        {
            break;
        }

        // If item is unused.
        if(item.filename[0] == 0xE5)
        {
            continue;
        }

        i++;
    }

    res = i;

out:
    return res;
}

/*
    Purpose: Get root directory.
    Parameter disk:
    Parameter fat_private:
    Parameter directory: Out directory - where we load the directory to.
*/
int fat16_get_root_directory(struct disk* disk, struct fat_private* fat_private, struct fat_directory* directory)
{
    int res = 0;
    struct fat_header* primary_header = &fat_private->header.primary_header;
    // calculate root directory position. First sector.
    int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sectors;
    int root_dir_entries = fat_private->header.primary_header.root_dir_entries;
    int root_dir_size = (root_dir_entries * sizeof(struct fat_directory_item));
    int total_sectors = root_dir_size / disk->sector_size;
    // 600 % 512 = 88 so two sectors. But 600/512 = 1. So 1+1=2.
    if(root_dir_size % disk->sector_size)
    {
        total_sectors += 1;
    }

    int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);

    struct fat_directory_item* dir = kzalloc(root_dir_size);
    if(!dir)
    {
        res  = -ENOMEM;
        goto out;
    }

    struct disk_stream* stream = fat_private->directory_stream;
    if (diskstreamer_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != PEACHOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    if (diskstreamer_read(stream, dir, root_dir_size) != PEACHOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    directory->item = dir;
    directory->total = total_items;
    directory->sector_pos = root_dir_sector_pos;
    directory->ending_sector_pos = root_dir_sector_pos + (root_dir_size / disk->sector_size);

out:
    return res;
}

/*
    Purpose: Resolve function
    Parameter disk:
*/
int fat16_resolve(struct disk* disk)
{
    int res = 0;
    // private data to bind to disk so that when called can know what is going on with disk
    // store temporary information
    struct fat_private* fat_private = kzalloc(sizeof(struct fat_private));
    fat16_init_private(disk, fat_private);

    disk->fs_private = fat_private;
    disk->filesystem = & fat16_fs;

    struct disk_stream* stream = diskstreamer_new(disk->id);
    if(!stream)
    {
        res = -ENOMEM;
        goto out;
    }

    if(diskstreamer_read(stream, &fat_private->header, sizeof(fat_private->header)) != PEACHOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    // check if fat16 signature is present
    // 0x29 means valid fat16 signature
    if(fat_private->header.shared.extended_header.signature != 0x29)
    {
        res = -EFSNOTUS;
        goto out;
    }

    // if fail to load root directory
    if(fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != PEACHOS_ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    

out:
    if(stream)
    {
        diskstreamer_close(stream);
    }

    if(res < 0)
    {
        kfree(fat_private);
        disk->fs_private = 0;
    }

    return res;
}

/*
    Purpose: Remove spaces and add null terminator if spaces in filename.
             (ex) FILE    ... 4 spaces after file turns into FILE. 0 spaces after file.
    Parameter out: End of string where null terminator is or is set.
    Parameter in: Beginning of string to search for spaces.
    Return: void. Sets the end of a string to a null terminator.
*/
void fat16_to_proper_string(char** out, const char* in)
{
    // Go through string. Look for null space or terminator
    // When its found a space, exit
    while(*in != 0x00 && *in != 0x20)
    {
        **out = *in;
        *out += 1;
        in +=1;
    }

    // add null terminator if found space
    if(*in == 0x20)
    {
        **out = 0x00;
    }
}

/*
    Purpose: Create the filename by joining fat directory item and extension.
             (ex) TEST EXT ABC -> TEST.ABC
    Parameter item: Item in directory to be converted.
    Parameter out: Location of result.
    Parameter max_len: Maximum length the filename could be.
    Return: Void. Sets out to result.
*/
void fat16_get_full_relative_filename(struct fat_directory_item* item, char* out, int max_len)
{
    memset(out, 0x00, max_len);
    char *out_tmp = out;
    fat16_to_proper_string(&out_tmp, (const char*) item->filename);
    if(item->ext[0] != 0x00 && item->ext[0] != 0x20)
    {
        *out_tmp++ = '.';
        fat16_to_proper_string(&out_tmp, (const char*) item->ext);
    }
}

/*
    Purpose: To clone our directory so it can be worked on without memory issues.
    Parameter item: Item to be copied.
    Parameter size: Size to set memory
    Return: item_copy, the copy of our fat directory item.
*/
struct fat_directory_item* fat16_clone_directory_item(struct fat_directory_item* item, int size)
{
    struct fat_directory_item*  item_copy = 0;
    if(size < sizeof(struct fat_directory_item))
    {
        return 0;
    }

    item_copy = kzalloc(size);
    if(!item_copy)
    {
        return 0;
    }

    memcpy(item_copy, item, size);
    return item_copy;
}

/*
    Purpose: Get the sector number of the cluster 
    Parameter private:
    Parameter cluster: 
    Return: 
*/
static int fat16_cluster_to_sector(struct fat_private* private, int cluster)
{
    // ending sector position
    // add cluster number minus two
    // times sectors per cluster
     return private->root_directory.ending_sector_pos + ((cluster - 2) * private->header.primary_header.sectors_per_cluster);
}

/*
    Purpose: Get the first cluster by joining high 16 bits and low 16 bits.
    Parameter item: fat directory item to join high and low bits.
    Return: The joined bits.
*/
static uint32_t fat16_get_first_cluster(struct fat_directory_item* item)
{
    return (item->high_16_bits_first_cluster) | item->low_16_bits_first_cluster;
}

/*
    Purpose:
    Parameter:
    Parameter:
    Return:
*/
static uint32_t fat16_get_first_fat_sector(struct fat_private* private)
{
    return private->header.primary_header.reserved_sectors;
}

/*
    Purpose:
    Parameter :
    Parameter:
    Return: 
*/
static int fat16_get_fat_entry(struct disk* disk, int cluster)
{
    int res = -1;
    struct fat_private* private = disk->fs_private;
    struct disk_stream* stream = private->fat_read_stream;
    if(!stream)
    {
        goto out;
    }

    // go to first fat table
    uint32_t fat_table_position = fat16_get_first_fat_sector(private) * disk->sector_size;
    res = diskstreamer_seek(stream, fat_table_position * (cluster * PEACHOS_FAT16_FAT_ENTRY_SIZE));
    if(res < 0)
    {
        goto out;
    }
    uint16_t result = 0;
    res = diskstreamer_read(stream, &result, sizeof(result));
    if(res < 0)
    {
        goto out;
    }

    res = result;

out:
    return res;
}

/*
    Purppose: Get the correct cluster to use based on the starting cluster and the offset.
    Parameter disk:
    Parameter starting_cluster:
    Parameter offset:
    Return:
*/
static int fat16_get_cluster_for_offset(struct disk* disk, int starting_cluster, int offset)
{
    int res = 0;
    struct fat_private* private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = starting_cluster;
    int clusters_ahead = offset / size_of_cluster_bytes;
    for(int i=0; i<clusters_ahead; i++)
    {
        /*
            Get fat entry from file allocation table.
            Use starting cluster.
            Results in either next entry in the file or error.
        */
        int entry = fat16_get_fat_entry(disk, cluster_to_use);;
        if(entry == 0xFF8 || entry == 0xFFF)
        {
            // We are at the last entry in the file
            res = -EIO;
            goto out;
        }

        // Check if sector marked as bad. Such as corrupted sector we can't read from
        if(entry == PEACHOS_FAT16_BAD_SECTOR)
        {
            res = -EIO;
            goto out;
        }

        // Check if reserved sectors
        if(entry == 0xFF0 || entry == 0xFF6)
        {
            res = -EIO;
            goto out;
        }

        if(entry == 0x00)
        {
            res = -EIO;
            goto out;
        }

        cluster_to_use = entry;
    }

    res = cluster_to_use;

out:
    return res;
}

/*
    Purpose:
    Parameter disk:
    Parameter stream:
    Parameter cluster:
    Parameter offset:
    Parameter total:
    Parameter out:
    Return:
*/
static int fat16_read_internal_from_stream(struct disk* disk, struct disk_stream* stream, int cluster, int offset, int total, void* out)
{
    int res = 0;
    struct fat_private* private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = fat16_get_cluster_for_offset(disk, cluster, offset);
    if(cluster_to_use   < 0)
    {
        res = cluster_to_use;
        goto out;
    }

    int offset_from_cluster = offset % size_of_cluster_bytes;

    int starting_sector = fat16_cluster_to_sector(private, cluster_to_use);
    int starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster;
    int total_to_read = total > size_of_cluster_bytes ? size_of_cluster_bytes : total;
    res = diskstreamer_seek(stream, starting_pos);
    if(res != PEACHOS_ALL_OK)
    {
        goto out;
    }

    res = diskstreamer_read(stream,out, total_to_read);
    if(res!= PEACHOS_ALL_OK)
    {
        goto out;
    }

    // Recurse until read total amount
    total -= total_to_read;
    if(total > 0)
    {
        // We still have more to read
        res = fat16_read_internal_from_stream(disk, stream, cluster, offset + total_to_read, total, out + total_to_read);
    }

out:    
    return res;
}

/*
    Purpose: Take starting cluster and read total amount of bytes. If it exceeds the cluster size, look in file allocation table for next cluster
             and read from that one.
    Parameter disk:
    Parameter starting_cluster:
    Parameter offset:
    Parameter total:
    Parameter out:
    Return:
*/
static int fat16_read_internal(struct disk* disk, int starting_cluster, int offset, int total, void* out)
{
    struct fat_private* fs_private = disk->fs_private;
    struct disk_stream* stream = fs_private->cluster_read_stream;
    return fat16_read_internal_from_stream(disk, stream, starting_cluster, offset, total, out);
}

/*
    Purpose:
    Parameter:
    Return:
*/
void fat16_free_directory(struct fat_directory* directory)
{
    if(!directory)
    {
        return;
    }

    if(directory->item)
    {
        kfree(directory->item);
    }

    kfree(directory);
}

/*
    Purpose:
    Parameter:
    Parameter:
    Return:
*/
void fat16_fat_item_free(struct fat_item* item)
{
    if(item->type == FAT_ITEM_TYPE_DIRECTORY)
    {
        fat16_free_directory(item->directory);
    }
    else if(item->type == FAT_ITEM_TYPE_FILE)
    {
        kfree(item->item);
    }

    kfree(item);
}

/*
    Purpose: Create memory and load directory from disk.
    Parameter disk: disk directory located on.
    Parameter item: 
    Return: The directory
*/
struct fat_directory* fat16_load_fat_directory(struct disk* disk, struct fat_directory_item* item)
{
    int res = 0;
    struct fat_directory* directory = 0;
    struct fat_private* fat_private = disk->fs_private;
    // make sure it is a directory and not a file
    if(!(item->attribute & FAT_FILE_SUBDIRECTORY))
    {
        res = -EINVARG;
        goto out;
    }

    directory = kzalloc(sizeof(struct fat_directory));
    if(!directory)
    {
        res = -ENOMEM;
        goto out;
    }

    // calculate the cluster - where the directory contents are - essentially the directory filenames (more fat items)
    // the cluster where the directory items are listed
    int cluster = fat16_get_first_cluster(item);
    // get sector number of cluster
    int cluster_sector = fat16_cluster_to_sector(fat_private, cluster);
    int total_items = fat16_get_total_items_for_directory(disk, cluster_sector);
    directory->total = total_items;
    // since directory is array of items, total * size of fat directory item gets total size
    int directory_size = directory->total * sizeof(struct fat_directory_item);
    // make room in memory to load entire directory into memory
    directory->item = kzalloc(directory_size);
    if(!directory->item)
    {
        res = -ENOMEM;
        goto out;
    }

    // read from the disk
    res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->item);
    if(res != PEACHOS_ALL_OK)
    {
        goto out;
    }

out:
    return directory;
}

/*
    Purpose: Create a new fat item, with new memory space, so memory issues with old don't happen.
    Parameter disk: Which disk the fat item is on.
    Parameter item: Item to create.
    Return: fat_item clone.
*/
struct fat_item* fat16_new_fat_item_for_directory_item(struct disk* disk, struct fat_directory_item* item)
{
    struct fat_item* f_item = kzalloc(sizeof(struct fat_item));
    if(!f_item)
    {
        return 0;
    }

    // Does the fat item attribute have the subdirectory flag
    if(item->attribute & FAT_FILE_SUBDIRECTORY)
    {
        f_item->directory = fat16_load_fat_directory(disk, item);
        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
    }

    // if not directory, it's a file
    f_item->type = FAT_ITEM_TYPE_FILE;
    // We cannot rely on the item passed so we clone.
    // Memory passed could have been deleted or freed which would cause serious issues.
    f_item->item = fat16_clone_directory_item(item, sizeof(struct fat_directory_item));
    return f_item;
}

/*
    Purpose:
    Parameter disk:
    Parameter directory:
    Parameter name: First path part (ex 0:/abc/test.txt -> abc is path part)
    Return: 
*/
struct fat_item* fat16_find_item_in_directory(struct disk* disk, struct fat_directory* directory, const char* name)
{
    struct fat_item* f_item = 0;
    char tmp_filename[PEACHOS_MAX_PATH];
    for(int i=0; i<directory->total; i++)
    {
        fat16_get_full_relative_filename(&directory->item[i], tmp_filename, sizeof(tmp_filename));
        // if we have found a matching filename
        if(istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0)
        {
            // Create a new fat item
            f_item = fat16_new_fat_item_for_directory_item(disk, &directory->item[i]);
        }
    }

    return f_item;
}

/*
    Purpose: Get the address of a directory it  
    Parameter disk: disk to look on
    Parameter path: path to look on
    Return: Returns the current item in that directory.
*/
struct fat_item* fat16_get_directory_entry(struct disk* disk, struct path_part* path)
{
    struct fat_private* fat_private = disk->fs_private;
    struct fat_item* current_item = 0;
    // Look for item
    // ex) 0:/abc/test.txt - root item is abc
    // ex) 0:/test.txt - root item is test.txt    
    struct fat_item* root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part);
    if(!root_item)
    {
        goto out;
    }

    struct path_part* next_part = path->next;
    current_item = root_item;
    while(next_part != 0)
    {
        if(current_item->type != FAT_ITEM_TYPE_DIRECTORY)
        {
            current_item = 0;
            break;
        }

        struct fat_item* tmp_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->part);
        fat16_fat_item_free(current_item);
        current_item = tmp_item;
        next_part = next_part->next;
    }

out:
    return current_item;
}

/*
    Purpose: Open a file. Locate the correct file
    Parameter disk:
    Parameter path: Path in which we want to open.
    Parameter mode: Check if the mode is "r".
    Return: Return descriptor.
*/
void * fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode)
{
    if( mode != FILE_MODE_READ)

    {
        return ERROR(-ERDONLY);
    }

    struct fat_file_descriptor* descriptor = 0;
    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if(!descriptor)
    {
        return ERROR(-ENOMEM);
    }

    // See if we can locate file with given path.
    // Describes either directory or normal file
    descriptor->item = fat16_get_directory_entry(disk, path);
    if(!descriptor->item)
    {
        return ERROR(-EIO);
    }

    descriptor->pos = 0;
    // Returns descriptor private data to file.c (fopen)
    return descriptor;
}

/*
    Purpose: FAT16 implementation of read. Function pointer points here.
    Parameter descriptor: Holds the fat_item and position. Extract item from this struct.
    Parameter size: Size of each member.
    Parameter nmemb: Number of members we want to get. Use to loop starting at fat_item.pos.
    Parameter out_ptr:
    Return: Number of nmemb successfully read.
*/
int fat16_read(struct disk* disk, void* descriptor, uint32_t size, uint32_t nmemb, char* out_ptr)
{
    int res = 0;
    struct fat_file_descriptor* fat_desc = descriptor;
    struct fat_directory_item* item = fat_desc->item->item;
    int offset = fat_desc->pos;
    for(uint32_t i=0; i<nmemb; i++)
    {
        res = fat16_read_internal(disk, fat16_get_first_cluster(item), offset, size, out_ptr);
        if(ISERR(res))
        {
            goto out;
        }

        out_ptr += size;
        offset += size;
    }

    res = nmemb;
out:
    return res;
}

/*
    Purpose: The FAT16 implemetation of seek.
    Parameter private: Descriptor data (not fat private data).
    Parameter offset:
    Parameter seek_mode:
    Return:
*/
int fat16_seek(void* private, uint32_t offset, FILE_SEEK_MODE seek_mode)
{
    int res = 0;
    struct fat_file_descriptor* desc = private;
    struct fat_item* desc_item = desc->item;
    // make sure file, not a directory
    if(desc_item->type != FAT_ITEM_TYPE_FILE)
    {
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_item* ritem = desc_item->item;
    if(offset >= ritem->filesize)
    {
        res = -EIO;
        goto out;
    }

    switch(seek_mode)
    {
        case SEEK_SET:
            desc->pos = offset;
        break;

        case SEEK_END:
            res = -EUNIMP;
            break;

        case SEEK_CUR:
            desc->pos = offset;
            break;

        default:
            res = EINVARG;
            break;
    }

out:    
    return res;
}