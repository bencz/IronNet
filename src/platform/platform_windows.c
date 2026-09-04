/*
 * IronNet CLR Interpreter
 * platform_windows.c - Windows native memory and error backend
 */

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "iron/platform.h"

void *iron_platform_alloc_hglobal(iron_size size)
{
    SIZE_T allocation_size;

    allocation_size = size == 0 ? 1 : (SIZE_T)size;
    return (void *)LocalAlloc(LMEM_FIXED, allocation_size);
}

void iron_platform_free_hglobal(void *memory)
{
    if (memory) {
        (void)LocalFree((HLOCAL)memory);
    }
}

iron_i32 iron_platform_get_last_error(void)
{
    return (iron_i32)GetLastError();
}
