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
        *pos = *pos == 0 ? 0 : *pos / curr->block_size;
    }

    return curr;
}

/*
** Traverses all bucket metadatas to find bucket with block size matching
** the (aligned) malloc size requested by user.
** The last_group parameter is updated when the last bucket metadata of wanted
** size is found but full.
*/
void *find_block(size_t size, struct bucket_meta **last_group)
{
    struct bucket_meta *curr = allocator;

    // Find first block with wanted size.
    while (curr && curr->block_size != size)
    {
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

        *last_group = curr;

        // Move directly to next block with sibling.
        curr = curr->next_sibling;
    }

    return NULL;
}

/*
** Creates a new bucket and appends metadata to last one of allocator.
** A pointer to a free block is returned.
** If a bucket with same size was previously found (last_group),
** they are linked.
*/
void *requestBlock(struct bucket_meta *last_group, size_t size)
{
    struct bucket_meta *meta = allocator->last_created;

    // Move to next metadata using pointer arithmetic.
    struct bucket_meta *new = meta + 1;

    // Check if current metadata is the last of current metadata page.
    struct bucket_meta *page_start = page_begin(meta, PAGE_SIZE);
    struct bucket_meta *last = page_start + (MAX_META - 1);

    if (last == meta)
    {
        new = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (new == MAP_FAILED)
            return NULL;
    }

    meta->next = new;
    new->next = NULL;
    new->next_sibling = NULL;
    new->block_size = size;

    // Update pointer of last_created metadata in allocator.
    allocator->last_created = new;

    reset_list(new->free_list, size);

    // Map page for new bucket.
    new->bucket = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (new->bucket == MAP_FAILED)
        return NULL;

    // Return a free block to the user.
    int block_pos = mark_block(new->free_list, size);

    void *block = get_block(new->bucket, block_pos, new->block_size);

    // Link last bucket with wanted size to new one.
    if (last_group)
        last_group->next_sibling = new;

    return block;
}

// Initializes a new bucket allocator.
struct bucket_meta *init_alloc(size_t size)
{
    struct bucket_meta *new =
        mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (new == MAP_FAILED)
        return NULL;

    new->next = NULL;
    new->next_sibling = NULL;
    new->block_size = size;
    new->page_size = PAGE_SIZE;
    new->last_created = new;

    reset_list(new->free_list, size);

    // Map page for new bucket.
    size = size ? size : align(size + 1);

    new->bucket = mmap(NULL, size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (new->bucket == MAP_FAILED)
        return NULL;

    return new;
}

__attribute__((visibility("default"))) void *malloc(size_t size)
{
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
    struct bucket_meta *last_group = NULL;
    void *block = find_block(size, &last_group);

    // requestBlock is alled when no free block was found.
    return block ? block : requestBlock(last_group, size);
}

// Checks if all blocks of a givzn bucket are free.
int is_free(struct bucket_meta *meta)
{
    if (meta->block_size == 0)
        return 0;

    size_t nb_flags = (PAGE_SIZE / meta->block_size);
    nb_flags += nb_flags == 0 ? 1 : 0;

    size_t i = 0;
    while (i < nb_flags && meta->free_list[i].free == YES)
    {
        i++;
    }

    return (i == nb_flags);
}

__attribute__((visibility("default"))) void free(void *ptr)
{
    if (!ptr)
        return;

    int pos;
    struct bucket_meta *meta = find_meta(ptr, &pos);

    if (!meta)
        return;

    // Check pointer validity.
    if (get_block(meta->bucket, pos, meta->block_size) != ptr)
    {
        errx(1, "invalid pointer");
        return;
    }

    if (meta)
    {
        // Mark block as free if bucket must not be unmapped.
        set_free(pos, meta->free_list);

        if (is_free(meta))
        {
            munmap(meta->bucket, sysconf(_SC_PAGESIZE));
            meta->bucket = NULL;

            // Unlink unmapped bucket meta from metadata list.
            if (meta == allocator)
            {
                if (meta->next)
                {
                    meta->next->last_created = allocator->last_created;
                    allocator = meta->next;
                }
            }
        }
    }
}

/*
** Naive implementation of realloc where data is moved to a different bucket
** depending on realloc size.
*/
__attribute__((visibility("default"))) void *realloc(void *ptr, size_t size)
{
    if (!ptr)
        return malloc(size);

    if (size == 0)
    {
        free(ptr);
        return ptr;
    }

    int pos;
    struct bucket_meta *meta = find_meta(ptr, &pos);

    if (meta)
    {
        // Check pointer validity.
        if (get_block(meta->bucket, pos, meta->block_size) != ptr)
        {
            errx(1, "invalid pointer");
            return NULL;
        }

        size = align(size);

        void *new = malloc(size);

        if (new)
        {
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
    // Check for overflow.
    size_t increment;
    int cond = __builtin_mul_overflow(size, nmemb, &increment);

    if (cond)
        return NULL;

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
