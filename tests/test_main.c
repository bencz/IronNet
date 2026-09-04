/*
 * IronNet CLR Interpreter
 * test_main.c - Basic unit tests
 */

#include <stdio.h>
#include <string.h>
#include "iron/iron.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;
static iron_bool g_current_test_failed = IRON_FALSE;

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("  Running %s... ", #name); \
    g_tests_run++; \
    g_current_test_failed = IRON_FALSE; \
    test_##name(); \
    if (!g_current_test_failed) { \
        g_tests_passed++; \
        printf("PASSED\n"); \
    } \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        g_current_test_failed = IRON_TRUE; \
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
    ASSERT_STR_EQ(iron_error_str(IRON_ERR_ARGUMENT_NULL), "Argument cannot be null");
    ASSERT_STR_EQ(iron_error_str(IRON_ERR_ARGUMENT_OUT_OF_RANGE), "Argument is outside the valid range");
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

static volatile iron_i32 g_once_count = 0;

static void once_initializer(void)
{
    iron_atomic_inc_i32(&g_once_count);
}

static void *increment_thread(void *argument)
{
    volatile iron_i32 *value;

    value = (volatile iron_i32 *)argument;
    iron_atomic_inc_i32(value);
    return argument;
}

static void *semaphore_post_thread(void *argument)
{
    iron_semaphore_t *semaphore;

    semaphore = (iron_semaphore_t *)argument;
    iron_thread_sleep(10);
    iron_semaphore_post(semaphore);
    return argument;
}

TEST(thread_lifecycle)
{
    volatile iron_i32 value;
    iron_thread_t thread;
    iron_result_t result;
    void *thread_result;

    value = 0;
    result = iron_thread_create_simple(&thread, increment_thread, (void *)&value);
    ASSERT(IRON_RESULT_OK(result));

    thread_result = NULL;
    result = iron_thread_join(&thread, &thread_result);
    ASSERT(IRON_RESULT_OK(result));
    ASSERT_EQ(thread_result, (void *)&value);
    ASSERT_EQ(iron_atomic_load_i32(&value), 1);
}

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

TEST(recursive_mutex)
{
    iron_rmutex_t mutex;
    iron_result_t result;

    result = iron_rmutex_init(&mutex);
    ASSERT(IRON_RESULT_OK(result));

    iron_rmutex_lock(&mutex);
    ASSERT(iron_rmutex_trylock(&mutex));
    iron_rmutex_unlock(&mutex);
    iron_rmutex_unlock(&mutex);
    iron_rmutex_destroy(&mutex);
}

TEST(read_write_lock)
{
    iron_rwlock_t lock;
    iron_result_t result;

    result = iron_rwlock_init(&lock);
    ASSERT(IRON_RESULT_OK(result));

    iron_rwlock_rdlock(&lock);
    ASSERT(!iron_rwlock_trywrlock(&lock));
    iron_rwlock_unlock(&lock);

    iron_rwlock_wrlock(&lock);
    ASSERT(!iron_rwlock_tryrdlock(&lock));
    iron_rwlock_unlock(&lock);
    iron_rwlock_destroy(&lock);
}

TEST(semaphore)
{
    iron_semaphore_t semaphore;
    iron_thread_t thread;
    iron_result_t result;

    result = iron_semaphore_init(&semaphore, 0);
    ASSERT(IRON_RESULT_OK(result));
    ASSERT(!iron_semaphore_trywait(&semaphore));

    result = iron_thread_create_simple(&thread, semaphore_post_thread, &semaphore);
    ASSERT(IRON_RESULT_OK(result));
    ASSERT(iron_semaphore_timedwait(&semaphore, 1000));
    ASSERT(IRON_RESULT_OK(iron_thread_join(&thread, NULL)));
    ASSERT(!iron_semaphore_timedwait(&semaphore, 1));
    iron_semaphore_destroy(&semaphore);
}

TEST(event)
{
    iron_event_t event;
    iron_result_t result;

    result = iron_event_init(&event, IRON_TRUE, IRON_FALSE);
    ASSERT(IRON_RESULT_OK(result));
    ASSERT(!iron_event_timedwait(&event, 1));
    iron_event_set(&event);
    ASSERT(iron_event_timedwait(&event, 1));
    ASSERT(iron_event_timedwait(&event, 1));
    iron_event_reset(&event);
    ASSERT(!iron_event_timedwait(&event, 1));
    iron_event_destroy(&event);

    result = iron_event_init(&event, IRON_FALSE, IRON_TRUE);
    ASSERT(IRON_RESULT_OK(result));
    ASSERT(iron_event_timedwait(&event, 1));
    ASSERT(!iron_event_timedwait(&event, 1));
    iron_event_destroy(&event);
}

TEST(thread_local_storage)
{
    iron_tls_key_t key;
    iron_result_t result;
    int value;

    value = 42;
    result = iron_tls_create(&key);
    ASSERT(IRON_RESULT_OK(result));
    ASSERT_EQ(iron_tls_get(&key), NULL);
    iron_tls_set(&key, &value);
    ASSERT_EQ(iron_tls_get(&key), &value);
    iron_tls_destroy(&key);
}

TEST(once)
{
    iron_once_t once = IRON_ONCE_INIT;

    g_once_count = 0;
    iron_once(&once, once_initializer);
    iron_once(&once, once_initializer);
    ASSERT_EQ(iron_atomic_load_i32(&g_once_count), 1);
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
#ifndef IRON_NO_NATIVE_64BIT
    volatile iron_i64 val64 = 0;
#endif
    
    iron_atomic_store_i32(&val, 42);
    ASSERT_EQ(iron_atomic_load_i32(&val), 42);
    
    ASSERT_EQ(iron_atomic_add_i32(&val, 10), 42);
    ASSERT_EQ(iron_atomic_load_i32(&val), 52);
    
    ASSERT(iron_atomic_cas_i32(&val, 52, 100));
    ASSERT_EQ(iron_atomic_load_i32(&val), 100);
    
    ASSERT(!iron_atomic_cas_i32(&val, 52, 200)); /* Should fail */
    ASSERT_EQ(iron_atomic_load_i32(&val), 100);

#ifndef IRON_NO_NATIVE_64BIT
    iron_atomic_store_i64(&val64, 0x100000000LL);
    ASSERT_EQ(iron_atomic_add_i64(&val64, 5), 0x100000000LL);
    ASSERT(iron_atomic_cas_i64(&val64, 0x100000005LL, 9));
    ASSERT_EQ(iron_atomic_exchange_i64(&val64, 12), 9);
    ASSERT_EQ(iron_atomic_load_i64(&val64), 12);
#endif
}

/* ============================================================================
 * Protected Control Flow Tests
 * ============================================================================ */

static iron_interp_result_t execute_control_flow_instruction(const iron_u8 *code, iron_u32 code_size, iron_u32 *offset)
{
    iron_thread_context_t thread;
    iron_stack_frame_t frame;
    iron_interp_result_t result;

    memset(&thread, 0, sizeof(thread));
    memset(&frame, 0, sizeof(frame));
    frame.code = code;
    frame.code_size = code_size;
    frame.ip = *offset;
    thread.current_frame = &frame;

    result = iron_exec_instruction(&thread);
    *offset = frame.ip;
    return result;
}

TEST(leave_full_width_displacement)
{
    static iron_u8 code[70006];
    iron_u32 offset;

    code[0] = IRON_CEE_LEAVE;
    iron_write_u32_le(code + 1, 70000);
    offset = 0;
    ASSERT_EQ(execute_control_flow_instruction(code, sizeof(code), &offset), IRON_INTERP_OK);
    ASSERT_EQ(offset, 70005);

    code[70000] = IRON_CEE_LEAVE;
    iron_write_u32_le(code + 70001, (iron_u32)(iron_i32)-70005);
    offset = 70000;
    ASSERT_EQ(execute_control_flow_instruction(code, sizeof(code), &offset), IRON_INTERP_OK);
    ASSERT_EQ(offset, 0);
}

TEST(leave_operand_validation)
{
    iron_u8 code[5] = { IRON_CEE_LEAVE, 0, 0, 0, 0 };
    iron_u32 offset;
    iron_u32 length;

    for (length = 1; length < sizeof(code); length++) {
        offset = 0;
        ASSERT_EQ(execute_control_flow_instruction(code, length, &offset), IRON_INTERP_ERROR);
    }

    offset = 0;
    ASSERT_EQ(execute_control_flow_instruction(code, sizeof(code), &offset), IRON_INTERP_ERROR);

    iron_write_u32_le(code + 1, (iron_u32)(iron_i32)-6);
    ASSERT_EQ(execute_control_flow_instruction(code, sizeof(code), &offset), IRON_INTERP_ERROR);

    code[0] = IRON_CEE_LEAVE_S;
    ASSERT_EQ(execute_control_flow_instruction(code, 1, &offset), IRON_INTERP_ERROR);

    code[1] = 3;
    ASSERT_EQ(execute_control_flow_instruction(code, sizeof(code), &offset), IRON_INTERP_ERROR);
}

TEST(endfinally_requires_active_handler)
{
    const iron_u8 code[] = { IRON_CEE_ENDFINALLY };
    iron_u32 offset;

    offset = 0;
    ASSERT_EQ(execute_control_flow_instruction(code, sizeof(code), &offset), IRON_INTERP_ERROR);
}

TEST(fault_preserves_suspended_exception)
{
    const iron_u8 code[] = { IRON_CEE_NOP, IRON_CEE_ENDFINALLY };
    iron_thread_context_t thread;
    iron_exec_context_t context;
    iron_stack_frame_t frame;
    iron_runtime_method_t method;
    iron_method_body_t body;
    iron_exception_clause_t clause;
    iron_exception_t exception;
    iron_bool entered;
    iron_interp_result_t result;

    memset(&thread, 0, sizeof(thread));
    memset(&context, 0, sizeof(context));
    memset(&frame, 0, sizeof(frame));
    memset(&method, 0, sizeof(method));
    memset(&body, 0, sizeof(body));
    memset(&clause, 0, sizeof(clause));
    memset(&exception, 0, sizeof(exception));

    context.allocator = iron_system_allocator();
    thread.exec_ctx = &context;
    thread.current_frame = &frame;
    thread.exception_state.current_exception = &exception;
    frame.method = &method;
    frame.code = code;
    frame.code_size = sizeof(code);
    method.body = &body;
    body.exceptions = &clause;
    body.exception_count = 1;
    clause.flags = IRON_EX_CLAUSE_FAULT;
    clause.try_length = 1;
    clause.handler_offset = 1;
    clause.handler_length = 1;

    entered = iron_exec_enter_finally(&thread, 0, 0, 0, &exception);
    ASSERT(entered);
    ASSERT_EQ(thread.exception_state.current_exception, NULL);
    ASSERT_EQ(frame.finally_continuation->exception, &exception);

    result = iron_exec_instruction(&thread);
    iron_exec_discard_finally(&thread, UINT32_MAX);
    ASSERT_EQ(result, IRON_INTERP_EXCEPTION);
    ASSERT_EQ(thread.exception_state.current_exception, &exception);
    ASSERT_EQ(frame.finally_continuation, NULL);
    ASSERT_EQ(frame.flags & IRON_FRAME_FINALLY, 0);
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
    RUN_TEST(leave_full_width_displacement);
    RUN_TEST(leave_operand_validation);
    RUN_TEST(endfinally_requires_active_handler);
    RUN_TEST(fault_preserves_suspended_exception);
    printf("\n");
    
    printf("Threading Tests:\n");
    RUN_TEST(thread_lifecycle);
    RUN_TEST(mutex);
    RUN_TEST(recursive_mutex);
    RUN_TEST(read_write_lock);
    RUN_TEST(semaphore);
    RUN_TEST(event);
    RUN_TEST(thread_local_storage);
    RUN_TEST(once);
    RUN_TEST(spinlock);
    RUN_TEST(atomics);
    printf("\n");
    
    printf("==================\n");
    printf("Results: %d/%d tests passed\n", g_tests_passed, g_tests_run);
    
    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
