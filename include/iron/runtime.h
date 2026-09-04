/*
 * IronNet CLR Interpreter
 * runtime.h - Runtime type system and object model
 * 
 * Defines runtime representations of:
 * - Types (TypeDef, TypeRef, TypeSpec, generics)
 * - Methods, Fields, Properties, Events
 * - Assemblies and Modules
 * 
 * Strict C99 compatible
 */

#ifndef IRON_RUNTIME_H
#define IRON_RUNTIME_H

#include "platform.h"
#include "types.h"
#include "forward.h"
#include "memory.h"
#include "vtable.h"
#include "metadata.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Application Domain
 * ============================================================================ */

/* Assembly load callback */
typedef iron_assembly_t *(*iron_assembly_load_fn)(iron_domain_t *domain,
                                                   const char *name,
                                                   void *user_data);

/* Domain configuration */
typedef struct iron_domain_config {
    const char *name;
    const char *base_path;
    const char **assembly_paths;
    iron_u32 assembly_path_count;
    iron_assembly_load_fn load_callback;
    void *load_callback_data;
} iron_domain_config_t;

/* Application domain - isolation boundary */
struct iron_domain {
    iron_object_t base;
    iron_allocator_t *allocator;
    
    /* Domain info */
    const char *name;
    const char *base_path;
    
    /* Loaded assemblies */
    iron_assembly_t **assemblies;
    iron_u32 assembly_count;
    iron_u32 assembly_capacity;
    
    /* Type cache */
    iron_hashmap_t type_cache;        /* Full name -> type */
    iron_hashmap_t type_descriptors;  /* Descriptor address -> membership */
    iron_hashmap_t generic_inst_cache; /* Generic instantiation cache */
    iron_hashmap_t generic_method_cache; /* Generic method instantiation cache */
    
    /* String interner */
    iron_interner_t interner;
    
    /* Assembly resolution */
    const char **assembly_paths;
    iron_u32 assembly_path_count;
    iron_assembly_load_fn load_callback;
    void *load_callback_data;
    
    /* Core types (cached) */
    iron_runtime_type_t *type_object;
    iron_runtime_type_t *type_value_type;
    iron_runtime_type_t *type_enum;
    iron_runtime_type_t *type_string;
    iron_runtime_type_t *type_array;
    iron_runtime_type_t *type_delegate;
    iron_runtime_type_t *type_multicast_delegate;
    iron_runtime_type_t *type_exception;
    iron_runtime_type_t *type_type;
    iron_runtime_type_t *type_void;
    iron_runtime_type_t *type_boolean;
    iron_runtime_type_t *type_char;
    iron_runtime_type_t *type_sbyte;
    iron_runtime_type_t *type_byte;
    iron_runtime_type_t *type_int16;
    iron_runtime_type_t *type_uint16;
    iron_runtime_type_t *type_int32;
    iron_runtime_type_t *type_uint32;
    iron_runtime_type_t *type_int64;
    iron_runtime_type_t *type_uint64;
    iron_runtime_type_t *type_single;
    iron_runtime_type_t *type_double;
    iron_runtime_type_t *type_intptr;
    iron_runtime_type_t *type_uintptr;
};

/* Domain API */
IRON_API iron_result_t iron_domain_create(iron_domain_t **out_domain,
                                           const iron_domain_config_t *config,
                                           iron_allocator_t *alloc);
IRON_API void iron_domain_destroy(iron_domain_t *domain);
IRON_API void iron_domain_add_search_path(iron_domain_t *domain, const char *path);
IRON_API iron_result_t iron_domain_load_assembly(iron_domain_t *domain,
                                                  const char *path,
                                                  iron_assembly_t **out_assembly);

IRON_API iron_result_t iron_domain_load_assembly_memory(iron_domain_t *domain,
                                                         const iron_u8 *data,
                                                         iron_size size,
                                                         iron_assembly_t **out_assembly);
