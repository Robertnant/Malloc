include "tools.h"

#define FULL(A) ((A) == 0)

/*
** Marks a block of the free_list as used.
** Returns position of block set if successful else -1 if all blocks used.
*/
int mark_block(*size_t free_list)
{
    size_t count = sysconf(_SC_PAGESIZE) / sizeof(size_t);

    size_t i;
    for (i = 0; i < count; i++)
    {
        if (!FULL(free_list[i]))
        {
            // Test each bit of free list until a free block is found.
            int pos = 0;
            while (!is_set(free_list[i], pos))
            {
                pos++;
            }

            free_list[i] = SET_USED(free_list[i], pos);

            return pos;
        }
    }

    // All blocks used.
    return -1;
}

// Gets the nth (0 indexed) block of a bucket using its block_s size.
void *get_block(void *bucket, size_t n, size_t block_size)
{
    char *bucket_cast = bucket;
    return bucket_cast + (n * block_size);
}
