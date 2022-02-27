#define _GNU_SOURCE

#include "allocator.h"

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// Allocates and returns a new blk_allocator structure.
struct blk_allocator *blka_new(void)
{
    return calloc(1, sizeof(struct blk_allocator));
}

// Allocates a blk_meta structure and adds to head of a blk_allocator.
struct blk_meta *blka_alloc(struct blk_allocator *blka, size_t size)
{
    // Get the system's page size.
    long pagesize = sysconf(_SC_PAGESIZE);

    /*
    ** Calculate number of pages of length pagesize needed to hold at least size
    ** amount of data.
    */

    // Sum of requested size and size of fields in blk_meta.
    size_t total_size = size + sizeof(struct blk_meta);
    size_t nb_pages = (total_size / pagesize) + (total_size % pagesize);

    // Length allocated is rounded up automatically to multiple of pagesize.
    struct blk_meta *new = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    new->size = nb_pages *pagesize - sizeof(struct blk_meta);
    new->next = blka->meta;

    return new;
}

// Deallocates memory of a given blk_meta structure.
void blka_free(struct blk_meta *blk)
{
    munmap(blk, blk->size + sizeof(struct blk_meta));
}

// De-allocates memory of the first blk_meta structure in a block allocator.
void blka_pop(struct blk_allocator *blka)
{
    struct blk_meta *next = blka->meta->next;
    blka_free(blka->meta);
    blka->meta = next;
}

// Free allocated blk_allocator.
void blka_delete(struct blk_allocator *blka)
{
    while (blka->meta)
    {
        blka_pop(blka);
    }
}
