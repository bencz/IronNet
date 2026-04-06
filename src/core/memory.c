/*
 * IronNet CLR Interpreter
 * memory.c - Memory management implementation
 */

#include "iron/memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* ============================================================================
 * Memory Utilities
 * ============================================================================ */

void iron_memcpy(void *dst, const void *src, iron_size size)
{
    memcpy(dst, src, size);
}

void iron_memmove(void *dst, const void *src, iron_size size)
{
    memmove(dst, src, size);
}

void iron_memset(void *dst, int val, iron_size size)
{
    memset(dst, val, size);
}

int iron_memcmp(const void *a, const void *b, iron_size size)
{
    return memcmp(a, b, size);
}

iron_size iron_strlen(const char *str)
{
    return strlen(str);
}

int iron_strcmp(const char *a, const char *b)
{
    return strcmp(a, b);
}

int iron_strncmp(const char *a, const char *b, iron_size n)
{
    return strncmp(a, b, n);
}

char *iron_strcpy(char *dst, const char *src)
{
    return strcpy(dst, src);
}

char *iron_strncpy(char *dst, const char *src, iron_size n)
{
    return strncpy(dst, src, n);
}

/* ============================================================================
 * System Allocator
 * ============================================================================ */

static void *sys_alloc(iron_allocator_t *self, iron_size size, iron_size align)
{
    void *ptr;
    (void)self;
    (void)align;
    
    ptr = malloc(size);
    if (ptr) {
        iron_memset(ptr, 0, size);
    }
    return ptr;
}

static void *sys_realloc(iron_allocator_t *self, void *ptr, iron_size old_size,
                         iron_size new_size, iron_size align)
{
    void *new_ptr;
    (void)self;
    (void)old_size;
    (void)align;
    
    new_ptr = realloc(ptr, new_size);
    if (new_ptr && new_size > old_size) {
        iron_memset((char*)new_ptr + old_size, 0, new_size - old_size);
    }
    return new_ptr;
}

static void sys_free(iron_allocator_t *self, void *ptr, iron_size size)
{
    (void)self;
    (void)size;
    free(ptr);
}

static void sys_reset(iron_allocator_t *self)
{
    (void)self;
    /* System allocator doesn't support reset */
}

static void sys_destroy(iron_allocator_t *self)
{
    (void)self;
    /* System allocator is a singleton, nothing to destroy */
}

static const iron_allocator_vtable_t g_sys_allocator_vt = {
    sys_alloc,
    sys_realloc,
    sys_free,
    sys_reset,
    sys_destroy
};

static iron_allocator_t g_sys_allocator = {
    &g_sys_allocator_vt
};

iron_allocator_t *iron_system_allocator(void)
{
    return &g_sys_allocator;
}

/* ============================================================================
 * Arena Allocator
 * ============================================================================ */

#define ARENA_DEFAULT_BLOCK_SIZE (64 * 1024)
#define ARENA_ALIGNMENT 16

static iron_size align_up(iron_size size, iron_size align)
{
    return (size + align - 1) & ~(align - 1);
}

static iron_arena_block_t *arena_new_block(iron_arena_t *arena, iron_size min_size)
{
    iron_size block_size;
    iron_arena_block_t *block;
    
    block_size = arena->block_size;
    if (min_size > block_size - sizeof(iron_arena_block_t)) {
        block_size = min_size + sizeof(iron_arena_block_t);
    }
    
    block = (iron_arena_block_t *)iron_alloc(arena->backing, block_size);
    if (!block) return NULL;
    
    block->next = NULL;
    block->size = block_size - sizeof(iron_arena_block_t);
    block->used = 0;
    
    arena->total_allocated += block_size;
    
    return block;
}

