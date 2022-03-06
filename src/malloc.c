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
** Finds are returns metadata of a given valid block address.
** Returns position of block given in bucket through int pointer.
*/
struct bucket_meta *find_meta(void *ptr, int *pos)
{
    size_t *start = page_begin(ptr, sysconf(_SC_PAGESIZE));

    struct bucket_meta *curr = allocator;

    while (curr && curr->bucket != start)
    {
        curr = curr->next;
    }

    if (curr)
    {
        *pos = (size_t) ptr - start;
        pos = 0 ? 0 : pos / curr->block_size;
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
        struct bucket_meta *last_group, struct bucket_meta *last)
{
    struct bucket_meta *curr = allocator;

    // Find first block with wanted size.
    while (curr->block_size != size)
    {
        last = curr;
        curr = curr->next;
    }

    // Return a free block or move to next bucket of same size if current full.
    while (curr)
    {
        int block_pos = mark_block(curr->free_list);

        if (block_pos != -1)
            return get_block(curr->bucket, block_pos, curr->block_size);

        last_group = curr;

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

    void *block = get_block(new->bucket, block_pos, new->block_size);

    if (last_group)
        last_group->next_sibling = block;

    return block;
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
        new->free_list[i] = SIZE_MAX;
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
    struct bucket_meta *last_group = NULL;
    void *block = find_block(allocator, size, last_group, last);

    return block ? block : requestBlock(last, last_group, size);
}

__attribute__((visibility("default")))
void free(void *ptr)
{
    // Step 0: Check if pointer is valid.
    // Step 1: Get address begining of page.
    // Step 2: Find the address in allocator.
    // Step 3: With block size from metadata, determine nth position of ptr.
    // Step 4: Update free list.
    // Step 5: if all blocks are free, unmap bucket.
}

/*
** Naive implementation of realloc where data is moved to a different bucket
** depending on realloc size.
*/
__attribute__((visibility("default")))
void *realloc(void *ptr, size_t size)
{
    if (!ptr)
        return malloc(size);

    if (is_valid(ptr))
    {
        size_t aligned_size = align(size);
    }

    return NULL;
}

__attribute__((visibility("default")))
void *calloc(size_t nmemb, size_t size)
{
    // Cast pointer to long double as blocks are multiples of this type.
    long double *new = malloc(nmemb * size);

    if (new)
    {
        long double count = align(nmemb * size) / sizeof(long double);

        for (long double i = 0; i < count; i++)
        {
            new[i] = 0;
        }
    }

    return new;
}
