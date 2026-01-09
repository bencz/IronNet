/*
 * IronNet CLR Interpreter
 * memory.h - Memory management with arena allocator and vtable-based allocators
 * 
 * Pure C89 compatible
 */

#ifndef IRON_MEMORY_H
#define IRON_MEMORY_H

#include "platform.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Allocator Interface (vtable-based polymorphism)
 * ============================================================================ */

typedef struct iron_allocator iron_allocator_t;

/* Allocator vtable */
typedef struct iron_allocator_vtable {
    void *(*alloc)(iron_allocator_t *self, iron_size size, iron_size align);
    void *(*realloc)(iron_allocator_t *self, void *ptr, iron_size old_size, 
                     iron_size new_size, iron_size align);
    void  (*free)(iron_allocator_t *self, void *ptr, iron_size size);
    void  (*reset)(iron_allocator_t *self);
    void  (*destroy)(iron_allocator_t *self);
} iron_allocator_vtable_t;

/* Base allocator structure */
struct iron_allocator {
    const iron_allocator_vtable_t *vt;
};

/* Allocator interface macros */
#define iron_alloc(a, size) \
    ((a)->vt->alloc((a), (size), sizeof(void*)))

#define iron_alloc_aligned(a, size, align) \
    ((a)->vt->alloc((a), (size), (align)))

#define iron_realloc(a, ptr, old_size, new_size) \
    ((a)->vt->realloc((a), (ptr), (old_size), (new_size), sizeof(void*)))

#define iron_free(a, ptr, size) \
    ((a)->vt->free((a), (ptr), (size)))

#define iron_allocator_reset(a) \
    ((a)->vt->reset((a)))

#define iron_allocator_destroy(a) \
    ((a)->vt->destroy((a)))

/* Type-safe allocation helpers */
#define iron_new(a, T) \
    ((T*)iron_alloc((a), sizeof(T)))

#define iron_new_array(a, T, n) \
    ((T*)iron_alloc((a), sizeof(T) * (n)))

#define iron_delete(a, ptr, T) \
    iron_free((a), (ptr), sizeof(T))

#define iron_delete_array(a, ptr, T, n) \
    iron_free((a), (ptr), sizeof(T) * (n))

/* ============================================================================
 * System Allocator (malloc/free wrapper)
 * ============================================================================ */

IRON_API iron_allocator_t *iron_system_allocator(void);

/* ============================================================================
 * Arena Allocator - Fast bump allocation with bulk free
 * ============================================================================ */

/* Arena block */
typedef struct iron_arena_block {
    struct iron_arena_block *next;
    iron_size size;
    iron_size used;
    /* data follows */
} iron_arena_block_t;

/* Arena allocator */
typedef struct iron_arena {
    iron_allocator_t base;
    iron_arena_block_t *head;
    iron_arena_block_t *current;
    iron_allocator_t *backing;
    iron_size block_size;
    iron_size total_allocated;
    iron_size total_used;
} iron_arena_t;

/* Initialize arena with backing allocator */
IRON_API void iron_arena_init(iron_arena_t *arena, iron_allocator_t *backing, 
                               iron_size block_size);

/* Create new arena */
IRON_API iron_arena_t *iron_arena_create(iron_allocator_t *backing, 
                                          iron_size block_size);

/* Get arena as allocator */
#define iron_arena_allocator(arena) (&(arena)->base)

/* Arena checkpoint for temporary allocations */
typedef struct iron_arena_mark {
    iron_arena_block_t *block;
    iron_size used;
} iron_arena_mark_t;

/* Save arena state */
IRON_API iron_arena_mark_t iron_arena_save(iron_arena_t *arena);

/* Restore arena to saved state */
IRON_API void iron_arena_restore(iron_arena_t *arena, iron_arena_mark_t mark);

/* ============================================================================
 * Pool Allocator - Fixed-size object allocation
 * ============================================================================ */

typedef struct iron_pool {
    iron_allocator_t base;
    iron_allocator_t *backing;
    void *free_list;
    iron_size object_size;
    iron_size objects_per_block;
    iron_size total_allocated;
    iron_size total_free;
    void **blocks;
    iron_size block_count;
    iron_size block_capacity;
} iron_pool_t;

/* Initialize pool */
IRON_API void iron_pool_init(iron_pool_t *pool, iron_allocator_t *backing,
                              iron_size object_size, iron_size objects_per_block);

/* Create new pool */
IRON_API iron_pool_t *iron_pool_create(iron_allocator_t *backing,
                                        iron_size object_size, 
                                        iron_size objects_per_block);

/* Get pool as allocator */
#define iron_pool_allocator(pool) (&(pool)->base)

/* ============================================================================
 * Dynamic Array
 * ============================================================================ */

/* Array header (stored before data) */
typedef struct iron_array_header {
    iron_allocator_t *allocator;
    iron_size length;
    iron_size capacity;
} iron_array_header_t;

#define IRON_ARRAY_HEADER(a) \
    (((iron_array_header_t*)(a)) - 1)

#define IRON_ARRAY_LEN(a) \
    ((a) ? IRON_ARRAY_HEADER(a)->length : 0)

#define IRON_ARRAY_CAP(a) \
    ((a) ? IRON_ARRAY_HEADER(a)->capacity : 0)

/* Internal functions */
IRON_API void *iron_array_create_(iron_allocator_t *alloc, iron_size elem_size, 
                                   iron_size capacity);
IRON_API void *iron_array_grow_(void *array, iron_size elem_size, 
                                 iron_size min_capacity);