static void *arena_alloc(iron_allocator_t *self, iron_size size, iron_size align)
{
    iron_arena_t *arena = (iron_arena_t *)self;
    iron_arena_block_t *block;
    iron_size aligned_offset;
    void *ptr;
    
    if (size == 0) return NULL;
    
    /* Ensure minimum alignment */
    if (align < ARENA_ALIGNMENT) align = ARENA_ALIGNMENT;
    
    block = arena->current;
    if (block) {
        aligned_offset = align_up(block->used, align);
        if (aligned_offset + size <= block->size) {
            ptr = (char *)block + sizeof(iron_arena_block_t) + aligned_offset;
            block->used = aligned_offset + size;
            arena->total_used += size;
            return ptr;
        }
    }
    
    /* Need new block */
    block = arena_new_block(arena, size);
    if (!block) return NULL;
    
    if (arena->current) {
        arena->current->next = block;
    }
    arena->current = block;
    if (!arena->head) {
        arena->head = block;
    }
    
    ptr = (char *)block + sizeof(iron_arena_block_t);
    block->used = size;
    arena->total_used += size;
    
    return ptr;
}

static void *arena_realloc(iron_allocator_t *self, void *ptr, iron_size old_size,
                           iron_size new_size, iron_size align)
{
    void *new_ptr;
    
    if (!ptr) return arena_alloc(self, new_size, align);
    if (new_size == 0) return NULL;
    
    /* Arena doesn't support true realloc, just allocate new and copy */
    new_ptr = arena_alloc(self, new_size, align);
    if (new_ptr && old_size > 0) {
        iron_memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
    }
    return new_ptr;
}

static void arena_free(iron_allocator_t *self, void *ptr, iron_size size)
{
    /* Arena doesn't support individual frees */
    (void)self;
    (void)ptr;
    (void)size;
}

static void arena_reset(iron_allocator_t *self)
{
    iron_arena_t *arena = (iron_arena_t *)self;
    iron_arena_block_t *block;
    
    for (block = arena->head; block; block = block->next) {
        block->used = 0;
    }
    arena->current = arena->head;
    arena->total_used = 0;
}

static void arena_destroy(iron_allocator_t *self)
{
    iron_arena_t *arena = (iron_arena_t *)self;
    iron_arena_block_t *block, *next;
    
    for (block = arena->head; block; block = next) {
        next = block->next;
        iron_free(arena->backing, block, block->size + sizeof(iron_arena_block_t));
    }
    
    arena->head = NULL;
    arena->current = NULL;
    arena->total_allocated = 0;
    arena->total_used = 0;
}

static const iron_allocator_vtable_t g_arena_vt = {
    arena_alloc,
    arena_realloc,
    arena_free,
    arena_reset,
    arena_destroy
};

void iron_arena_init(iron_arena_t *arena, iron_allocator_t *backing, 
                     iron_size block_size)
{
    arena->base.vt = &g_arena_vt;
    arena->head = NULL;
    arena->current = NULL;
    arena->backing = backing ? backing : iron_system_allocator();
    arena->block_size = block_size > 0 ? block_size : ARENA_DEFAULT_BLOCK_SIZE;
    arena->total_allocated = 0;
    arena->total_used = 0;
}

iron_arena_t *iron_arena_create(iron_allocator_t *backing, iron_size block_size)
{
    iron_arena_t *arena;
    iron_allocator_t *alloc = backing ? backing : iron_system_allocator();
    
    arena = (iron_arena_t *)iron_alloc(alloc, sizeof(iron_arena_t));
    if (!arena) return NULL;
    
    iron_arena_init(arena, backing, block_size);
    return arena;
}

iron_arena_mark_t iron_arena_save(iron_arena_t *arena)
{
    iron_arena_mark_t mark;
    mark.block = arena->current;
    mark.used = arena->current ? arena->current->used : 0;
    return mark;
}

void iron_arena_restore(iron_arena_t *arena, iron_arena_mark_t mark)
{
    iron_arena_block_t *block;
    
    /* Reset all blocks after the marked one */
    if (mark.block) {
        for (block = mark.block->next; block; block = block->next) {
            block->used = 0;
        }
        mark.block->used = mark.used;
    } else {
        for (block = arena->head; block; block = block->next) {
            block->used = 0;
        }
    }
    
    arena->current = mark.block ? mark.block : arena->head;
}

