#ifndef TOOLS_H
#define TOOLS_H

#define FULL(A) ((A) == 0)
#define FREE(A) ((A) == SIZE_MAX)
#define SIZE_BITS sizeof(size_t) * 8
#define SET_FREE(NUM, POS) ((NUM) |= 1 << (POS))
#define SET_USED(NUM, POS) ((NUM) &= ~(1 << (POS)))

int mark_block(size_t *free_list);
void *get_block(void *bucket, size_t n, size_t block_size);
void *page_begin(void *ptr, size_t page_size);
void set_free(size_t pos, size_t *free_list);

#endif /* !TOOLS_H */
