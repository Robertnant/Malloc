#define _GNU_SOURCE
#include <criterion/criterion.h>

#include "malloc.h"

Test(MALLOC, simple)
{
    char *str = malloc(10);
    cr_assert_eq(str == NULL, 0);
}