/* ============================================================================
 * Pool Allocator
 * ============================================================================ */

#define POOL_DEFAULT_OBJECTS_PER_BLOCK 64

static void pool_add_block(iron_pool_t *pool)
{
    iron_size block_size;
    char *block, *obj;
    iron_size i;
    
    block_size = pool->object_size * pool->objects_per_block;
    block = (char *)iron_alloc(pool->backing, block_size);
    if (!block) return;
    
    /* Add block to list */
    if (pool->block_count >= pool->block_capacity) {
        iron_size new_cap = pool->block_capacity ? pool->block_capacity * 2 : 8;
        void **new_blocks = (void **)iron_realloc(pool->backing, pool->blocks,
            pool->block_capacity * sizeof(void*), new_cap * sizeof(void*));
        if (!new_blocks) {
            iron_free(pool->backing, block, block_size);
            return;
        }
        pool->blocks = new_blocks;
        pool->block_capacity = new_cap;
    }
    pool->blocks[pool->block_count++] = block;
    
    /* Add objects to free list */
    for (i = 0; i < pool->objects_per_block; i++) {
        obj = block + i * pool->object_size;
        *(void **)obj = pool->free_list;
        pool->free_list = obj;
    }
    
    pool->total_allocated += pool->objects_per_block;
    pool->total_free += pool->objects_per_block;
}

static void *pool_alloc(iron_allocator_t *self, iron_size size, iron_size align)
{
    iron_pool_t *pool = (iron_pool_t *)self;
    void *obj;
    
    (void)size;
    (void)align;
    
    if (!pool->free_list) {
        pool_add_block(pool);
        if (!pool->free_list) return NULL;
    }
    
    obj = pool->free_list;
    pool->free_list = *(void **)obj;
    pool->total_free--;
    
    iron_memset(obj, 0, pool->object_size);
    return obj;
}

static void *pool_realloc(iron_allocator_t *self, void *ptr, iron_size old_size,
                          iron_size new_size, iron_size align)
{
    /* Pool doesn't support realloc */
    (void)self;
    (void)ptr;
    (void)old_size;
    (void)new_size;
    (void)align;
    return NULL;
}

static void pool_free_obj(iron_allocator_t *self, void *ptr, iron_size size)
{
    iron_pool_t *pool = (iron_pool_t *)self;
    
    (void)size;
    
    if (!ptr) return;
    
    *(void **)ptr = pool->free_list;
    pool->free_list = ptr;
    pool->total_free++;
}

static void pool_reset(iron_allocator_t *self)
{
    iron_pool_t *pool = (iron_pool_t *)self;
    iron_size i, j;
    char *block, *obj;
    
    pool->free_list = NULL;
    pool->total_free = 0;
    
    for (i = 0; i < pool->block_count; i++) {
        block = (char *)pool->blocks[i];
        for (j = 0; j < pool->objects_per_block; j++) {
            obj = block + j * pool->object_size;
            *(void **)obj = pool->free_list;
            pool->free_list = obj;
        }
        pool->total_free += pool->objects_per_block;
    }
}

static void pool_destroy(iron_allocator_t *self)
{
    iron_pool_t *pool = (iron_pool_t *)self;
    iron_size i;
    iron_size block_size = pool->object_size * pool->objects_per_block;
    
    for (i = 0; i < pool->block_count; i++) {
        iron_free(pool->backing, pool->blocks[i], block_size);
    }
    
    if (pool->blocks) {
        iron_free(pool->backing, pool->blocks, 
                  pool->block_capacity * sizeof(void*));
    }
    
    pool->blocks = NULL;
    pool->block_count = 0;
    pool->block_capacity = 0;
    pool->free_list = NULL;
    pool->total_allocated = 0;
    pool->total_free = 0;
}

static const iron_allocator_vtable_t g_pool_vt = {
    pool_alloc,
    pool_realloc,
    pool_free_obj,
    pool_reset,
    pool_destroy
};

