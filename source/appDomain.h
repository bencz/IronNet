#ifndef _DNI_APPDOMAIN_H_
#define _DNI_APPDOMAIN_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct _AppDomain AppDomain;
typedef struct _InstructionPointerMappingNode InstructionPointerMappingNode;
typedef struct _DomainSpecificMethod DomainSpecificMethod;
typedef struct _StaticGenericField StaticGenericField;

#include "uthash.h"
//#include "gc.h"
#include "irOpcode.h"
#include "irStructures.h"
#include "metadataStructures.h"

struct _AppDomain
{
	uint32_t DomainIndex;

	//Process* Process;
	uint32_t ThreadCount;
	//Thread** Threads;
	uint32_t IRAssemblyCount;
	IRAssembly** IRAssemblies;

	TypeDefinition* CachedType___System_Array;
	TypeDefinition* CachedType___System_Boolean;
	TypeDefinition* CachedType___System_Byte;
	TypeDefinition* CachedType___System_Char;
	TypeDefinition* CachedType___System_Double;
	TypeDefinition* CachedType___System_Enum;
	TypeDefinition* CachedType___System_Exception;
	TypeDefinition* CachedType___System_Int16;
	TypeDefinition* CachedType___System_Int32;
	TypeDefinition* CachedType___System_Int64;
	TypeDefinition* CachedType___System_IntPtr;
	TypeDefinition* CachedType___System_Object;
	TypeDefinition* CachedType___System_RuntimeFieldHandle;
	TypeDefinition* CachedType___System_RuntimeMethodHandle;
	TypeDefinition* CachedType___System_RuntimeTypeHandle;
	TypeDefinition* CachedType___System_SByte;
	TypeDefinition* CachedType___System_Single;
	TypeDefinition* CachedType___System_String;
	TypeDefinition* CachedType___System_Type;
	TypeDefinition* CachedType___System_TypedReference;
	TypeDefinition* CachedType___System_UInt16;
	TypeDefinition* CachedType___System_UInt32;
	TypeDefinition* CachedType___System_UInt64;
	TypeDefinition* CachedType___System_UIntPtr;
	TypeDefinition* CachedType___System_ValueType;
	TypeDefinition* CachedType___System_Void;
	TypeDefinition* CachedType___System_Threading_InternalThread;

	void** StaticValues;
	bool** StaticConstructorsCalled;

	InstructionPointerMappingNode** InstructionPointerMappingTree;
	DomainSpecificMethod* DomainSpecificMethodsTable;
	StaticGenericField* StaticGenericFieldsTable;

	//GC* GarbageCollector;
};

#endif