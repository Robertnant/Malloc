#ifndef MALLOC_H
#define MALLOC_H

#define YES 1
#define NO 0

/*
** List of addresses of different buckets (pages split in different sizes)
** available. This list is not visible by user.
** Each list element has a pointer to a bucket, a pointer to the next bucket,
** another one to a bucket with same block size, the size of each block,
** a free list represented as bytes (1 for a free block and 0 for a used one),
** and a list to help know if blocks are merged (from realloc).
*/
struct bucket_meta
{
    void *bucket;
    void *next;
    struct bucket_meta *next_sibling;
    size_t block_size;
    size_t free_list[sysconf(_SC_PAGESIZE) / sizeof(size_t)];
    size_t last_block[sysconf(_SC_PAGESIZE) / sizeof(size_t)];
};

#endif /* !MALLOC_H */
