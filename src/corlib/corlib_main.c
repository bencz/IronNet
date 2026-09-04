/*
 * IronNet CLR Interpreter
 * corlib/corlib_main.c - Internal call registration
 */

#include "iron/corlib.h"

/* External declarations for internal calls from other files */

/* string.c */
extern iron_result_t icall_String_get_Length(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_String_get_Item(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_String_Concat(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_String_ctor_CharCount(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_String_ctor_CharArray(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_String_ctor_CharArrayRange(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_String_ctor_CharPointer(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_String_ctor_CharPointerRange(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* object.c */
extern iron_result_t icall_Object_ctor(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Object_GetType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Object_GetHashCode(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Object_Equals(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Object_ReferenceEquals(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Object_ToString(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Object_MemberwiseClone(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* valuetype.c */
extern iron_result_t icall_ValueType_EqualsInternal(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_ValueType_GetHashCodeInternal(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* console.c */
extern iron_result_t icall_Console_WriteLine_String(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Console_WriteLine_Int32(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
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

/* floating.c */
extern iron_result_t icall_Single_ToString(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Double_ToString(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* reflection.c */
extern iron_result_t icall_Assembly_GetExecutingAssembly(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Assembly_GetCallingAssembly(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Assembly_GetEntryAssembly(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Assembly_GetAssembly(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeAssembly_get_FullName(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeAssembly_get_Location(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeAssembly_GetTypes(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeAssembly_GetTypeCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetMethodsCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetConstructors(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetFields(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetProperties(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetEvents(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetNestedTypes(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_IsAssignableFrom(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_IsInstanceOfType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_get_Name(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_get_DeclaringType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_get_Attributes(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_get_ReturnType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_get_ContainsGenericParameters(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_get_IsGenericMethod(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_get_IsGenericMethodDefinition(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_GetGenericArguments(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_GetGenericMethodDefinition(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_MakeGenericMethod(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_GetParameters(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeMethodInfo_Invoke(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeFieldInfo_get_Name(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeFieldInfo_get_DeclaringType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeFieldInfo_get_FieldType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeFieldInfo_get_Attributes(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeFieldInfo_GetValue(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeFieldInfo_SetValue(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeParameterInfo_get_Name(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeParameterInfo_get_ParameterType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeParameterInfo_get_Position(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeParameterInfo_get_IsOptional(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeParameterInfo_get_DefaultValue(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimePropertyInfo_get_Name(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimePropertyInfo_get_DeclaringType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimePropertyInfo_get_PropertyType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimePropertyInfo_get_CanRead(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimePropertyInfo_get_CanWrite(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimePropertyInfo_GetGetMethod(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimePropertyInfo_GetSetMethod(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimePropertyInfo_GetIndexParameters(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeEventInfo_get_Name(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeEventInfo_get_DeclaringType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeEventInfo_get_Attributes(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeEventInfo_get_EventHandlerType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeEventInfo_GetAddMethod(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeEventInfo_GetRemoveMethod(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_RuntimeEventInfo_GetRaiseMethod(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* environment.c */
extern iron_result_t icall_Environment_get_TickCount(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Environment_GetUtcNowTicks(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Environment_GetLocalNowTicks(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Environment_Exit(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* array.c */
extern iron_result_t icall_Array_get_Length(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Array_get_Rank(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Array_GetLength(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Array_GetLowerBound(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Array_GetUpperBound(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Array_Copy(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Array_GetValue(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Array_SetValue(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Array_CreateInstance(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* gc.c */
extern iron_result_t icall_GC_Collect(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_GC_GetTotalMemory(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_GC_SuppressFinalize(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_GC_ReRegisterForFinalize(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* type.c */
extern iron_result_t icall_Type_GetTypeFromHandle(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_Name(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_FullName(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_Namespace(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_BaseType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_DeclaringType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_Attributes(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_IsClass(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_IsValueType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_IsInterface(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_IsArray(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_IsEnum(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_HasElementType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_IsPointer(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_IsByRef(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_IsGenericType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_IsGenericTypeDefinition(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_IsGenericParameter(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_ContainsGenericParameters(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_GenericParameterPosition(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_GenericParameterAttributes(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_get_DeclaringMethod(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetElementType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetArrayRank(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetGenericArguments(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetGenericParameterConstraints(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetGenericTypeDefinitionCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_GetInterfaces(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_MakeGenericType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_MakeArrayType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_MakePointerType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Type_MakeByRefType(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* enum.c */
extern iron_result_t icall_Enum_ToString(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Enum_Equals(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Enum_GetHashCode(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Enum_IsDefinedCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Enum_GetNameCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Enum_GetNamesCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Enum_GetValuesCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* runtime_helpers.c */
extern iron_result_t icall_RuntimeHelpers_InitializeArray(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* threading.c */
extern iron_result_t icall_Interlocked_Increment_Int32(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_Increment_Int64(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_Decrement_Int32(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_Decrement_Int64(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_Exchange_Int32(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_Exchange_Int64(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_Exchange_Object(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_CompareExchange_Int32(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_CompareExchange_Int64(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_CompareExchange_Object(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_Add_Int32(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_Add_Int64(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Interlocked_MemoryBarrier(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Thread_Sleep(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Thread_StartInternal(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Thread_GetIsAliveInternal(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Thread_JoinInternal(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Thread_GetCurrentThreadInternal(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Monitor_Enter(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Monitor_Exit(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Monitor_TryEnter(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Monitor_Wait(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Monitor_Pulse(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Monitor_PulseAll(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* marshal.c */
extern iron_result_t icall_Marshal_SizeOfCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Marshal_AllocHGlobalCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Marshal_FreeHGlobal(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Marshal_CopyToNative(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Marshal_CopyFromNative(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Marshal_PtrToStringAnsiCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Marshal_PtrToStringUniCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Marshal_StringToHGlobalAnsiCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Marshal_StringToHGlobalUniCore(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_Marshal_GetLastWin32Error(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

/* string.c (additional) */
extern iron_result_t icall_String_Equals(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);
extern iron_result_t icall_String_InternalAllocateStr(iron_exec_context_t*, iron_stack_value_t*, iron_u32, iron_stack_value_t*);

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
    { "System.Object", "MemberwiseClone", "", icall_Object_MemberwiseClone },

    /* System.ValueType */
    { "System.ValueType", "EqualsInternal", "System.Object", icall_ValueType_EqualsInternal },
    { "System.ValueType", "GetHashCodeInternal", "", icall_ValueType_GetHashCodeInternal },

    /* System.String */
    { "System.String", ".ctor", "System.Char,System.Int32", icall_String_ctor_CharCount },
    { "System.String", ".ctor", "System.Char[]", icall_String_ctor_CharArray },
    { "System.String", ".ctor", "System.Char[],System.Int32,System.Int32", icall_String_ctor_CharArrayRange },
    { "System.String", ".ctor", "System.Char*", icall_String_ctor_CharPointer },
    { "System.String", ".ctor", "System.Char*,System.Int32,System.Int32", icall_String_ctor_CharPointerRange },
    { "System.String", "get_Length", "", icall_String_get_Length },
    { "System.String", "get_Item", "System.Int32", icall_String_get_Item },
    { "System.String", "Concat", "System.String,System.String", icall_String_Concat },
    { "System.String", "Equals", "System.String,System.String", icall_String_Equals },
    { "System.String", "InternalAllocateStr", "System.Int32", icall_String_InternalAllocateStr },
    
    /* System.Int32 */
    { "System.Int32", "ToString", "", icall_Int32_ToString },
    { "System.Int32", "Parse", "System.String", icall_Int32_Parse },

    /* System.Single and System.Double */
    { "System.Single", "ToString", "", icall_Single_ToString },
    { "System.Double", "ToString", "", icall_Double_ToString },

    /* System.Reflection.Assembly */
    { "System.Reflection.Assembly", "GetExecutingAssembly", "", icall_Assembly_GetExecutingAssembly },
    { "System.Reflection.Assembly", "GetCallingAssembly", "", icall_Assembly_GetCallingAssembly },
    { "System.Reflection.Assembly", "GetEntryAssembly", "", icall_Assembly_GetEntryAssembly },
    { "System.Reflection.Assembly", "GetAssembly", "System.Type", icall_Assembly_GetAssembly },
    { "System.Reflection.RuntimeAssembly", "get_FullName", "", icall_RuntimeAssembly_get_FullName },
    { "System.Reflection.RuntimeAssembly", "get_Location", "", icall_RuntimeAssembly_get_Location },
    { "System.Reflection.RuntimeAssembly", "GetTypes", "", icall_RuntimeAssembly_GetTypes },
    { "System.Reflection.RuntimeAssembly", "GetTypeCore", "System.String,System.Boolean", icall_RuntimeAssembly_GetTypeCore },
    { "System.Type", "GetMethodsCore", "System.Reflection.BindingFlags", icall_Type_GetMethodsCore },
    { "System.Type", "GetConstructors", "System.Reflection.BindingFlags", icall_Type_GetConstructors },
    { "System.Type", "GetFields", "System.Reflection.BindingFlags", icall_Type_GetFields },
    { "System.Type", "GetProperties", "System.Reflection.BindingFlags", icall_Type_GetProperties },
    { "System.Type", "GetEvents", "System.Reflection.BindingFlags", icall_Type_GetEvents },
    { "System.Type", "GetNestedTypes", "System.Reflection.BindingFlags", icall_Type_GetNestedTypes },
    { "System.Type", "IsAssignableFrom", "System.Type", icall_Type_IsAssignableFrom },
    { "System.Type", "IsInstanceOfType", "System.Object", icall_Type_IsInstanceOfType },
    { "System.Reflection.RuntimeMethodInfo", "get_Name", "", icall_RuntimeMethodInfo_get_Name },
    { "System.Reflection.RuntimeMethodInfo", "get_DeclaringType", "", icall_RuntimeMethodInfo_get_DeclaringType },
    { "System.Reflection.RuntimeMethodInfo", "get_Attributes", "", icall_RuntimeMethodInfo_get_Attributes },
    { "System.Reflection.RuntimeMethodInfo", "get_ReturnType", "", icall_RuntimeMethodInfo_get_ReturnType },
    { "System.Reflection.RuntimeMethodInfo", "get_ContainsGenericParameters", "", icall_RuntimeMethodInfo_get_ContainsGenericParameters },
    { "System.Reflection.RuntimeMethodInfo", "get_IsGenericMethod", "", icall_RuntimeMethodInfo_get_IsGenericMethod },
    { "System.Reflection.RuntimeMethodInfo", "get_IsGenericMethodDefinition", "", icall_RuntimeMethodInfo_get_IsGenericMethodDefinition },
    { "System.Reflection.RuntimeMethodInfo", "GetGenericArguments", "", icall_RuntimeMethodInfo_GetGenericArguments },
    { "System.Reflection.RuntimeMethodInfo", "GetGenericMethodDefinition", "", icall_RuntimeMethodInfo_GetGenericMethodDefinition },
    { "System.Reflection.RuntimeMethodInfo", "MakeGenericMethod", "System.Type[]", icall_RuntimeMethodInfo_MakeGenericMethod },
    { "System.Reflection.RuntimeMethodInfo", "GetParameters", "", icall_RuntimeMethodInfo_GetParameters },
    { "System.Reflection.RuntimeMethodInfo", "Invoke", "System.Object,System.Object[]", icall_RuntimeMethodInfo_Invoke },
    { "System.Reflection.RuntimeConstructorInfo", "get_Name", "", icall_RuntimeMethodInfo_get_Name },
    { "System.Reflection.RuntimeConstructorInfo", "get_DeclaringType", "", icall_RuntimeMethodInfo_get_DeclaringType },
    { "System.Reflection.RuntimeConstructorInfo", "get_Attributes", "", icall_RuntimeMethodInfo_get_Attributes },
    { "System.Reflection.RuntimeConstructorInfo", "get_ContainsGenericParameters", "", icall_RuntimeMethodInfo_get_ContainsGenericParameters },
    { "System.Reflection.RuntimeConstructorInfo", "GetParameters", "", icall_RuntimeMethodInfo_GetParameters },
    { "System.Reflection.RuntimeConstructorInfo", "Invoke", "System.Object,System.Object[]", icall_RuntimeMethodInfo_Invoke },
    { "System.Reflection.RuntimeFieldInfo", "get_Name", "", icall_RuntimeFieldInfo_get_Name },
    { "System.Reflection.RuntimeFieldInfo", "get_DeclaringType", "", icall_RuntimeFieldInfo_get_DeclaringType },
    { "System.Reflection.RuntimeFieldInfo", "get_FieldType", "", icall_RuntimeFieldInfo_get_FieldType },
    { "System.Reflection.RuntimeFieldInfo", "get_Attributes", "", icall_RuntimeFieldInfo_get_Attributes },
    { "System.Reflection.RuntimeFieldInfo", "GetValue", "System.Object", icall_RuntimeFieldInfo_GetValue },
    { "System.Reflection.RuntimeFieldInfo", "SetValue", "System.Object,System.Object", icall_RuntimeFieldInfo_SetValue },
    { "System.Reflection.RuntimeParameterInfo", "get_Name", "", icall_RuntimeParameterInfo_get_Name },
    { "System.Reflection.RuntimeParameterInfo", "get_ParameterType", "", icall_RuntimeParameterInfo_get_ParameterType },
    { "System.Reflection.RuntimeParameterInfo", "get_Position", "", icall_RuntimeParameterInfo_get_Position },
    { "System.Reflection.RuntimeParameterInfo", "get_IsOptional", "", icall_RuntimeParameterInfo_get_IsOptional },
    { "System.Reflection.RuntimeParameterInfo", "get_DefaultValue", "", icall_RuntimeParameterInfo_get_DefaultValue },
    { "System.Reflection.RuntimePropertyInfo", "get_Name", "", icall_RuntimePropertyInfo_get_Name },
    { "System.Reflection.RuntimePropertyInfo", "get_DeclaringType", "", icall_RuntimePropertyInfo_get_DeclaringType },
    { "System.Reflection.RuntimePropertyInfo", "get_PropertyType", "", icall_RuntimePropertyInfo_get_PropertyType },
    { "System.Reflection.RuntimePropertyInfo", "get_CanRead", "", icall_RuntimePropertyInfo_get_CanRead },
    { "System.Reflection.RuntimePropertyInfo", "get_CanWrite", "", icall_RuntimePropertyInfo_get_CanWrite },
    { "System.Reflection.RuntimePropertyInfo", "GetGetMethod", "", icall_RuntimePropertyInfo_GetGetMethod },
    { "System.Reflection.RuntimePropertyInfo", "GetSetMethod", "", icall_RuntimePropertyInfo_GetSetMethod },
    { "System.Reflection.RuntimePropertyInfo", "GetIndexParameters", "", icall_RuntimePropertyInfo_GetIndexParameters },
    { "System.Reflection.RuntimeEventInfo", "get_Name", "", icall_RuntimeEventInfo_get_Name },
    { "System.Reflection.RuntimeEventInfo", "get_DeclaringType", "", icall_RuntimeEventInfo_get_DeclaringType },
    { "System.Reflection.RuntimeEventInfo", "get_Attributes", "", icall_RuntimeEventInfo_get_Attributes },
    { "System.Reflection.RuntimeEventInfo", "get_EventHandlerType", "", icall_RuntimeEventInfo_get_EventHandlerType },
    { "System.Reflection.RuntimeEventInfo", "GetAddMethod", "", icall_RuntimeEventInfo_GetAddMethod },
    { "System.Reflection.RuntimeEventInfo", "GetRemoveMethod", "", icall_RuntimeEventInfo_GetRemoveMethod },
    { "System.Reflection.RuntimeEventInfo", "GetRaiseMethod", "", icall_RuntimeEventInfo_GetRaiseMethod },
    
    /* System.Console */
    { "System.Console", "WriteLine", "System.String", icall_Console_WriteLine_String },
    { "System.Console", "WriteLine", "System.Int32", icall_Console_WriteLine_Int32 },
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
    { "System.Environment", "GetUtcNowTicks", "", icall_Environment_GetUtcNowTicks },
    { "System.Environment", "GetLocalNowTicks", "", icall_Environment_GetLocalNowTicks },
    { "System.Environment", "Exit", "System.Int32", icall_Environment_Exit },
    
    /* System.Array */
    { "System.Array", "get_Length", "", icall_Array_get_Length },
    { "System.Array", "get_Rank", "", icall_Array_get_Rank },
    { "System.Array", "GetLength", "System.Int32", icall_Array_GetLength },
    { "System.Array", "GetLowerBound", "System.Int32", icall_Array_GetLowerBound },
    { "System.Array", "GetUpperBound", "System.Int32", icall_Array_GetUpperBound },
    { "System.Array", "Copy", "System.Array,System.Array,System.Int32", icall_Array_Copy },
    { "System.Array", "Copy", "System.Array,System.Int32,System.Array,System.Int32,System.Int32", icall_Array_Copy },
    { "System.Array", "Clear", "System.Array,System.Int32,System.Int32", icall_Array_Clear },
    { "System.Array", "GetValue", "System.Int32", icall_Array_GetValue },
    { "System.Array", "GetValue", "System.Int32,System.Int32", icall_Array_GetValue },
    { "System.Array", "GetValue", "System.Int32,System.Int32,System.Int32", icall_Array_GetValue },
    { "System.Array", "GetValue", "System.Int32[]", icall_Array_GetValue },
    { "System.Array", "SetValue", "System.Object,System.Int32", icall_Array_SetValue },
    { "System.Array", "SetValue", "System.Object,System.Int32,System.Int32", icall_Array_SetValue },
    { "System.Array", "SetValue", "System.Object,System.Int32,System.Int32,System.Int32", icall_Array_SetValue },
    { "System.Array", "SetValue", "System.Object,System.Int32[]", icall_Array_SetValue },
    { "System.Array", "CreateInstance", "System.Type,System.Int32[]", icall_Array_CreateInstance },
    { "System.Array", "CreateInstance", "System.Type,System.Int32[],System.Int32[]", icall_Array_CreateInstance },

    /* System.GC */
    { "System.GC", "Collect", "", icall_GC_Collect },
    { "System.GC", "GetTotalMemory", "System.Boolean", icall_GC_GetTotalMemory },
    { "System.GC", "SuppressFinalize", "System.Object", icall_GC_SuppressFinalize },
    { "System.GC", "ReRegisterForFinalize", "System.Object", icall_GC_ReRegisterForFinalize },

    /* System.Type */
    { "System.Type", "GetTypeFromHandle", "System.RuntimeTypeHandle", icall_Type_GetTypeFromHandle },
    { "System.Type", "get_Name", "", icall_Type_get_Name },
    { "System.Type", "get_FullName", "", icall_Type_get_FullName },
    { "System.Type", "get_Namespace", "", icall_Type_get_Namespace },
    { "System.Type", "get_BaseType", "", icall_Type_get_BaseType },
    { "System.Type", "get_DeclaringType", "", icall_Type_get_DeclaringType },
    { "System.Type", "get_Attributes", "", icall_Type_get_Attributes },
    { "System.Type", "get_IsClass", "", icall_Type_get_IsClass },
    { "System.Type", "get_IsValueType", "", icall_Type_get_IsValueType },
    { "System.Type", "get_IsInterface", "", icall_Type_get_IsInterface },
    { "System.Type", "get_IsArray", "", icall_Type_get_IsArray },
    { "System.Type", "get_IsEnum", "", icall_Type_get_IsEnum },
    { "System.Type", "get_HasElementType", "", icall_Type_get_HasElementType },
    { "System.Type", "get_IsPointer", "", icall_Type_get_IsPointer },
    { "System.Type", "get_IsByRef", "", icall_Type_get_IsByRef },
    { "System.Type", "get_IsGenericType", "", icall_Type_get_IsGenericType },
    { "System.Type", "get_IsGenericTypeDefinition", "", icall_Type_get_IsGenericTypeDefinition },
    { "System.Type", "get_IsGenericParameter", "", icall_Type_get_IsGenericParameter },
    { "System.Type", "get_ContainsGenericParameters", "", icall_Type_get_ContainsGenericParameters },
    { "System.Type", "get_GenericParameterPosition", "", icall_Type_get_GenericParameterPosition },
    { "System.Type", "get_GenericParameterAttributes", "", icall_Type_get_GenericParameterAttributes },
    { "System.Type", "get_DeclaringMethod", "", icall_Type_get_DeclaringMethod },
    { "System.Type", "GetElementType", "", icall_Type_GetElementType },
    { "System.Type", "GetArrayRank", "", icall_Type_GetArrayRank },
    { "System.Type", "GetGenericArguments", "", icall_Type_GetGenericArguments },
    { "System.Type", "GetGenericParameterConstraints", "", icall_Type_GetGenericParameterConstraints },
    { "System.Type", "GetGenericTypeDefinitionCore", "", icall_Type_GetGenericTypeDefinitionCore },
    { "System.Type", "GetInterfaces", "", icall_Type_GetInterfaces },
    { "System.Type", "MakeGenericType", "System.Type[]", icall_Type_MakeGenericType },
    { "System.Type", "MakeArrayType", "", icall_Type_MakeArrayType },
    { "System.Type", "MakeArrayType", "System.Int32", icall_Type_MakeArrayType },
    { "System.Type", "MakePointerType", "", icall_Type_MakePointerType },
    { "System.Type", "MakeByRefType", "", icall_Type_MakeByRefType },

    /* System.Enum */
    { "System.Enum", "ToString", "", icall_Enum_ToString },
    { "System.Enum", "Equals", "System.Object", icall_Enum_Equals },
    { "System.Enum", "GetHashCode", "", icall_Enum_GetHashCode },
    { "System.Enum", "IsDefinedCore", "System.Type,System.Object", icall_Enum_IsDefinedCore },
    { "System.Enum", "GetNameCore", "System.Type,System.Object", icall_Enum_GetNameCore },
    { "System.Enum", "GetNamesCore", "System.Type", icall_Enum_GetNamesCore },
    { "System.Enum", "GetValuesCore", "System.Type", icall_Enum_GetValuesCore },

    /* System.Runtime.CompilerServices.RuntimeHelpers */
    { "System.Runtime.CompilerServices.RuntimeHelpers", "InitializeArray", "System.Array,System.RuntimeFieldHandle", icall_RuntimeHelpers_InitializeArray },

    /* System.Threading.Interlocked */
    { "System.Threading.Interlocked", "Increment", "System.Int32&", icall_Interlocked_Increment_Int32 },
    { "System.Threading.Interlocked", "Increment", "System.Int64&", icall_Interlocked_Increment_Int64 },
    { "System.Threading.Interlocked", "Decrement", "System.Int32&", icall_Interlocked_Decrement_Int32 },
    { "System.Threading.Interlocked", "Decrement", "System.Int64&", icall_Interlocked_Decrement_Int64 },
    { "System.Threading.Interlocked", "Exchange", "System.Int32&,System.Int32", icall_Interlocked_Exchange_Int32 },
    { "System.Threading.Interlocked", "Exchange", "System.Int64&,System.Int64", icall_Interlocked_Exchange_Int64 },
    { "System.Threading.Interlocked", "Exchange", "System.Object&,System.Object", icall_Interlocked_Exchange_Object },
    { "System.Threading.Interlocked", "CompareExchange", "System.Int32&,System.Int32,System.Int32", icall_Interlocked_CompareExchange_Int32 },
    { "System.Threading.Interlocked", "CompareExchange", "System.Int64&,System.Int64,System.Int64", icall_Interlocked_CompareExchange_Int64 },
    { "System.Threading.Interlocked", "CompareExchange", "System.Object&,System.Object,System.Object", icall_Interlocked_CompareExchange_Object },
    { "System.Threading.Interlocked", "Add", "System.Int32&,System.Int32", icall_Interlocked_Add_Int32 },
    { "System.Threading.Interlocked", "Add", "System.Int64&,System.Int64", icall_Interlocked_Add_Int64 },
    { "System.Threading.Interlocked", "MemoryBarrier", "", icall_Interlocked_MemoryBarrier },

    /* System.Threading.Thread */
    { "System.Threading.Thread", "Sleep", "System.Int32", icall_Thread_Sleep },
    { "System.Threading.Thread", "StartInternal", "System.Delegate,System.Object,System.Boolean", icall_Thread_StartInternal },
    { "System.Threading.Thread", "GetIsAliveInternal", "", icall_Thread_GetIsAliveInternal },
    { "System.Threading.Thread", "JoinInternal", "System.Int32", icall_Thread_JoinInternal },
    { "System.Threading.Thread", "GetCurrentThreadInternal", "", icall_Thread_GetCurrentThreadInternal },

    /* System.Threading.Monitor */
    { "System.Threading.Monitor", "Enter", "System.Object", icall_Monitor_Enter },
    { "System.Threading.Monitor", "Exit", "System.Object", icall_Monitor_Exit },
    { "System.Threading.Monitor", "TryEnter", "System.Object,System.Int32", icall_Monitor_TryEnter },
    { "System.Threading.Monitor", "Wait", "System.Object,System.Int32", icall_Monitor_Wait },
    { "System.Threading.Monitor", "Pulse", "System.Object", icall_Monitor_Pulse },
    { "System.Threading.Monitor", "PulseAll", "System.Object", icall_Monitor_PulseAll },

    /* System.Runtime.InteropServices.Marshal */
    { "System.Runtime.InteropServices.Marshal", "SizeOfCore", "System.Type", icall_Marshal_SizeOfCore },
    { "System.Runtime.InteropServices.Marshal", "AllocHGlobalCore", "System.Int32", icall_Marshal_AllocHGlobalCore },
    { "System.Runtime.InteropServices.Marshal", "FreeHGlobal", "System.IntPtr", icall_Marshal_FreeHGlobal },
    { "System.Runtime.InteropServices.Marshal", "CopyToNative", "System.Byte[],System.Int32,System.IntPtr,System.Int32", icall_Marshal_CopyToNative },
    { "System.Runtime.InteropServices.Marshal", "CopyFromNative", "System.IntPtr,System.Byte[],System.Int32,System.Int32", icall_Marshal_CopyFromNative },
    { "System.Runtime.InteropServices.Marshal", "PtrToStringAnsiCore", "System.IntPtr", icall_Marshal_PtrToStringAnsiCore },
    { "System.Runtime.InteropServices.Marshal", "PtrToStringUniCore", "System.IntPtr", icall_Marshal_PtrToStringUniCore },
    { "System.Runtime.InteropServices.Marshal", "StringToHGlobalAnsiCore", "System.String", icall_Marshal_StringToHGlobalAnsiCore },
    { "System.Runtime.InteropServices.Marshal", "StringToHGlobalUniCore", "System.String", icall_Marshal_StringToHGlobalUniCore },
    { "System.Runtime.InteropServices.Marshal", "GetLastWin32Error", "", icall_Marshal_GetLastWin32Error },

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