void iron_pool_init(iron_pool_t *pool, iron_allocator_t *backing,
                    iron_size object_size, iron_size objects_per_block)
{
    /* Ensure minimum object size for free list pointer */
    if (object_size < sizeof(void*)) {
        object_size = sizeof(void*);
    }
    /* Align object size */
    object_size = align_up(object_size, sizeof(void*));
    
    pool->base.vt = &g_pool_vt;
    pool->backing = backing ? backing : iron_system_allocator();
    pool->free_list = NULL;
    pool->object_size = object_size;
    pool->objects_per_block = objects_per_block > 0 ? 
                              objects_per_block : POOL_DEFAULT_OBJECTS_PER_BLOCK;
    pool->total_allocated = 0;
    pool->total_free = 0;
    pool->blocks = NULL;
    pool->block_count = 0;
    pool->block_capacity = 0;
}

iron_pool_t *iron_pool_create(iron_allocator_t *backing,
                              iron_size object_size, iron_size objects_per_block)
{
    iron_pool_t *pool;
    iron_allocator_t *alloc = backing ? backing : iron_system_allocator();
    
    pool = (iron_pool_t *)iron_alloc(alloc, sizeof(iron_pool_t));
    if (!pool) return NULL;
    
    iron_pool_init(pool, backing, object_size, objects_per_block);
    return pool;
}

/* ============================================================================
 * Dynamic Array
 * ============================================================================ */

void *iron_array_create_(iron_allocator_t *alloc, iron_size elem_size, 
                         iron_size capacity)
{
    iron_array_header_t *header;
    iron_size total_size;
    
    if (!alloc) alloc = iron_system_allocator();
    if (capacity == 0) capacity = 8;
    
    total_size = sizeof(iron_array_header_t) + elem_size * capacity;
    header = (iron_array_header_t *)iron_alloc(alloc, total_size);
    if (!header) return NULL;
    
    header->allocator = alloc;
    header->length = 0;
    header->capacity = capacity;
    
    return header + 1;
}

void *iron_array_grow_(void *array, iron_size elem_size, iron_size min_capacity)
{
    iron_array_header_t *header;
    iron_size new_capacity, old_size, new_size;
    iron_array_header_t *new_header;
    
    if (!array) {
        return iron_array_create_(NULL, elem_size, min_capacity);
    }
    
    header = IRON_ARRAY_HEADER(array);
    
    if (min_capacity <= header->capacity) {
        return array;
    }
    
    new_capacity = header->capacity * 2;
    if (new_capacity < min_capacity) {
        new_capacity = min_capacity;
    }
    
    old_size = sizeof(iron_array_header_t) + elem_size * header->capacity;
    new_size = sizeof(iron_array_header_t) + elem_size * new_capacity;
    
    new_header = (iron_array_header_t *)iron_realloc(header->allocator, 
                                                      header, old_size, new_size);
    if (!new_header) return NULL;
    
    new_header->capacity = new_capacity;
    return new_header + 1;
}

void iron_array_free_(void *array, iron_size elem_size)
{
    iron_array_header_t *header;
    iron_size total_size;
    
    if (!array) return;
    
    header = IRON_ARRAY_HEADER(array);
    total_size = sizeof(iron_array_header_t) + elem_size * header->capacity;
    iron_free(header->allocator, header, total_size);
}

/* ============================================================================
 * String Builder
 * ============================================================================ */

void iron_sb_init(iron_string_builder_t *sb, iron_allocator_t *alloc,
                  iron_size initial_capacity)
{
    sb->allocator = alloc ? alloc : iron_system_allocator();
    sb->length = 0;
    sb->capacity = initial_capacity > 0 ? initial_capacity : 64;
    sb->data = (char *)iron_alloc(sb->allocator, sb->capacity);
    if (sb->data) {
        sb->data[0] = '\0';
    }
}

void iron_sb_destroy(iron_string_builder_t *sb)
{
    if (sb->data) {
        iron_free(sb->allocator, sb->data, sb->capacity);
        sb->data = NULL;
    }
    sb->length = 0;
    sb->capacity = 0;
}

void iron_sb_clear(iron_string_builder_t *sb)
{
    sb->length = 0;
    if (sb->data) {
        sb->data[0] = '\0';
    }
}