IRON_API void iron_array_free_(void *array, iron_size elem_size);

/* Type-safe array macros */
#define iron_array_create(alloc, T, cap) \
    ((T*)iron_array_create_((alloc), sizeof(T), (cap)))

#define iron_array_free(a) \
    (iron_array_free_((a), sizeof(*(a))), (a) = NULL)

#define iron_array_push(a, v) \
    ((a) = iron_array_grow_((a), sizeof(*(a)), IRON_ARRAY_LEN(a) + 1), \
     (a)[IRON_ARRAY_HEADER(a)->length++] = (v))

#define iron_array_pop(a) \
    ((a)[--IRON_ARRAY_HEADER(a)->length])

#define iron_array_last(a) \
    ((a)[IRON_ARRAY_LEN(a) - 1])

#define iron_array_clear(a) \
    ((a) ? (IRON_ARRAY_HEADER(a)->length = 0) : 0)

/* ============================================================================
 * String Builder
 * ============================================================================ */

typedef struct iron_string_builder {
    iron_allocator_t *allocator;
    char *data;
    iron_size length;
    iron_size capacity;
} iron_string_builder_t;

IRON_API void iron_sb_init(iron_string_builder_t *sb, iron_allocator_t *alloc,
                            iron_size initial_capacity);
IRON_API void iron_sb_destroy(iron_string_builder_t *sb);
IRON_API void iron_sb_clear(iron_string_builder_t *sb);
IRON_API void iron_sb_reserve(iron_string_builder_t *sb, iron_size capacity);
IRON_API void iron_sb_append(iron_string_builder_t *sb, const char *str, 
                              iron_size len);
IRON_API void iron_sb_append_cstr(iron_string_builder_t *sb, const char *str);
IRON_API void iron_sb_append_char(iron_string_builder_t *sb, char c);
IRON_API void iron_sb_printf(iron_string_builder_t *sb, const char *fmt, ...) 
    IRON_PRINTF(2, 3);
IRON_API iron_string_view_t iron_sb_view(const iron_string_builder_t *sb);
IRON_API char *iron_sb_to_cstr(iron_string_builder_t *sb, iron_allocator_t *alloc);

/* ============================================================================
 * Hash Map
 * ============================================================================ */

/* Hash function type */
typedef iron_u32 (*iron_hash_fn)(const void *key, iron_size key_size);

/* Key comparison function type */
typedef iron_bool (*iron_key_eq_fn)(const void *a, const void *b, 
                                     iron_size key_size);

/* Hash map entry */
typedef struct iron_hashmap_entry {
    iron_u32 hash;
    void *key;
    void *value;
} iron_hashmap_entry_t;

/* Hash map */
typedef struct iron_hashmap {
    iron_allocator_t *allocator;
    iron_hashmap_entry_t *entries;
    iron_size capacity;
    iron_size count;
    iron_size key_size;
    iron_size value_size;
    iron_hash_fn hash_fn;
    iron_key_eq_fn key_eq_fn;
} iron_hashmap_t;

IRON_API void iron_hashmap_init(iron_hashmap_t *map, iron_allocator_t *alloc,
                                 iron_size key_size, iron_size value_size,
                                 iron_hash_fn hash_fn, iron_key_eq_fn key_eq_fn);
IRON_API void iron_hashmap_destroy(iron_hashmap_t *map);
IRON_API iron_bool iron_hashmap_get(iron_hashmap_t *map, const void *key, 
                                     void *value_out);
IRON_API void iron_hashmap_set(iron_hashmap_t *map, const void *key, 
                                const void *value);
IRON_API iron_bool iron_hashmap_remove(iron_hashmap_t *map, const void *key);
IRON_API void iron_hashmap_clear(iron_hashmap_t *map);

/* Common hash functions */
IRON_API iron_u32 iron_hash_string(const void *key, iron_size key_size);
IRON_API iron_u32 iron_hash_ptr(const void *key, iron_size key_size);
IRON_API iron_u32 iron_hash_u32(const void *key, iron_size key_size);
IRON_API iron_bool iron_string_eq(const void *a, const void *b, iron_size key_size);
IRON_API iron_bool iron_ptr_eq(const void *a, const void *b, iron_size key_size);

/* ============================================================================
 * String Interner
 * ============================================================================ */

typedef struct iron_interner {
    iron_allocator_t *allocator;
    iron_arena_t string_arena;
    iron_hashmap_t map;
} iron_interner_t;

IRON_API void iron_interner_init(iron_interner_t *interner, iron_allocator_t *alloc);
IRON_API void iron_interner_destroy(iron_interner_t *interner);
IRON_API const char *iron_intern(iron_interner_t *interner, const char *str, 
                                  iron_size len);
IRON_API const char *iron_intern_cstr(iron_interner_t *interner, const char *str);

/* ============================================================================
 * Memory Utilities
 * ============================================================================ */

IRON_API void iron_memcpy(void *dst, const void *src, iron_size size);
IRON_API void iron_memmove(void *dst, const void *src, iron_size size);
IRON_API void iron_memset(void *dst, int val, iron_size size);
IRON_API int iron_memcmp(const void *a, const void *b, iron_size size);
IRON_API iron_size iron_strlen(const char *str);
IRON_API int iron_strcmp(const char *a, const char *b);
IRON_API int iron_strncmp(const char *a, const char *b, iron_size n);
IRON_API char *iron_strcpy(char *dst, const char *src);
IRON_API char *iron_strncpy(char *dst, const char *src, iron_size n);

#ifdef __cplusplus
}
#endif

#endif /* IRON_MEMORY_H */
