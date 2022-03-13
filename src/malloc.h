#ifndef MALLOC_H
#define MALLOC_H

#include <err.h>
#include <stddef.h>
#include <stdint.h>

#define YES 1
#define NO 0

#define PAGE_SIZE 4096
#define MAX_FLAGS PAGE_SIZE / sizeof(long double)

struct free_list
{
    int free;
};

/*
** List of addresses of different buckets (pages split in different sizes)
** available. This list is not visible by user.
** Each list element has a pointer to a bucket, a pointer to the next bucket,
** another one to a bucket with same block size, the size of each block,
** a free list represented as bytes (1 for a free block and 0 for a used one),
** and a list to help know if blocks are merged (from realloc).
** The free list is in reality composed of multiple lists in order to have
** enough bits to represent a page.
*/
struct bucket_meta
{
    void *bucket;
    struct bucket_meta *next;
    struct bucket_meta *last_created;
    struct bucket_meta *next_sibling;
    size_t block_size;
    size_t page_size;
    struct free_list free_list[MAX_FLAGS];
};

#endif /* !MALLOC_H */
