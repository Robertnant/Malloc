#include "page_begin.h"

void *page_begin(void *ptr, size_t page_size)
{
    char *char_ptr = ptr;

    size_t start = (size_t)ptr & (page_size - 1);
    return char_ptr - start;
}
