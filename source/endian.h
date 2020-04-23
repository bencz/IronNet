//
// Created by Bencz on 22/04/2020.
//

#ifndef IRONNET_ENDIAN_H
#define IRONNET_ENDIAN_H

#if _WIN32 || _WIN64
    #define __BIG_ENDIAN 0
#else
    #include <endian.h>
    #if __BYTE_ORDER == __BIG_ENDIAN
        #define BIG_ENDIAN 1
    #endif
#endif

#include <stdint.h>

#if BIG_ENDIAN
#   define VAL16(x)    ((uint16_t)(((x) >> 8) | ((x) << 8)))
#   define VAL32(y)    ((uint32_t)(((y) >> 24) | (((y) >> 8) & 0x0000FF00L) | (((y) & 0x0000FF00L) << 8) | ((y) << 24)))
#   define VAL64(z)    ((uint64_t)(((uint64_t)VAL32(z) << 32) | VAL32((z) >> 32)))
#else
#   define VAL16(x)    x
#   define VAL32(y)    y
#   define VAL64(z)    z
#endif

#endif //IRONNET_ENDIAN_H
