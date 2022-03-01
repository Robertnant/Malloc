#include <stddef.h>

#define SET_FREE(NUM, POS) ((NUM) |= 1 << (POS))
#define SET_USED(NUM, POS) ((NUM) &= ~(1 << (POS)))

// Block allocator.
struct bucket_meta *allocator = NULL;

// Aligns size to make malloc implementation faster.
size_t align(size_t size)
{
    return (size + sizeof(long double) - 1) & ~(sizeof(long double) - 1);
}

/*
** Traverses all bucket metadatas to find bucket with block size matching
** the (aligned) malloc size requested by user.
** A pointer to metadata of last bucket checked is returned (last bucket in
** list of metadata).
*/
void *find_block(struct bucket_meta *allocator, size_t size,
        struct bucket_meta *last)
{
    struct bucket_meta *curr = allocator;

    while (curr)
    {
        while (curr->block_size != size)
        {
            curr = curr->next;
        }

        if (curr)
        {
            // Return a free block or look for new bucket if current if full.
            int block_pos = mark_block(curr->free_list);

            if (block_pos != -1)
                return get_block(curr->bucket, block_pos, curr->block_size);

            last = curr;
            curr = curr->next;
        }
    }

    return NULL;
}

/*
** Creates a new bucket and appends metadata to list of metadata given.
** A pointer to a free block is returned.
*/
void *requestBlock(struct bucket_meta *meta, size_t size)
{
    struct bucket_meta *new = meta + sizeof(struct bucket_meta);

    meta->next = new;
    new->next = NULL;
    new->next_sibling = NULL;
    new->block_size = size;

    for (size_t i = 0; i < sysconf(_SC_PAGESIZE) / sizeof(size_t); i++)
    {
        new->free_list[i] = SIZE_MAX;
        new->last_block[i] = SIZE_MAX;
    }

    // Map page for new bucket.
    new->bucket = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |
            MAP_ANONYMOUS, -1, 0);

    // Return a free block to the user.
    int block_pos = mark_block(new->free_list);

    return get_block(new->bucket, block_pos, new->block_size);
}

// Initializes a new bucket allocator.
struct bucket_meta *init_alloc(size_t size)
{
    struct bucket_meta *new = mmap(NULL, sysconf(_SC_PAGESIZE),
            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    new->next = NULL;
    new->next_sibling = NULL;
    new->block_size = size;

    for (size_t i = 0; i < sysconf(_SC_PAGESIZE) / sizeof(size_t); i++)
    {
        new->free_list[i] = SIZE_MAX;;
        new->last_block[i] = SIZE_MAX;
    }

    // Map page for new bucket.
    new->bucket = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |
            MAP_ANONYMOUS, -1, 0);

    return new;
}

__attribute__((visibility("default")))
void *malloc(size_t size)
{
    // TODO: check size overflow.

    // TODO: check if align matches the one of subject.
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
    void *block = find_block(allocator, size, last);

    return block ? block : requestBlock(last, size);
}

__attribute__((visibility("default")))
void free(void *ptr)
{
    return NULL;
}

__attribute__((visibility("default")))
void *realloc(void *ptr, size_t size)
{
    return NULL;
}

__attribute__((visibility("default")))
void *calloc(size_t nmemb, size_t size)
{
    return NULL;
}
