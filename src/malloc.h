#ifndef MALLOC_H
#define MALLOC_H

// Composed of the metadata of the block and a pointer to start of payload.
struct blk_meta
{
    int free;
    struct blk_meta *next;
    struct blk_meta *prev;
    size_t size;
    char data[];
};

struct blk_allocator
{
    struct blk_meta *meta;
};

struct blk_allocator *blka_new(void);
void blka_delete(struct blk_allocator *blka);

struct blk_meta *blka_alloc(struct blk_allocator *blka, size_t size);
void blka_free(struct blk_meta *blk);
void blka_pop(struct blk_allocator *blka);

#endif /* !MALLOC_H */
