#ifndef TOOLS_H
#define TOOLS_H

#include <stddef.h>
#include <unistd.h>

#include "malloc.h"

#define SIZE_BITS sizeof(size_t) * 8

int mark_block(struct free_list *free_list, size_t size);
void *get_block(void *bucket, int n, size_t block_size);
void *page_begin(void *ptr, size_t page_size);
void set_free(size_t pos, struct free_list *free_list);
void reset_list(struct free_list *free_list, size_t block_size);

#endif /* !TOOLS_H */
