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

// Initializes a new bucket allocator.
struct bucket_meta *init_alloc(size_t size)
{
    struct bucket_meta *allocator = mmap(NULL, sysconf(_SC_PAGESIZE),
            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    allocator->next = NULL;
    allocator->next_sibling = NULL;
    allocator->block_size = size;

    for (size_t i = 0; i < sysconf(_SC_PAGESIZE) / sizeof(size_t); i++)
    {
        allocator->free_list[i] = SIZE_MAX;;
        allocator->last_block[i] = SIZE_MAX;
    }

    // Map page for new bucket.
    allocator->bucket = mmap(NULL, size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    return allocator;
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
}

__attribute__((visibility("default")))
void free(void *ptr)
{
}

__attribute__((visibility("default")))
void *realloc(void *ptr, size_t size)
{
    return NULL;
}

__attribute__((visibility("default")))
void *calloc(size_t nmemb, size_t size)
{

}