IRON_API iron_assembly_t *iron_domain_find_assembly(iron_domain_t *domain,
                                                     const char *name);
IRON_API iron_runtime_type_t *iron_domain_find_type(iron_domain_t *domain,
                                                     const char *full_name);
IRON_API iron_bool iron_domain_is_type_descriptor(const iron_domain_t *domain, const void *pointer);

/* ============================================================================
 * Assembly
 * ============================================================================ */

struct iron_assembly {
    iron_object_t base;
    iron_domain_t *domain;
    iron_allocator_t *allocator;
    
    /* PE image and metadata */
    iron_pe_image_t image;
    iron_metadata_t metadata;
    
    /* Assembly info */
    const char *name;
    const char *location;
    const char *culture;
    iron_u16 major_version;
    iron_u16 minor_version;
    iron_u16 build_number;
    iron_u16 revision_number;
    iron_u32 flags;
    const iron_u8 *public_key;
    iron_u32 public_key_size;
    void *managed_object;
    
    /* Modules */
    iron_module_t *module;  /* Main module */
    
    /* Referenced assemblies */
    iron_assembly_t **references;
    iron_u32 reference_count;
    
    /* Metadata object caches */
    iron_runtime_type_t *types;
    iron_u32 type_count;
    iron_runtime_method_t *methods;
    iron_u32 method_count;
    iron_runtime_field_t *fields;
    iron_u32 field_count;
    iron_runtime_property_t *properties;
    iron_u32 property_count;
    iron_runtime_event_t *events;
    iron_u32 event_count;
    
    /* Entry point */
    iron_runtime_method_t *entry_point;
};

/* Assembly API */
IRON_API iron_result_t iron_assembly_load(iron_assembly_t **out_assembly,
                                           iron_domain_t *domain,
                                           const char *path);
IRON_API iron_result_t iron_assembly_load_memory(iron_assembly_t **out_assembly,
                                                  iron_domain_t *domain,
                                                  const iron_u8 *data,
                                                  iron_size size);
IRON_API void iron_assembly_free(iron_assembly_t *assembly);
IRON_API iron_runtime_type_t *iron_assembly_find_type(iron_assembly_t *assembly,
                                                       const char *namespace_,
                                                       const char *name);
IRON_API iron_runtime_method_t *iron_assembly_get_entry_point(iron_assembly_t *assembly);
IRON_API iron_runtime_method_t *iron_resolve_method_token(iron_assembly_t *assembly, iron_token_t token);

/* ============================================================================
 * Module
 * ============================================================================ */

struct iron_module {
    iron_object_t base;
    iron_assembly_t *assembly;
    
    /* Module info */
    const char *name;
    iron_u8 mvid[16];  /* Module version ID (GUID) */
    
    /* Types defined in this module */
    iron_runtime_type_t **types;
    iron_u32 type_count;
    
    /* Global fields and methods */
    iron_runtime_field_t **global_fields;
    iron_u32 global_field_count;
    iron_runtime_method_t **global_methods;
    iron_u32 global_method_count;
};

/* ============================================================================
 * Runtime Type
 * ============================================================================ */

/* Type kind */
typedef enum iron_type_kind {
    IRON_KIND_CLASS,
    IRON_KIND_VALUETYPE,
    IRON_KIND_INTERFACE,
    IRON_KIND_ENUM,
    IRON_KIND_DELEGATE,
    IRON_KIND_ARRAY,
    IRON_KIND_POINTER,
    IRON_KIND_BYREF,
    IRON_KIND_GENERIC_PARAM,
    IRON_KIND_GENERIC_INST,
    IRON_KIND_FNPTR
} iron_type_kind_t;

/* Runtime type structure */
struct iron_runtime_type {
    const iron_runtime_type_vtable_t *vt;
    
    /* Identity */
    iron_token_t token;
    iron_module_t *module;
    
    /* Names (interned) */
    const char *name;
    const char *namespace_;
    const char *full_name;
    