void iron_sb_reserve(iron_string_builder_t *sb, iron_size capacity)
{
    char *new_data;
    
    if (capacity <= sb->capacity) return;
    
    new_data = (char *)iron_realloc(sb->allocator, sb->data, 
                                     sb->capacity, capacity);
    if (new_data) {
        sb->data = new_data;
        sb->capacity = capacity;
    }
}

void iron_sb_append(iron_string_builder_t *sb, const char *str, iron_size len)
{
    iron_size new_len;
    
    if (!str || len == 0) return;
    
    new_len = sb->length + len + 1;
    if (new_len > sb->capacity) {
        iron_size new_cap = sb->capacity * 2;
        if (new_cap < new_len) new_cap = new_len;
        iron_sb_reserve(sb, new_cap);
    }
    
    iron_memcpy(sb->data + sb->length, str, len);
    sb->length += len;
    sb->data[sb->length] = '\0';
}

void iron_sb_append_cstr(iron_string_builder_t *sb, const char *str)
{
    if (str) {
        iron_sb_append(sb, str, iron_strlen(str));
    }
}

void iron_sb_append_char(iron_string_builder_t *sb, char c)
{
    iron_sb_append(sb, &c, 1);
}

void iron_sb_printf(iron_string_builder_t *sb, const char *fmt, ...)
{
    va_list args;
    int len;
    char temp[1024];

    va_start(args, fmt);
    len = vsnprintf(temp, sizeof(temp), fmt, args);
    va_end(args);

    if (len < 0) {
        return;
    }

    /* Clamp to buffer size if output was truncated */
    if ((iron_size)len >= sizeof(temp)) {
        len = (int)(sizeof(temp) - 1);
    }

    if (len > 0) {
        iron_sb_append(sb, temp, (iron_size)len);
    }
}

iron_string_view_t iron_sb_view(const iron_string_builder_t *sb)
{
    iron_string_view_t sv;
    sv.data = sb->data;
    sv.length = sb->length;
    return sv;
}

char *iron_sb_to_cstr(iron_string_builder_t *sb, iron_allocator_t *alloc)
{
    char *str;
    
    if (!alloc) alloc = sb->allocator;
    
    str = (char *)iron_alloc(alloc, sb->length + 1);
    if (str) {
        iron_memcpy(str, sb->data, sb->length + 1);
    }
    return str;
}

/* ============================================================================
 * Hash Map
 * ============================================================================ */

#define HASHMAP_INITIAL_CAPACITY 16
#define HASHMAP_LOAD_FACTOR 0.75

