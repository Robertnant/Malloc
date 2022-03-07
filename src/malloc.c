#define _GNU_SOURCE

#include "malloc.h"

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "tools.h"

#define MAX_META PAGE_SIZE / sizeof(struct bucket_meta)

// Block allocator.
struct bucket_meta *allocator = NULL;

// Aligns size to make malloc implementation faster.
size_t align(size_t size)
{
    return (size + sizeof(long double) - 1) & ~(sizeof(long double) - 1);
}

/*
** Finds and returns metadata of a given valid block address.
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
        if (curr->bucket)
        {
            int block_pos = mark_block(curr->free_list, size);

            if (block_pos != -1)
            {
                return get_block(curr->bucket, block_pos, curr->block_size);
            }
        }

        // TODO With new change, this line below might cause a problem.
        *last_group = curr;

        // Move directly to next block with sibling.
        curr = curr->next_sibling;
    }

    // TODO: Optimize.
    // Get address of last metadata in allocator.
    while (curr->next)
    {
        curr = curr->next;
    }

    *last = curr;

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
    if (meta == NULL || meta->bucket == NULL)
        write(1, "isnulll\n", 8);

    // Increase bucket metadata count in allocator.
    allocator->count += 1;

    // Move to next metadata using pointer arithmetic.
    struct bucket_meta *new = meta + 1;

    // Check if current metadata is the last of current metadata page.
    struct bucket_meta *page_start = page_begin(meta, PAGE_SIZE);
    struct bucket_meta *last = page_start + (MAX_META - 1);

    if (last == meta)
    {
        new = mmap(NULL, PAGE_SIZE, PROT_READ |
                PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }

    meta->next = new;
    new->next = NULL;
    new->next_sibling = NULL;
    new->block_size = size;

    reset_list(new->free_list, new->last_block, size);

    // Map page for new bucket.
    new->bucket = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // Return a free block to the user.
    int block_pos = mark_block(new->free_list, size);

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
    new->count = 1;
    new->page_size = PAGE_SIZE;

    reset_list(new->free_list, new->last_block, size);

    // Map page for new bucket.
    new->bucket = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    return new;
}

// TODO requestBlock should not be called when last is NULL.
__attribute__((visibility("default"))) void *malloc(size_t size)
{
    // TODO: check size overflow.

    size = align(size);

    // Create new allocator containing addresses of buckets if not found yet.
    if (allocator == NULL)
    {
        allocator = init_alloc(size);

        // Return a free block to the user.
        int block_pos = mark_block(allocator->free_list, size);

        return get_block(allocator->bucket, block_pos, allocator->block_size);
    }

    // Find free block or create new bucket (after last one) to get new block.
    struct bucket_meta *last = NULL;
    struct bucket_meta *last_group = NULL;
    void *block = find_block(allocator, size, &last_group, &last);

    // requestBlock is alled when no free block was found.
    return block ? block : requestBlock(last, last_group, size);
}

// TODO: When unmapping a bucket, find way to make metadata slot reusable.
// (link as next bucket meta of last metadata in allocator).
__attribute__((visibility("default"))) void free(void *ptr)
{
    int pos;
    struct bucket_meta *meta = find_meta(ptr, &pos);

    if (meta)
    {
        // Mark block as free if bucket must not be unmapped.
        set_free(pos, meta->free_list);

        // Unmap bucket if all blocks are free.
        size_t nb_flags = (PAGE_SIZE / meta->block_size);
        size_t count = nb_flags / SIZE_BITS;
        count += count == 0 ? 1 : 0;

        size_t remaining_flags = nb_flags % SIZE_BITS;

        if (remaining_flags == 0)
            count += 1;

        size_t i = 0;

        while (i < count - 1 && (meta->free_list[i] == SIZE_MAX))
        {
            i++;
        }

        // Check last size_t of free list if all previous are free.
        if (i == count - 1)
        {
            if (remaining_flags == 0)
            {
                i++;
            }
            else
            {
                size_t pos = 0;
                while (pos < remaining_flags && IS_SET(meta->free_list[i], pos))
                {
                    pos++;
                }

                if (pos == remaining_flags)
                    i++;
            }
        }

        if (i == count)
        {
            munmap(meta->bucket, sysconf(_SC_PAGESIZE));

            // Unlink unmapped bucket meta from metadata list.
            if (meta != allocator)
            {
                struct bucket_meta *prev = meta - 1;
                prev->next = meta->next;
            }
            else if (meta->next)
            {
                meta->next->count = allocator->count;
                allocator = meta->next;
            }
            else
            {
                meta->bucket = NULL;
            }
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