    /* Type info */
    iron_type_kind_t kind;
    iron_u32 attrs;
    iron_element_type_t element_type;
    
    /* Layout */
    iron_u32 instance_size;
    iron_u32 alignment;
    iron_u32 packing_size;
    iron_u32 metadata_size;
    iron_bool layout_computed;
    
    /* Hierarchy */
    iron_runtime_type_t *base_type;
    iron_runtime_type_t **interfaces;
    iron_u32 interface_count;
    iron_runtime_type_t *declaring_type;  /* For nested types */
    iron_runtime_type_t *element;         /* For arrays, pointers, byrefs */
    iron_u32 array_rank;                  /* Zero for non-arrays */
    
    /* Members */
    iron_runtime_field_t **fields;
    iron_u32 field_count;
    iron_runtime_method_t **methods;
    iron_u32 method_count;
    iron_runtime_property_t **properties;
    iron_u32 property_count;
    iron_runtime_event_t **events;
    iron_u32 event_count;
    iron_runtime_type_t **nested_types;
    iron_u32 nested_type_count;
    
    /* Generics */
    iron_bool is_generic_definition;
    iron_bool is_generic_instance;
    iron_runtime_type_t **generic_params;
    iron_u32 generic_param_count;
    iron_runtime_type_t *generic_definition;  /* For instantiated types */
    iron_runtime_type_t **generic_args;       /* For instantiated types */
    iron_u32 generic_arg_count;
    iron_u32 generic_parameter_position;
    iron_runtime_method_t *declaring_method;  /* For method generic parameters */
    
    /* VTable */
    iron_runtime_method_t **vtable;
    iron_u32 vtable_size;
    
    /* Interface map */
    struct {
        iron_runtime_type_t *interface;
        iron_u32 vtable_offset;
    } *interface_map;
    iron_u32 interface_map_count;
    
    /* Static fields storage */
    void *static_data;
    iron_u32 static_data_size;
    iron_bool static_initializing;
    iron_bool static_initialized;
    
    /* Type initializer (.cctor) */
    iron_runtime_method_t *type_initializer;
};

/* Type API */
IRON_API iron_runtime_type_t *iron_type_nullable_argument(const iron_runtime_type_t *type);
IRON_API iron_bool iron_type_nullable_layout(iron_runtime_type_t *type, iron_u32 *has_value_offset, iron_u32 *value_offset);
IRON_API iron_runtime_type_t *iron_type_resolve_token(iron_module_t *module,
                                                       iron_token_t token);
IRON_API iron_runtime_type_t *iron_type_make_array(iron_domain_t *domain,
                                                    iron_runtime_type_t *element,
                                                    iron_u32 rank);
IRON_API iron_runtime_type_t *iron_type_make_mdarray(iron_domain_t *domain,
                                                      iron_runtime_type_t *element,
                                                      iron_u32 rank);
IRON_API iron_runtime_type_t *iron_type_make_pointer(iron_domain_t *domain,
                                                      iron_runtime_type_t *element);
IRON_API iron_runtime_type_t *iron_type_make_byref(iron_domain_t *domain,
                                                    iron_runtime_type_t *element);
IRON_API iron_runtime_type_t *iron_type_make_generic(iron_domain_t *domain,
                                                      iron_runtime_type_t *definition,
                                                      iron_runtime_type_t **args,
                                                      iron_u32 arg_count);
IRON_API iron_result_t iron_type_validate_generic_arguments(iron_runtime_type_t *definition,
                                                             iron_runtime_type_t **args,
                                                             iron_u32 arg_count);
IRON_API iron_result_t iron_generic_parameter_get_constraints(iron_runtime_type_t *parameter,
                                                               iron_allocator_t *allocator,
                                                               iron_runtime_type_t ***constraints,
                                                               iron_u32 *constraint_count);
