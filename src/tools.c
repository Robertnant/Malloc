#include "tools.h"

/*
** Marks a block of the free_list as used.
** Returns position of block set if successful else -1 if all blocks used.
*/
int mark_block(struct free_list *free_list, size_t block_size)
{
    if (block_size == 0)
        return 0;

    size_t count = (PAGE_SIZE / block_size);
    count += count == 0 ? 1 : 0;

    size_t i;
    while (i < count && free_list[i].free == NO)
    {
        i++;
    }

    if (i < count)
    {
        free_list[i].free = NO;
        return i;
    }

    // All blocks used.
    return -1;
}

// Sets block at given position as free.
void set_free(size_t pos, struct free_list *free_list)
{
    free_list[pos].free = YES;
}

// Sets all blocks of free list as free.
void reset_list(struct free_list *free_list, size_t block_size)
{
    size_t i = 0;

    if (block_size == 0)
    {
        free_list[0].free = YES;
        i++;
    }
    else
    {
        size_t nb_flags = (PAGE_SIZE / block_size);
        nb_flags += nb_flags ? 0 : 1;

        while (i < nb_flags)
        {
            free_list[i].free = YES;
            i++;
        }
    }

    while (i < MAX_FLAGS)
    {
        free_list[i].free = NO;
        i++;
    }
}

// Gets the nth (0 indexed) block of a bucket using its block_s size.
void *get_block(void *bucket, int n, size_t block_size)
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
