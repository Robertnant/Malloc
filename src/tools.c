#include "tools.h"
#include "malloc.h"

/*
** Marks a block of the free_list as used.
** Returns position of block set if successful else -1 if all blocks used.
*/
int mark_block(size_t *free_list, size_t block_size)
{
    size_t count = (PAGE_SIZE / block_size) / sizeof(size_t);
    count += count == 0 ? 1 : 0;

    size_t i;
    for (i = 0; i < count; i++)
    {
        if (!FULL(free_list[i]))
        {
            // Test each bit of free list until a free block is found.
            int pos = 0;
            while (!IS_SET(free_list[i], pos))
            {
                pos++;
            }

            SET_USED(free_list[i], pos);

            return pos;
        }
    }

    // All blocks used.
    return -1;
}

// Sets block at given position as free.
void set_free(size_t pos, size_t *free_list)
{
    // Get index of list containing flag wanted.
    size_t list_index = pos / SIZE_BITS;
    size_t index = pos % SIZE_BITS;

    SET_FREE(free_list[list_index], index);
}

// Gets the nth (0 indexed) block of a bucket using its block_s size.
void *get_block(void *bucket, size_t n, size_t block_size)
{
    char *bucket_cast = bucket;
    return bucket_cast + (n * block_size);
}

// Gets begining of page from given block address.
void *page_begin(void *ptr, size_t page_size)
{
    char *char_ptr = ptr;

    size_t start = (size_t)ptr & (page_size - 1);
    return char_ptr - start;
}
