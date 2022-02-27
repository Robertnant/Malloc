#include <stddef.h>

// Start of the heap.
struct blk_meta *heap_start = NULL;

/*
** Finds a free block large enough to hold size parameter data or returns NULL
** if none is found. If a block is found then it is set as used.
*/
struct blk_meta *find_block(size_t size)
{
    struct blk_meta *curr = heap_start;

    while (curr)
    {
        if (curr->free && curr->size >= size)
        {
            curr->free = 0;
            return curr;
        }

        curr = curr->next;
    }

    return NULL;
}

// Splits block into one block of wanted size and another with remaining size.
void my_split(struct blk_meta *block, size_t size)
{
    struct blk_meta *new = block->data + size;

    new->size = block->size - (size - sizeof(struct blk_meta));
    block->size = size;

    new->free = 1;
    new->next = block->next;
    block->next = new;
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

    if (block)
    {
        // Split chunck found if can hold a new block (with its metadata).
        if (block->size > size + sizeof(struct blk_meta))
        {
            my_split(block, size);
        }
    }
    else
    {
        // If no block is found, map new memory for use.
        block = expand_heap();

        // Set heap start for the first time if not set.
        if (heap_start == NULL)
            heap_start = block;
    }

    return block->data;
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
