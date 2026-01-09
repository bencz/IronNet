/*
 * IronNet CLR Interpreter
 * test_main.c - Basic unit tests
 */

#include <stdio.h>
#include <string.h>
#include "iron/iron.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  Running %s... ", #name); \
    g_tests_run++; \
    test_##name(); \
    g_tests_passed++; \
    printf("PASSED\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAILED\n    Assertion failed: %s\n    at %s:%d\n", \
               #cond, __FILE__, __LINE__); \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

/* ============================================================================
 * Platform Tests
 * ============================================================================ */

TEST(platform_info)
{
    const iron_platform_info_t *info = iron_get_platform_info();
    ASSERT(info != NULL);
    ASSERT(info->arch_name != NULL);
    ASSERT(info->os_name != NULL);
    ASSERT(info->word_size > 0);
    ASSERT(info->ptr_size > 0);
}

TEST(endian_detection)
{
    iron_endian_t endian = iron_detect_endian();
    ASSERT(endian == IRON_ENDIAN_LITTLE || endian == IRON_ENDIAN_BIG);
}

TEST(byte_read_write)
{
    iron_u8 buf[8];
    
    /* Test 16-bit */
    iron_write_u16_le(buf, 0x1234);
    ASSERT_EQ(iron_read_u16_le(buf), 0x1234);
    
    /* Test 32-bit */
    iron_write_u32_le(buf, 0x12345678);
    ASSERT_EQ(iron_read_u32_le(buf), 0x12345678);
    
    /* Test 64-bit */
    iron_write_u64_le(buf, 0x123456789ABCDEF0ULL);
    ASSERT_EQ(iron_read_u64_le(buf), 0x123456789ABCDEF0ULL);
}

/* ============================================================================
 * Memory Tests
 * ============================================================================ */

TEST(system_allocator)
{
    iron_allocator_t *alloc = iron_system_allocator();
    void *ptr;
    
    ASSERT(alloc != NULL);
    
    ptr = iron_alloc(alloc, 1024);
    ASSERT(ptr != NULL);
    
    iron_free(alloc, ptr, 1024);
}

TEST(arena_allocator)
{
    iron_arena_t arena;
    void *p1, *p2, *p3;
    iron_arena_mark_t mark;
    
    iron_arena_init(&arena, NULL, 4096);
    
    p1 = iron_alloc(&arena.base, 100);
    ASSERT(p1 != NULL);
    
    mark = iron_arena_save(&arena);
    
    p2 = iron_alloc(&arena.base, 200);
    ASSERT(p2 != NULL);
    
    p3 = iron_alloc(&arena.base, 300);
    ASSERT(p3 != NULL);
    
    iron_arena_restore(&arena, mark);
    
    iron_allocator_destroy(&arena.base);
}

TEST(pool_allocator)
{
    iron_pool_t pool;
    void *objs[100];
    int i;
    
    iron_pool_init(&pool, NULL, 64, 16);
    
    for (i = 0; i < 100; i++) {
        objs[i] = iron_alloc(&pool.base, 64);
        ASSERT(objs[i] != NULL);
    }
    
    for (i = 0; i < 100; i++) {
        iron_free(&pool.base, objs[i], 64);
    }
    
    iron_allocator_destroy(&pool.base);
}

TEST(string_builder)
{
    iron_string_builder_t sb;
    iron_string_view_t sv;
    
    iron_sb_init(&sb, NULL, 16);
    
    iron_sb_append_cstr(&sb, "Hello");
    iron_sb_append_char(&sb, ' ');
    iron_sb_append_cstr(&sb, "World");
    
    sv = iron_sb_view(&sb);
    ASSERT_EQ(sv.length, 11);
    ASSERT(strncmp(sv.data, "Hello World", 11) == 0);
    
    iron_sb_destroy(&sb);
}

TEST(hashmap)
{
    iron_hashmap_t map;
    const char *key1 = "key1";
    const char *key2 = "key2";
    int val1 = 100, val2 = 200;
    int result;
    
    iron_hashmap_init(&map, NULL, sizeof(const char*), sizeof(int),
                      iron_hash_string, iron_string_eq);
    
    iron_hashmap_set(&map, &key1, &val1);
    iron_hashmap_set(&map, &key2, &val2);
    
    ASSERT(iron_hashmap_get(&map, &key1, &result));
    ASSERT_EQ(result, 100);
    
    ASSERT(iron_hashmap_get(&map, &key2, &result));
    ASSERT_EQ(result, 200);
    
    ASSERT(iron_hashmap_remove(&map, &key1));
    ASSERT(!iron_hashmap_get(&map, &key1, &result));
    
    iron_hashmap_destroy(&map);
}

/* ============================================================================
 * Type Tests
 * ============================================================================ */

TEST(string_view)
{
    iron_string_view_t sv1 = iron_sv_from_cstr("Hello World");
    iron_string_view_t sv2 = iron_sv_from_cstr("Hello");
    
    ASSERT_EQ(sv1.length, 11);
    ASSERT(iron_sv_starts_with(sv1, sv2));
    
    ASSERT_EQ(iron_sv_find(sv1, 'W'), 6);
    ASSERT_EQ(iron_sv_find(sv1, 'X'), -1);
}

TEST(error_strings)
{
    ASSERT_STR_EQ(iron_error_str(IRON_OK), "Success");
    ASSERT_STR_EQ(iron_error_str(IRON_ERR_OUT_OF_MEMORY), "Out of memory");
    ASSERT_STR_EQ(iron_error_str(IRON_ERR_FILE_NOT_FOUND), "File not found");
}

TEST(element_types)
{
    ASSERT_EQ(iron_element_type_size(IRON_TYPE_I4), 4);
    ASSERT_EQ(iron_element_type_size(IRON_TYPE_I8), 8);
    ASSERT_EQ(iron_element_type_size(IRON_TYPE_R4), 4);
    ASSERT_EQ(iron_element_type_size(IRON_TYPE_R8), 8);
    
    ASSERT(iron_element_type_is_primitive(IRON_TYPE_I4));
    ASSERT(iron_element_type_is_primitive(IRON_TYPE_R8));
    ASSERT(!iron_element_type_is_primitive(IRON_TYPE_CLASS));
    
    ASSERT(iron_element_type_is_reference(IRON_TYPE_STRING));
    ASSERT(iron_element_type_is_reference(IRON_TYPE_CLASS));
    ASSERT(!iron_element_type_is_reference(IRON_TYPE_I4));
}

/* ============================================================================
 * Opcode Tests
 * ============================================================================ */

TEST(opcode_decode)
{
    iron_u8 code1[] = { 0x00 };  /* nop */
    iron_u8 code2[] = { 0x2A };  /* ret */
    iron_u8 code3[] = { 0xFE, 0x01 };  /* ceq */
    iron_u32 size;
    iron_opcode_t op;
    
    op = iron_opcode_decode(code1, &size);
    ASSERT_EQ(op, IRON_CEE_NOP);
    ASSERT_EQ(size, 1);
    
    op = iron_opcode_decode(code2, &size);
    ASSERT_EQ(op, IRON_CEE_RET);
    ASSERT_EQ(size, 1);
    
    op = iron_opcode_decode(code3, &size);
    ASSERT_EQ(op, IRON_CEE_CEQ);
    ASSERT_EQ(size, 2);
}

TEST(opcode_names)
{
    ASSERT_STR_EQ(iron_opcode_name(IRON_CEE_NOP), "nop");
    ASSERT_STR_EQ(iron_opcode_name(IRON_CEE_RET), "ret");
    ASSERT_STR_EQ(iron_opcode_name(IRON_CEE_ADD), "add");
}

TEST(opcode_branch)
{
    ASSERT(iron_opcode_is_branch(IRON_CEE_BR));
    ASSERT(iron_opcode_is_branch(IRON_CEE_BR_S));
    ASSERT(iron_opcode_is_branch(IRON_CEE_BRTRUE));
    ASSERT(!iron_opcode_is_branch(IRON_CEE_NOP));
    ASSERT(!iron_opcode_is_branch(IRON_CEE_ADD));
}

/* ============================================================================
 * Stack Tests
 * ============================================================================ */

TEST(eval_stack)
{
    iron_eval_stack_t stack;
    iron_allocator_t *alloc = iron_system_allocator();
    
    iron_stack_init(&stack, alloc, 64);
    
    iron_stack_push_i32(&stack, 10);
    iron_stack_push_i32(&stack, 20);
    iron_stack_push_i64(&stack, 30);
    
    ASSERT_EQ(stack.size, 3);
    
    ASSERT_EQ(iron_stack_pop_i64(&stack), 30);
    ASSERT_EQ(iron_stack_pop_i32(&stack), 20);
    ASSERT_EQ(iron_stack_pop_i32(&stack), 10);
    
    ASSERT_EQ(stack.size, 0);
    
    iron_stack_destroy(&stack, alloc);
}

/* ============================================================================
 * Threading Tests
 * ============================================================================ */

TEST(mutex)
{
    iron_mutex_t mutex;
    iron_result_t result;
    
    result = iron_mutex_init(&mutex);
    ASSERT(IRON_RESULT_OK(result));
    
    iron_mutex_lock(&mutex);
    ASSERT(!iron_mutex_trylock(&mutex)); /* Should fail - already locked */
    iron_mutex_unlock(&mutex);
    
    ASSERT(iron_mutex_trylock(&mutex)); /* Should succeed */
    iron_mutex_unlock(&mutex);
    
    iron_mutex_destroy(&mutex);
}

TEST(spinlock)
{
    iron_spinlock_t lock = IRON_SPINLOCK_INIT;
    
    iron_spinlock_lock(&lock);
    ASSERT(!iron_spinlock_trylock(&lock));
    iron_spinlock_unlock(&lock);
    
    ASSERT(iron_spinlock_trylock(&lock));
    iron_spinlock_unlock(&lock);
}

TEST(atomics)
{
    volatile iron_i32 val = 0;
    
    iron_atomic_store_i32(&val, 42);
    ASSERT_EQ(iron_atomic_load_i32(&val), 42);
    
    ASSERT_EQ(iron_atomic_add_i32(&val, 10), 42);
    ASSERT_EQ(iron_atomic_load_i32(&val), 52);
    
    ASSERT(iron_atomic_cas_i32(&val, 52, 100));
    ASSERT_EQ(iron_atomic_load_i32(&val), 100);
    
    ASSERT(!iron_atomic_cas_i32(&val, 52, 200)); /* Should fail */
    ASSERT_EQ(iron_atomic_load_i32(&val), 100);
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void)
{
    printf("IronNet Unit Tests\n");
    printf("==================\n\n");
    
    printf("Platform Tests:\n");
    RUN_TEST(platform_info);
    RUN_TEST(endian_detection);
    RUN_TEST(byte_read_write);
    printf("\n");
    
    printf("Memory Tests:\n");
    RUN_TEST(system_allocator);
    RUN_TEST(arena_allocator);
    RUN_TEST(pool_allocator);
    RUN_TEST(string_builder);
    RUN_TEST(hashmap);
    printf("\n");
    
    printf("Type Tests:\n");
    RUN_TEST(string_view);
    RUN_TEST(error_strings);
    RUN_TEST(element_types);
    printf("\n");
    
    printf("Opcode Tests:\n");
    RUN_TEST(opcode_decode);
    RUN_TEST(opcode_names);
    RUN_TEST(opcode_branch);
    printf("\n");
    
    printf("Stack Tests:\n");
    RUN_TEST(eval_stack);
    printf("\n");
    
    printf("Threading Tests:\n");
    RUN_TEST(mutex);
    RUN_TEST(spinlock);
    RUN_TEST(atomics);
    printf("\n");
    
    printf("==================\n");
    printf("Results: %d/%d tests passed\n", g_tests_passed, g_tests_run);
    
    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
