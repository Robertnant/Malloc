#define _GNU_SOURCE

#include "malloc.h"

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "tools.h"

// Block allocator.
struct bucket_meta *allocator = NULL;

// Aligns size to make malloc implementation faster.
size_t align(size_t size)
{
    return (size + sizeof(long double) - 1) & ~(sizeof(long double) - 1);
}

/*
** Finds are returns metadata of a given valid block address.
** Returns position of block given in bucket through int pointer.
*/
struct bucket_meta *find_meta(void *ptr, int *pos)
{
    char *start = page_begin(ptr, sysconf(_SC_PAGESIZE));

    struct bucket_meta *curr = allocator;

    while (curr && curr->bucket != start)
    {
        curr = curr->next;
    }

    if (curr && pos)
    {
        char *ptr_cast = ptr;
        *pos = ptr_cast - start;
        *pos = 0 ? 0 : *pos / curr->block_size;
    }

    return curr;
}

/*
** Traverses all bucket metadatas to find bucket with block size matching
** the (aligned) malloc size requested by user.
** The last_group parameter is updated when the last bucket metadata of wanted
** size is found but full. A pointer to the last metadata of the allocator is
** updated as well.
*/
void *find_block(struct bucket_meta *allocator, size_t size,
                 struct bucket_meta **last_group, struct bucket_meta **last)
{
    struct bucket_meta *curr = allocator;

    // Find first block with wanted size.
    while (curr && curr->block_size != size)
    {
        *last = curr;
        curr = curr->next;
    }

    // Return a free block or move to next bucket of same size if current full.
    while (curr)
    {
        int block_pos = mark_block(curr->free_list);

        if (block_pos != -1)
            return get_block(curr->bucket, block_pos, curr->block_size);

        *last_group = curr;

        // Move directly to next block with sibling.
        curr = curr->next_sibling;
    }

    return NULL;
}

/*
** Creates a new bucket and appends metadata to list of metadata given.
** A pointer to a free block is returned.
** If a bucket with same size was previously found (last_group),
** they are linked.
*/
void *requestBlock(struct bucket_meta *meta, struct bucket_meta *last_group,
                   size_t size)
{
    struct bucket_meta *new = meta + sizeof(struct bucket_meta);

    meta->next = new;
    new->next = NULL;
    new->next_sibling = NULL;
    new->block_size = size;

    for (size_t i = 0; i < MAX_FLAGS / sizeof(size_t); i++)
    {
        new->free_list[i] = SIZE_MAX;
        new->last_block[i] = SIZE_MAX;
    }

    // Map page for new bucket.
    new->bucket = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Return a free block to the user.
    int block_pos = mark_block(new->free_list);

    void *block = get_block(new->bucket, block_pos, new->block_size);

    if (last_group)
        last_group->next_sibling = block;

    return block;
}

// Initializes a new bucket allocator.
struct bucket_meta *init_alloc(size_t size)
{
    struct bucket_meta *new =
        mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    new->next = NULL;
    new->next_sibling = NULL;
    new->block_size = size;

    for (size_t i = 0; i < MAX_FLAGS / sizeof(size_t); i++)
    {
        new->free_list[i] = SIZE_MAX;
        new->last_block[i] = SIZE_MAX;
    }

    // Map page for new bucket.
    new->bucket = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    return new;
}

// TODO: Handle case when allocator page is used. At 75% usage of allocator for
// example, remap page to make it bigger.
__attribute__((visibility("default"))) void *malloc(size_t size)
{
    // TODO: check size overflow.

    size = align(size);

    // Create new allocator containing addresses of buckets if not found yet.
    if (allocator == NULL)
    {
        allocator = init_alloc(size);

        // Return a free block to the user.
        int block_pos = mark_block(allocator->free_list);

        return get_block(allocator->bucket, block_pos, allocator->block_size);
    }

    // Find free block or create new bucket (after last one) to get new block.
    struct bucket_meta *last = NULL;
    struct bucket_meta *last_group = NULL;
    void *block = find_block(allocator, size, &last_group, &last);

    return block ? block : requestBlock(last, last_group, size);
}

__attribute__((visibility("default"))) void free(void *ptr)
{
    int pos;
    struct bucket_meta *meta = find_meta(ptr, &pos);

    if (meta)
    {
        // Unmap bucket if all blocks are free.
        size_t count = MAX_FLAGS / sizeof(size_t);

        size_t i = 0;
        while (i < count && FREE(meta->free_list[i]))
        {
            i++;
        }

        if (i == count)
        {
            munmap(meta->bucket, sysconf(_SC_PAGESIZE));

            // Unlink unmapped bucket meta from metadata list.
            if (meta != allocator)
            {
                struct bucket_meta *prev = meta - sizeof(struct bucket_meta);
                prev->next = meta->next;
            }
            else if (meta->next)
            {
                allocator = meta->next;
            }
            else
            {
                // Nullify bucket pointer if allocator has only one metadata.
                meta->bucket = NULL;
                for (size_t i = 0; i < MAX_FLAGS / sizeof(size_t); i++)
                {
                    meta->free_list[i] = SIZE_MAX;
                    meta->last_block[i] = SIZE_MAX;
                }
            }
        }
        else
        {
            // Mark block as free if bucket must not be unmapped.
            set_free(pos, meta->free_list);
        }
    }
}

/*
** Naive implementation of realloc where data is moved to a different bucket
** depending on realloc size.
*/
// TODO Optimize way of checking pointer validity (maybe add *last_meta pointer
// to bucket metadata to then check if page_begin of pointer is within start
// and last_meta range of bucket metadatas. Maybe check if ptr is aligned.
// TODO Optimize memcpy
__attribute__((visibility("default"))) void *realloc(void *ptr, size_t size)
{
    if (!ptr)
        return malloc(size);

    int pos;
    struct bucket_meta *meta = find_meta(ptr, &pos);

    if (meta)
    {
        size = align(size);
        void *new = malloc(size);

        if (new)
        {
            struct bucket_meta *new_meta = find_meta(ptr, NULL);
            new_meta->free_list[0] -= 1;
            mempcpy(new, ptr, meta->block_size);

            // Free old bucket.
            free(ptr);

            return new;
        }
    }

    return NULL;
}

__attribute__((visibility("default"))) void *calloc(size_t nmemb, size_t size)
{
    // Cast pointer to long double as blocks are multiples of this type.
    long double *new = malloc(nmemb * size);

    if (new)
    {
        size_t count = align(nmemb * size) / sizeof(long double);

        for (size_t i = 0; i < count; i++)
        {
            new[i] = 0;
        }
    }

    return new;
}
