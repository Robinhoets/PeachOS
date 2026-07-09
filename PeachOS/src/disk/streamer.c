#include "streamer.h"
#include "memory/heap/kheap.h"
#include "config.h"

/*
    Purpose: Set up the positions to read stream from disk.
    Parameter disk_id: id of the disk to start reading from.
    Return: The disk_streamer with positions set.
*/
struct disk_stream* diskstreamer_new(int disk_id)
{
    struct disk* disk = disk_get(disk_id);
    if(!disk)
    {
        return 0;
    }

    struct disk_stream* streamer = kzalloc(sizeof(struct disk_stream));
    streamer->pos = 0;
    streamer->disk = disk;
    return streamer;
}

/*
    Purpose: Set position in the stream to position passed.
    Parameter stream: Our disk to seek in.
    Parameter pos: Position to be set to in stream object.
    Return: 0.
*/
int diskstreamer_seek(struct disk_stream* stream, int pos)
{
    stream->pos = pos;
    return 0;
}

/*
    Purpose: Read the stream.
    Parameter stream: Stream to read from.
    Parameter out: Location to read bytes to.
    Parameter total: Amount of bytes to read.
    Return: 0 if okay, negative value if error.
*/
int diskstreamer_read(struct disk_stream* stream, void* out, int total)
{
    int sector = stream->pos / PEACHOS_SECTOR_SIZE;
    int offset = stream->pos % PEACHOS_SECTOR_SIZE;
    char buf[PEACHOS_SECTOR_SIZE];

    // read one sector into buf
    int res = disk_read_block(stream->disk, sector, 1, buf);
    if(res < 0)
    {
        goto out;
    }

    // if total is more than a sector, then use sector size
    int total_to_read = total > PEACHOS_SECTOR_SIZE ? PEACHOS_SECTOR_SIZE : total;
    for(int i=0; i<total_to_read; i++)
    {
        // cast out to char pointer
        *(char*)out++ = buf[offset+i];
    }

    // If they ask to read more than a sector - Adjust the stream
    stream->pos += total_to_read;
    if(total > PEACHOS_SECTOR_SIZE)
    {
        // Recurse so we don't overflow. Out points to next sector and start over.
        res = diskstreamer_read(stream, out, total - PEACHOS_SECTOR_SIZE);
    }

out:
    return res;
}

/*
    Purpose: Close/free the stream.
    Parameter stream: Stream to be closed/freed.
    Return: void.
*/
void diskstreamer_close(struct disk_stream* stream)
{
    kfree(stream);
}