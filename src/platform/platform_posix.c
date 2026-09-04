/*
 * IronNet CLR Interpreter
 * platform_posix.c - POSIX native memory and error backend
 */

#include "iron/platform.h"
#include <errno.h>
#include <stdlib.h>

void *iron_platform_alloc_hglobal(iron_size size)
{
    return malloc(size == 0 ? 1 : size);
}

void iron_platform_free_hglobal(void *memory)
{
    free(memory);
}

iron_i32 iron_platform_get_last_error(void)
{
    return (iron_i32)errno;
}