IRON_API iron_bool iron_type_is_managed_reference(const iron_runtime_type_t *type);
IRON_API iron_bool iron_type_is_assignable_to(iron_runtime_type_t *source, iron_runtime_type_t *target);
IRON_API iron_runtime_method_t *iron_type_find_method_implementation(iron_runtime_type_t *type, const iron_runtime_method_t *contract);
IRON_API iron_bool iron_type_contains_generic_parameters(const iron_runtime_type_t *type);
IRON_API iron_size iron_type_storage_size(const iron_runtime_type_t *type);
IRON_API iron_result_t iron_type_compute_layout(iron_runtime_type_t *type);
IRON_API iron_result_t iron_type_init_static(iron_runtime_type_t *type,
                                              struct iron_exec_context *ctx);
IRON_API iron_runtime_method_t *iron_type_find_method(iron_runtime_type_t *type,
                                                       const char *name);
IRON_API iron_runtime_field_t *iron_type_find_instance_field(iron_runtime_type_t *type,
                                                              const char *name);

/* ============================================================================
 * Runtime Method
 * ============================================================================ */

/* Method kind */
typedef enum iron_method_kind {
    IRON_METHOD_NORMAL,
    IRON_METHOD_CONSTRUCTOR,
    IRON_METHOD_STATIC_CONSTRUCTOR,
    IRON_METHOD_PROPERTY_GET,
    IRON_METHOD_PROPERTY_SET,
    IRON_METHOD_EVENT_ADD,
    IRON_METHOD_EVENT_REMOVE,
    IRON_METHOD_EVENT_RAISE,
    IRON_METHOD_OPERATOR
} iron_method_kind_t;

struct iron_runtime_method {
    const iron_runtime_method_vtable_t *vt;
    
    /* Identity */
    iron_token_t token;
    iron_runtime_type_t *declaring_type;
    
    /* Name (interned) */
    const char *name;
    
    /* Attributes */
    iron_u16 attrs;
    iron_u16 impl_attrs;
    iron_method_kind_t kind;
    
    /* Signature */
    iron_call_conv_t calling_convention;
    iron_runtime_type_t *return_type;
    iron_runtime_param_t **params;
    iron_u32 param_count;
    iron_bool signature_loading;
    iron_bool signature_loaded;
    
    /* Generics */
    iron_bool is_generic_definition;
    iron_bool is_generic_instance;
    iron_runtime_type_t **generic_params;
    iron_u32 generic_param_count;
    iron_runtime_method_t *generic_definition;
    iron_runtime_type_t **generic_args;
    iron_u32 generic_arg_count;
    
    /* IL code */
    iron_method_body_t *body;
    
    /* Local variables */
    iron_runtime_type_t **locals;
    iron_u32 local_count;
    
    /* VTable slot (for virtual methods) */
    iron_i32 vtable_slot;
    
    /* Internal call (for runtime-implemented methods) */
    iron_bool is_internal_call;
    iron_method_invoke_fn internal_call;
    
    /* PInvoke info */
    iron_bool is_pinvoke;
    const char *pinvoke_module;
    const char *pinvoke_name;
    iron_u16 pinvoke_flags;
};

/* Method API */
IRON_API iron_runtime_method_t *iron_method_resolve_token(iron_module_t *module,
                                                           iron_token_t token);
IRON_API iron_runtime_method_t *iron_method_make_generic(iron_domain_t *domain,
                                                          iron_runtime_method_t *definition,
                                                          iron_runtime_type_t **args,
                                                          iron_u32 arg_count);
IRON_API iron_result_t iron_method_validate_generic_arguments(iron_runtime_method_t *definition,
                                                               iron_runtime_type_t **args,
                                                               iron_u32 arg_count);
IRON_API iron_result_t iron_method_load_signature(iron_runtime_method_t *method);
IRON_API iron_result_t iron_method_load_body(iron_runtime_method_t *method);

/* ============================================================================
 * Runtime Field
 * ============================================================================ */

struct iron_runtime_field {
    iron_object_t base;
    
