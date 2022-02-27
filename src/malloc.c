#include <stddef.h>

// Start of the heap.
struct blk_meta *heap_start = NULL;

// Finds a free block large enough to hold size parameter data.
struct blk_meta *find_block(size_t size)
{
    struct blk_meta *curr = heap_start;

    while (curr)
    {
        if (curr->free && curr->size >= size)
            return curr;

        curr = curr->next;
    }

    return NULL;
}

// Splits block into one block of wanted size and another with remaining size.
void split(struct blk_meta *block, size_t size)
{
}

// Aligns size to make malloc implementation faster.
size_t size align(size_t size)
{
    return (size + sizeof(intptr_t) - 1) & ~(sizeof(intptr_t) - 1);
}

__attribute__((visibility("default")))
void *malloc(size_t size)
{
    size = align(size);

    // Step 1: Find a free block large enough in list of blocks.
    struct blk_meta *block = find_block(size);

    // Split chunck found if necessary.
    if (block && (block->size > size + sizeof(struct blk_meta)))
    {
    }
    // Step 2: if no block is found, map new memory for use.
}

__attribute__((visibility("default")))
void free(void *ptr)
{
    // Gets pointer to start of block containing data and sets to free.
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
