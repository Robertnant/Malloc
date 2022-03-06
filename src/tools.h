#ifndef TOOLS_H
#define TOOLS_H

int mark_block(*size_t free_list);
void *get_block(void *bucket, size_t n, size_t block_size);
void *page_begin(void *ptr, size_t page_size);

#endif /* !TOOLS_H */