    /* Identity */
    iron_token_t token;
    iron_runtime_type_t *declaring_type;
    
    /* Name (interned) */
    const char *name;
    
    /* Attributes */
    iron_u16 attrs;
    
    /* Type */
    iron_runtime_type_t *field_type;
    iron_element_type_t element_type;
    iron_u32 generic_param_index;
    
    /* Layout */
    iron_u32 offset;  /* Offset in instance or static data */
    iron_u32 size;
    
    /* Constant value (for literal fields) */
    iron_bool has_constant;
    iron_element_type_t constant_type;
    iron_value_t constant_value;
    const iron_u8 *constant_data;
    iron_u32 constant_data_size;
    
    /* RVA data (for fields with RVA) */
    const iron_u8 *rva_data;
    iron_u32 rva_size;
};

/* Field API */
IRON_API iron_runtime_field_t *iron_field_resolve_token(iron_module_t *module,
                                                         iron_token_t token);
IRON_API void *iron_field_get_address(iron_runtime_field_t *field, void *instance);

/* ============================================================================
 * Runtime Parameter
 * ============================================================================ */

struct iron_runtime_param {
    iron_object_t base;
    
    /* Identity */
    iron_token_t token;
    iron_runtime_method_t *method;
    
    /* Name (interned) */
    const char *name;
    
    /* Info */
    iron_u16 attrs;
    iron_u16 sequence;
    iron_runtime_type_t *param_type;
    
    /* Default value */
    iron_bool has_default;
    iron_element_type_t default_type;
    iron_value_t default_value;
    const iron_u8 *default_data;
    iron_u32 default_data_size;
};

/* ============================================================================
 * Runtime Property
 * ============================================================================ */

struct iron_runtime_property {
    iron_object_t base;
    
    /* Identity */
    iron_token_t token;
    iron_runtime_type_t *declaring_type;
    
    /* Name (interned) */
    const char *name;
    
    /* Attributes */
    iron_u16 attrs;
    
    /* Type */
    iron_runtime_type_t *property_type;
    iron_bool signature_loading;
    iron_bool signature_loaded;
    
    /* Accessors */
    iron_runtime_method_t *getter;
    iron_runtime_method_t *setter;
    
    /* Indexer parameters */
    iron_runtime_param_t **index_params;
    iron_u32 index_param_count;
};

IRON_API iron_runtime_property_t *iron_property_resolve_token(iron_module_t *module, iron_token_t token);
IRON_API iron_result_t iron_property_load_signature(iron_runtime_property_t *property);

/* ============================================================================
 * Runtime Event
 * ============================================================================ */

struct iron_runtime_event {
    iron_object_t base;
    
    /* Identity */
    iron_token_t token;
    iron_runtime_type_t *declaring_type;
    
    /* Name (interned) */
    const char *name;
    
    /* Attributes */
    iron_u16 attrs;
    
    /* Type */
    iron_runtime_type_t *event_type;
    iron_token_t event_type_token;
    
    /* Accessors */
    iron_runtime_method_t *add_method;
    iron_runtime_method_t *remove_method;
    iron_runtime_method_t *raise_method;
};

IRON_API iron_runtime_event_t *iron_event_resolve_token(iron_module_t *module, iron_token_t token);

/* ============================================================================
 * Generic Instantiation Cache
 * ============================================================================ */

struct iron_generic_inst {
    iron_runtime_type_t **args;
    iron_u32 arg_count;
    iron_u32 hash;
};

struct iron_generic_method_inst {
    iron_runtime_method_t *definition;
    iron_runtime_type_t **args;
    iron_u32 arg_count;
};

IRON_API iron_u32 iron_generic_inst_hash(const iron_generic_inst_t *inst);
IRON_API iron_bool iron_generic_inst_equals(const iron_generic_inst_t *a,
                                             const iron_generic_inst_t *b);

#ifdef __cplusplus
}
#endif

#endif /* IRON_RUNTIME_H */