/* FNV-1a hash */
static iron_u32 fnv1a_hash(const void *data, iron_size size)
{
    const iron_u8 *bytes = (const iron_u8 *)data;
    iron_u32 hash = 2166136261u;
    iron_size i;
    
    for (i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    
    return hash;
}

iron_u32 iron_hash_string(const void *key, iron_size key_size)
{
    const char *str = *(const char **)key;
    (void)key_size;
    return fnv1a_hash(str, iron_strlen(str));
}

iron_u32 iron_hash_ptr(const void *key, iron_size key_size)
{
    (void)key_size;
    return fnv1a_hash(key, sizeof(void*));
}

iron_u32 iron_hash_u32(const void *key, iron_size key_size)
{
    (void)key_size;
    return *(const iron_u32 *)key;
}

iron_bool iron_string_eq(const void *a, const void *b, iron_size key_size)
{
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    (void)key_size;
    return iron_strcmp(sa, sb) == 0;
}

iron_bool iron_ptr_eq(const void *a, const void *b, iron_size key_size)
{
    (void)key_size;
    return *(void **)a == *(void **)b;
}

void iron_hashmap_init(iron_hashmap_t *map, iron_allocator_t *alloc,
                       iron_size key_size, iron_size value_size,
                       iron_hash_fn hash_fn, iron_key_eq_fn key_eq_fn)
{
    map->allocator = alloc ? alloc : iron_system_allocator();
    map->entries = NULL;
    map->capacity = 0;
    map->count = 0;
    map->key_size = key_size;
    map->value_size = value_size;
    map->hash_fn = hash_fn ? hash_fn : fnv1a_hash;
    map->key_eq_fn = key_eq_fn;
}

void iron_hashmap_destroy(iron_hashmap_t *map)
{
    iron_size i;
    
    if (map->entries) {
        for (i = 0; i < map->capacity; i++) {
            if (map->entries[i].key) {
                iron_free(map->allocator, map->entries[i].key, map->key_size);
                iron_free(map->allocator, map->entries[i].value, map->value_size);
            }
        }
        iron_free(map->allocator, map->entries, 
                  map->capacity * sizeof(iron_hashmap_entry_t));
    }
    
    map->entries = NULL;
    map->capacity = 0;
    map->count = 0;
}

static void hashmap_resize(iron_hashmap_t *map, iron_size new_capacity)
{
    iron_hashmap_entry_t *old_entries = map->entries;
    iron_size old_capacity = map->capacity;
    iron_size i, index;
    
    map->entries = (iron_hashmap_entry_t *)iron_alloc(map->allocator,
        new_capacity * sizeof(iron_hashmap_entry_t));
    if (!map->entries) {
        map->entries = old_entries;
        return;
    }
    
    map->capacity = new_capacity;
    map->count = 0;
    
    /* Rehash all entries */
    for (i = 0; i < old_capacity; i++) {
        if (old_entries[i].key) {
            index = old_entries[i].hash % new_capacity;
            while (map->entries[index].key) {
                index = (index + 1) % new_capacity;
            }
            map->entries[index] = old_entries[i];
            map->count++;
        }
    }
    
    if (old_entries) {
        iron_free(map->allocator, old_entries, 
                  old_capacity * sizeof(iron_hashmap_entry_t));
    }
}

iron_bool iron_hashmap_get(iron_hashmap_t *map, const void *key, void *value_out)
{
    iron_u32 hash;
    iron_size index;
    
    if (map->capacity == 0) return IRON_FALSE;
    
    hash = map->hash_fn(key, map->key_size);
    index = hash % map->capacity;
    
    while (map->entries[index].key) {
        if (map->entries[index].hash == hash) {
            if (map->key_eq_fn) {
                if (map->key_eq_fn(map->entries[index].key, key, map->key_size)) {
                    if (value_out) {
                        iron_memcpy(value_out, map->entries[index].value, 
                                    map->value_size);
                    }
                    return IRON_TRUE;
                }
            } else if (iron_memcmp(map->entries[index].key, key, 
                                   map->key_size) == 0) {
                if (value_out) {
                    iron_memcpy(value_out, map->entries[index].value, 
                                map->value_size);
                }
                return IRON_TRUE;
            }
        }
        index = (index + 1) % map->capacity;
    }
    
    return IRON_FALSE;
}

void iron_hashmap_set(iron_hashmap_t *map, const void *key, const void *value)
{
    iron_u32 hash;
    iron_size index;
    
    /* Resize if needed */
    if (map->capacity == 0) {
        hashmap_resize(map, HASHMAP_INITIAL_CAPACITY);
    } else if ((double)map->count / map->capacity >= HASHMAP_LOAD_FACTOR) {
        hashmap_resize(map, map->capacity * 2);
    }
    
    hash = map->hash_fn(key, map->key_size);
    index = hash % map->capacity;
    
    while (map->entries[index].key) {
        if (map->entries[index].hash == hash) {
            iron_bool match = IRON_FALSE;
            if (map->key_eq_fn) {
                match = map->key_eq_fn(map->entries[index].key, key, map->key_size);
            } else {
                match = iron_memcmp(map->entries[index].key, key, 
                                    map->key_size) == 0;
            }
            if (match) {
                /* Update existing */
                iron_memcpy(map->entries[index].value, value, map->value_size);
                return;
            }
        }
        index = (index + 1) % map->capacity;
    }
    
    /* Insert new */
    map->entries[index].hash = hash;
    map->entries[index].key = iron_alloc(map->allocator, map->key_size);
    map->entries[index].value = iron_alloc(map->allocator, map->value_size);
    iron_memcpy(map->entries[index].key, key, map->key_size);
    iron_memcpy(map->entries[index].value, value, map->value_size);
    map->count++;
}

iron_bool iron_hashmap_remove(iron_hashmap_t *map, const void *key)
{
    iron_u32 hash;
    iron_size index;

    if (map->capacity == 0) return IRON_FALSE;

    hash = map->hash_fn(key, map->key_size);
    index = hash % map->capacity;

    while (map->entries[index].key) {
        if (map->entries[index].hash == hash) {
            iron_bool match = IRON_FALSE;
            if (map->key_eq_fn) {
                match = map->key_eq_fn(map->entries[index].key, key, map->key_size);
            } else {
                match = iron_memcmp(map->entries[index].key, key,
                                    map->key_size) == 0;
            }
            if (match) {
                iron_size empty;

                iron_free(map->allocator, map->entries[index].key, map->key_size);
                iron_free(map->allocator, map->entries[index].value, map->value_size);
                map->entries[index].key = NULL;
                map->entries[index].value = NULL;
                map->entries[index].hash = 0;
                map->count--;

                /* Backward-shift deletion: move displaced entries back
                 * to maintain open-addressing probe chains */
                empty = index;
                index = (index + 1) % map->capacity;
                while (map->entries[index].key) {
                    iron_size natural = map->entries[index].hash % map->capacity;
                    /* Check if this entry's natural position is at or before
                     * the empty slot (accounting for wraparound) */
                    if ((empty <= index) ?
                        (natural <= empty || natural > index) :
                        (natural <= empty && natural > index)) {
                        map->entries[empty] = map->entries[index];
                        map->entries[index].key = NULL;
                        map->entries[index].value = NULL;
                        map->entries[index].hash = 0;
                        empty = index;
                    }
                    index = (index + 1) % map->capacity;
                }

                return IRON_TRUE;
            }
        }
        index = (index + 1) % map->capacity;
    }

    return IRON_FALSE;
}

void iron_hashmap_clear(iron_hashmap_t *map)
{
    iron_size i;
    
    for (i = 0; i < map->capacity; i++) {
        if (map->entries[i].key) {
            iron_free(map->allocator, map->entries[i].key, map->key_size);
            iron_free(map->allocator, map->entries[i].value, map->value_size);
            map->entries[i].key = NULL;
            map->entries[i].value = NULL;
            map->entries[i].hash = 0;
        }
    }
    map->count = 0;
}

/* ============================================================================
 * String Interner
 * ============================================================================ */

void iron_interner_init(iron_interner_t *interner, iron_allocator_t *alloc)
{
    interner->allocator = alloc ? alloc : iron_system_allocator();
    iron_arena_init(&interner->string_arena, interner->allocator, 4096);
    iron_hashmap_init(&interner->map, interner->allocator,
                      sizeof(const char*), sizeof(const char*),
                      iron_hash_string, iron_string_eq);
}

void iron_interner_destroy(iron_interner_t *interner)
{
    iron_hashmap_destroy(&interner->map);
    iron_allocator_destroy(&interner->string_arena.base);
}

const char *iron_intern(iron_interner_t *interner, const char *str, iron_size len)
{
    const char *existing;
    char *interned;
    
    if (!str) return NULL;
    
    /* Check if already interned */
    if (iron_hashmap_get(&interner->map, &str, &existing)) {
        return existing;
    }
    
    /* Allocate and copy string */
    interned = (char *)iron_alloc(&interner->string_arena.base, len + 1);
    if (!interned) return NULL;
    
    iron_memcpy(interned, str, len);
    interned[len] = '\0';
    
    /* Add to map */
    iron_hashmap_set(&interner->map, &interned, &interned);
    
    return interned;
}

const char *iron_intern_cstr(iron_interner_t *interner, const char *str)
{
    if (!str) return NULL;
    return iron_intern(interner, str, iron_strlen(str));
}
