#include "allocator.h"

// Allocates and returns a new blk_allocator structure.
struct blk_allocator *blka_new(void)
{
    return calloc(1, sizeof(struct blk_allocator));
}

// Allocates a blk_meta structure and adds to head of a blk_allocator.
struct blk_meta *blka_alloc(struct blk_allocator *blka, size_t size)
{
    struct blk_meta *new = malloc(sizeof(struct blk_meta));
    new->size = size;
    new->data = malloc(size * sizeof(char));
    new->next = blka->meta;

    return new;
}

// Deallocates memory of a given blk_meta structure.
void blka_free(struct blk_meta *blk)
{
    if (blk->size)
        free(blk->data);

    free(blk);
}

// De-allocates memory of the first blk_meta structure in a block allocator.
void blka_pop(struct blk_allocator *blka)
{
    blka_free(blka->meta);
}

// Free allocated blk_allocator.
void blka_delete(struct blk_allocator *blka)
{
    while (blka->meta)
    {
        struct blk_meta *next = blka->meta->next;
        blka_pop(blka->meta);
        blka->meta = next;
    }
}
