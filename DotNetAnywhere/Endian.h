//
// Created by Bencz on 24/04/2020.
//

#ifndef DNA_ENDIAN_H
#define DNA_ENDIAN_H


#if _WIN32 || _WIN64
#define BIGENDIAN 0
#else
#   include <endian.h>
#   if __BYTE_ORDER == __BIG_ENDIAN
#       define BIGENDIAN 1
#   endif
#endif

#include <stdint.h>

#define UINT16_SWAP_LE_BE_CONSTANT(x) ((((uint16_t) x) >> 8) | ((((uint16_t) x) << 8)))
#define UINT16_SWAP_LE_BE(x) ((uint16_t) (((uint16_t) x) >> 8) | ((((uint16_t)(x)) & 0xff) << 8))
#define UINT32_SWAP_LE_BE(x) ((uint32_t) \
			       ( (((uint32_t) (x)) << 24)| \
				 ((((uint32_t) (x)) & 0xff0000) >> 8) | \
		                 ((((uint32_t) (x)) & 0xff00) << 8) | \
			         (((uint32_t) (x)) >> 24)) )

#define UINT64_SWAP_LE_BE(x) ((uint64_t) (((uint64_t)(UINT32_SWAP_LE_BE(((uint64_t)x) & 0xffffffff))) << 32) | \
	      	               UINT32_SWAP_LE_BE(((uint64_t)x) >> 32))



#if BIGENDIAN == 1
#   define UINT64_FROM_BE(x) (x)
#   define UINT32_FROM_BE(x) (x)
#   define UINT16_FROM_BE(x) (x)
#   define UINT_FROM_BE(x)   (x)
#   define UINT64_FROM_LE(x) UINT64_SWAP_LE_BE(x)
#   define UINT32_FROM_LE(x) UINT32_SWAP_LE_BE(x)
#   define UINT16_FROM_LE(x) UINT16_SWAP_LE_BE(x)
#   define UINT_FROM_LE(x)   UINT32_SWAP_LE_BE(x)
#   define UINT64_TO_BE(x)   (x)
#   define UINT32_TO_BE(x)   (x)
#   define UINT16_TO_BE(x)   (x)
#   define UINT_TO_BE(x)     (x)
#   define UINT64_TO_LE(x)   UINT64_SWAP_LE_BE(x)
#   define UINT32_TO_LE(x)   UINT32_SWAP_LE_BE(x)
#   define UINT16_TO_LE(x)   UINT16_SWAP_LE_BE(x)
#   define UINT_TO_LE(x)     UINT32_SWAP_LE_BE(x)
#else
#   define UINT64_FROM_BE(x) UINT64_SWAP_LE_BE(x)
#   define UINT32_FROM_BE(x) UINT32_SWAP_LE_BE(x)
#   define UINT16_FROM_BE(x) UINT16_SWAP_LE_BE(x)
#   define UINT_FROM_BE(x)   UINT32_SWAP_LE_BE(x)
#   define UINT64_FROM_LE(x) (x)
#   define UINT32_FROM_LE(x) (x)
#   define UINT16_FROM_LE(x) (x)
#   define UINT_FROM_LE(x)   (x)
#   define UINT64_TO_BE(x)   UINT64_SWAP_LE_BE(x)
#   define UINT32_TO_BE(x)   UINT32_SWAP_LE_BE(x)
#   define UINT16_TO_BE(x)   UINT16_SWAP_LE_BE(x)
#   define UINT_TO_BE(x)     UINT32_SWAP_LE_BE(x)
#   define UINT64_TO_LE(x)   (x)
#   define UINT32_TO_LE(x)   (x)
#   define UINT16_TO_LE(x)   (x)
#   define UINT_TO_LE(x)     (x)
#endif


#if BIGENDIAN == 1
#	define SWAP16(x) (x) = UINT16_FROM_LE ((x))
#	define SWAP32(x) (x) = UINT32_FROM_LE ((x))
#	define SWAP64(x) (x) = UINT64_FROM_LE ((x))
#else
#	define SWAP64(x)
#	define SWAP32(x)
#	define SWAP16(x)
#endif

#if BIGENDIAN == 0 || BIGENDIAN == 1
#	define READ16(x) UINT16_FROM_LE (*((const uint16_t *) (x)))
#	define READ32(x) UINT32_FROM_LE (*((const uint32_t *) (x)))
#	define READ64(x) UINT64_FROM_LE (*((const uint64_t *) (x)))
#else
#endif

#endif //DNA_ENDIAN_H
