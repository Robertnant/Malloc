#define _GNU_SOURCE
#include <criterion/criterion.h>

#include "malloc.h"

Test(MALLOC, simple)
{
    char *str = malloc(10);
    free(str);
    cr_assert_eq(str == NULL, 0);
}

Test(CALLOC, simple)
{
    char *str = calloc(1, 500);
    free(str);
    cr_assert_eq(str == NULL, 0);
}

Test(CALLOC, overflow)
{
    size_t size = SIZE_MAX;
    char *str = calloc(1, size);
    cr_assert_eq(str == NULL, 1);
}

Test(REALLOC, simple)
{
    char *str = calloc(1, 20);
    char *str2 = realloc(str, 100);
    free(str);
    cr_assert_eq(str == str2, 0);
}

Test(REALLOC, same_block)
{
    char *str = calloc(1, 20);
    char *str2 = realloc(str, 30);
    free(str);
    cr_assert_eq(str == str2, 1);
}

Test(REALLOC, free_equivalent)
{
    char *str = calloc(1, 20);
    str = realloc(str, 0);

    int pos;
    struct bucket_meta *meta = find_meta(str, &pos);

    free(str);
    cr_assert_eq(meta->free_list[pos].free == YES, 1);
}

Test(REALLOC, malloc_equivalent)
{
    char *str = realloc(NULL, 30);
    free(str);
    cr_assert_eq(str == NULL, 0);
}
