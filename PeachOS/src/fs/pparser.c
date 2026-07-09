/*******************************************************************************
 * @file: pparser.c
 * @brief: Parse the file path.
 * @author: Robert Smith
 * @date: July 2026
 ******************************************************************************/
#include "pparser.h"
#include "kernel.h"
#include "string/string.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "status.h"

/*
    Purpose: Ensure path is a valid format
    Parameter filename: A path.
    Return:
    Valid path if:  (1) Length is equal or above 3
                    (2) It is a digit.
                    (3) Follows correct format (":/")
*/
static int pathparser_path_valid_format(const char* filename)
{
    int len = strnlen(filename, PEACHOS_MAX_PATH);
    return (len >= 3 && isdigit(filename[0]) && memcmp((void*)&filename[1], ":/", 2) == 0);
}

/*
    Purpose: Get drive number.
    Parameter path: Path to be parsed
    Return: Drive number.
*/
static int pathparser_get_drive_by_path(const char** path)
{
    if(!pathparser_path_valid_format(*path))
    {
        return -EBADPATH;
    }
    // get first byte and convert to numeric digit.
    int drive_no = tonumericdigit(*path[0]);

    // add 3 bytes to skip drive number (0:/ 1:/ 2:/)
    // function caller's path gets incremented.
    *path += 3;

    return drive_no;
}

/*
    Purpose: Create path root.
    Parameter drive_numbner: Drive number where root to be created from.
    Return: Pointer to that path (stucture).
*/
static struct path_root* pathparser_create_root(int drive_number)
{
    struct path_root* path_r = kzalloc(sizeof(struct path_root));
    path_r->drive_no = drive_number;
    path_r->first = 0;
    return path_r;
}

/*
    Purpose: Get path part (after the drive number).
    Paramter path: Full path to be extracted from.
    Return: Part of the path extracted.
*/
static const char* pathparser_get_path_part(const char** path)
{
    char* result_path_part = kzalloc(PEACHOS_MAX_PATH);
    int i = 0;
    // set result as long we don't hit a sub directory or end
    while(**path != '/' && **path != 0x00)
    {
        result_path_part[i] = **path;
        // changing caller's pointer
        *path +=1 ;
        i++;
    }

    if(**path == '/')
    {
        // Skip the forward slash to avoid problems
        *path += 1;
    }

    // We didn't actuall parse anything
    if(i == 0)
    {
        kfree(result_path_part);
        result_path_part = 0;
    }

    return result_path_part;
}

/*
    Purpose: Create path part structure elements.
    Parameter last: Last part of the path to be parsed.
    Paramter path: Full path the extract part from.
    Return: The part of the path extracted.
*/
struct path_part* pathparser_parse_path_part(struct path_part* last_part, const char** path)
{
    const char* path_part_str = pathparser_get_path_part(path);
    if(!path_part_str)
    {
        return 0;
    }
    struct path_part* part = kzalloc(sizeof(struct path_part));
    part->part = path_part_str;

    if(last_part)
    {
        last_part->next = part;
    }

    return part;
}

/*
    Purpose: Free memory.
    Parameter root: Root location to start freeing memory.
    Return: void
    * Note * created memory in pathparser_create_root() with kzalloc()
*/
void pathparser_free(struct path_root* root)
{
    struct path_part* part = root->first;
    while(part)
    {
        struct path_part* next_part = part->next;
        kfree((void*) part->part);
        kfree(part);
        part = next_part;
    }

    kfree(root);
}

/*
    Purpose: Public function
    Parameter path: Full path, including drive, to parse.
    Parameter current_directory_path: *** not currently used ***
    Return: Path root.
*/
struct path_root* pathparser_parse(const char* path, const char* current_directory_path)
{
    int res = 0;
    // Don't change path passed to us. We change the pointer in other functions. Keep original.
    const char* tmp_path = path;
    struct path_root* path_root = 0;

    if(strlen(path) > PEACHOS_MAX_PATH)
    {
        goto out;
    }

    // get drive number. If negative, error
    res = pathparser_get_drive_by_path(&tmp_path);
    if(res < 0)
    {
        goto out;
    }

    // get path root
    path_root = pathparser_create_root(res);
    if(!path_root)
    {
        goto out;
    }

    struct path_part* first_part = pathparser_parse_path_part(NULL, &tmp_path);
    if(!first_part)
    {
        goto out;
    }

    path_root->first = first_part;
    struct path_part* part = pathparser_parse_path_part(first_part, &tmp_path);
    while(part)
    {
        part = pathparser_parse_path_part(part, &tmp_path);
    }

out:
    return path_root;
}