#include "beware_overflow.h"

void *beware_overflow(void *ptr, size_t nmemb, size_t size)
{
    if (size == 0)
        return NULL;

    // Check for overflow.
    size_t increment;
    int cond = __builtin_mul_overflow(size, nmemb, &increment);

    if (cond)
        return NULL;

    char *new_ptr = ptr;

    return new_ptr + increment;
}
