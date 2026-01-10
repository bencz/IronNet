/*
 * IronNet CLR Interpreter
 * corlib/corlib_main.c - Internal call registration
 */

#include "iron/corlib.h"

/* External declarations for internal calls from other files */

/* string.c */
extern iron_result_t icall_String_get_Length(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_String_get_Chars(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_String_Concat(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* object.c */
extern iron_result_t icall_Object_ctor(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Object_GetType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Object_GetHashCode(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Object_Equals(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Object_ReferenceEquals(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Object_ToString(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* console.c */
extern iron_result_t icall_Console_WriteLine_String(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Console_WriteLine_Int32(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Console_WriteLine_Object(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Console_Write_String(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Console_Write_Int32(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Console_ReadLine(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* math.c */
extern iron_result_t icall_Math_Abs_Int32(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Abs_Double(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Sin(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Cos(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Tan(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Sqrt(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Pow(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Log(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Log10(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Exp(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Floor(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Ceiling(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Round(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Min_Int32(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Math_Max_Int32(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* int32.c */
extern iron_result_t icall_Int32_ToString(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Int32_Parse(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* environment.c */
extern iron_result_t icall_Environment_get_TickCount(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Environment_Exit(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* array.c */
extern iron_result_t icall_Array_get_Length(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Array_get_Rank(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Array_Copy(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* ============================================================================
 * Internal Call Registration Table
 * ============================================================================ */

typedef struct {
    const char *type_name;
    const char *method_name;
    const char *signature;
    iron_internal_call_fn fn;
} icall_entry_t;

static const icall_entry_t g_icall_table[] = {
    /* System.Object */
    { "System.Object", ".ctor", "", icall_Object_ctor },
    { "System.Object", "GetType", "", icall_Object_GetType },
    { "System.Object", "GetHashCode", "", icall_Object_GetHashCode },
    { "System.Object", "Equals", "System.Object", icall_Object_Equals },
    { "System.Object", "ReferenceEquals", "System.Object,System.Object", icall_Object_ReferenceEquals },
    { "System.Object", "ToString", "", icall_Object_ToString },
    
    /* System.String */
    { "System.String", "get_Length", "", icall_String_get_Length },
    { "System.String", "get_Chars", "System.Int32", icall_String_get_Chars },
    { "System.String", "Concat", "System.String,System.String", icall_String_Concat },
    
    /* System.Int32 */
    { "System.Int32", "ToString", "", icall_Int32_ToString },
    { "System.Int32", "Parse", "System.String", icall_Int32_Parse },
    
    /* System.Console */
    { "System.Console", "WriteLine", "System.String", icall_Console_WriteLine_String },
    { "System.Console", "WriteLine", "System.Int32", icall_Console_WriteLine_Int32 },
    { "System.Console", "WriteLine", "System.Object", icall_Console_WriteLine_Object },
    { "System.Console", "WriteLine", "", icall_Console_WriteLine_String },
    { "System.Console", "Write", "System.String", icall_Console_Write_String },
    { "System.Console", "Write", "System.Int32", icall_Console_Write_Int32 },
    { "System.Console", "ReadLine", "", icall_Console_ReadLine },
    
    /* System.Math */
    { "System.Math", "Abs", "System.Int32", icall_Math_Abs_Int32 },
    { "System.Math", "Abs", "System.Double", icall_Math_Abs_Double },
    { "System.Math", "Sin", "System.Double", icall_Math_Sin },
    { "System.Math", "Cos", "System.Double", icall_Math_Cos },
    { "System.Math", "Tan", "System.Double", icall_Math_Tan },
    { "System.Math", "Sqrt", "System.Double", icall_Math_Sqrt },
    { "System.Math", "Pow", "System.Double,System.Double", icall_Math_Pow },
    { "System.Math", "Log", "System.Double", icall_Math_Log },
    { "System.Math", "Log10", "System.Double", icall_Math_Log10 },
    { "System.Math", "Exp", "System.Double", icall_Math_Exp },
    { "System.Math", "Floor", "System.Double", icall_Math_Floor },
    { "System.Math", "Ceiling", "System.Double", icall_Math_Ceiling },
    { "System.Math", "Round", "System.Double", icall_Math_Round },
    { "System.Math", "Min", "System.Int32,System.Int32", icall_Math_Min_Int32 },
    { "System.Math", "Max", "System.Int32,System.Int32", icall_Math_Max_Int32 },
    
    /* System.Environment */
    { "System.Environment", "get_TickCount", "", icall_Environment_get_TickCount },
    { "System.Environment", "Exit", "System.Int32", icall_Environment_Exit },
    
    /* System.Array */
    { "System.Array", "get_Length", "", icall_Array_get_Length },
    { "System.Array", "get_Rank", "", icall_Array_get_Rank },
    { "System.Array", "Copy", "System.Array,System.Array,System.Int32", icall_Array_Copy },
    
    /* Sentinel */
    { NULL, NULL, NULL, NULL }
};

/* ============================================================================
 * Registration Function
 * ============================================================================ */

iron_result_t iron_register_corlib(iron_exec_context_t *ctx)
{
    const icall_entry_t *entry;
    
    if (!ctx) return IRON_ERROR(IRON_ERR_INVALID_ARGUMENT, "ctx is NULL");
    
    for (entry = g_icall_table; entry->type_name != NULL; entry++) {
        iron_register_internal_call(ctx, entry->type_name, entry->method_name, 
                                    entry->signature, entry->fn);
    }
    
    return IRON_SUCCESS;
}
