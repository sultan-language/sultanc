/* Owns Stage0 LLVM aggregate and tagged-value representation. */

#include "llvm/llvm_aggregates.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "llvm/runtime/llvm_runtime_memory.h"
#include "kernel/layout/layout.h"
#include "kernel/memory/memory.h"
#include "kernel/type/tagged.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Finds the LLVM aggregate type. */
__LLVM_Aggregate_Type__ *__LLVM_Find_Aggregate_Type__(__LLVM_Emitter__ *emitter,
                                                             __Semantic_Type_Entry__ *semantic)
{
    /* Tracks the index. */
    size_t index;
    for (index = 0U; index < emitter->aggregate_type_count; ++index)
    {
        if (emitter->aggregate_types[index].semantic == semantic)
        {
            return &emitter->aggregate_types[index];
        }
    }
    return NULL;
}

/* Returns the LLVM tagged byte address. */
/* Allocates staging storage for one tagged payload. Payload copies move the
 * canonical byte count, which can exceed the LLVM type's size (a Boolean is an
 * LLVM i1 field but has a larger canonical layout), so the storage holds the
 * LLVM type followed by the canonical byte count and has the canonical
 * alignment. Allocating only the LLVM type made the copy overflow the alloca. */
static LLVMValueRef __LLVM_Allocate_Payload_Stage__(__LLVM_Emitter__ *emitter,
                                                   LLVMTypeRef payload_llvm_type,
                                                   size_t payload_size,
                                                   size_t payload_alignment,
                                                   const char *name)
{
    /* Stores the staged member types. */
    LLVMTypeRef members[2];
    /* Stores the staged storage. */
    LLVMValueRef storage;
    members[0] = payload_llvm_type;
    members[1] = LLVMArrayType2(LLVMIntTypeInContext(emitter->context, 8U),
                                (unsigned long long)payload_size);
    storage = __LLVM_Allocate_Stack__(
        emitter, LLVMStructTypeInContext(emitter->context, members, 2U, 0), name);
    if (storage != NULL && payload_alignment > 1U)
        LLVMSetAlignment(storage, (unsigned)payload_alignment);
    return storage;
}

static LLVMValueRef __LLVM_Tagged_Byte_Address__(__LLVM_Emitter__ *emitter,
                                                 __Ast_Type__ *tagged_type,
                                                 LLVMTypeRef llvm_tagged_type,
                                                 LLVMValueRef storage,
                                                 size_t canonical_offset,
                                                 size_t access_size,
                                                 const char *name)
{
    /* Stores the payload size. */
    size_t payload_size = 0U;
    /* Stores the payload offset. */
    size_t payload_offset = 0U;
    /* Stores the byte type. */
    LLVMTypeRef byte_type;
    /* Stores the payload array type. */
    LLVMTypeRef payload_array_type;
    /* Stores the payload storage. */
    LLVMValueRef payload_storage;
    /* Stores the indices. */
    LLVMValueRef indices[2];
    /* Stores the relative. */
    size_t relative;

    if (!__LLVM_Tagged_Layout__(emitter, tagged_type, &payload_size, &payload_offset, NULL) ||
        canonical_offset < payload_offset)
    {
        __LLVM_Fail__("L2.5 tagged payload offset is outside canonical storage");
        return NULL;
    }
    relative = canonical_offset - payload_offset;
    if (relative > payload_size || access_size > payload_size - relative)
    {
        __LLVM_Fail__("L2.5 tagged payload access exceeds canonical payload storage");
        return NULL;
    }
    byte_type = LLVMIntTypeInContext(emitter->context, 8U);
    payload_array_type = LLVMArrayType(byte_type, (unsigned)payload_size);
    payload_storage = LLVMBuildStructGEP2(
        emitter->builder, llvm_tagged_type, storage, 1U, "tagged.payload.storage");
    if (payload_storage == NULL)
    {
        __LLVM_Fail__("L2.5 could not address tagged payload storage");
        return NULL;
    }
    indices[0] = LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), 0U, 0);
    indices[1] =
        LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)relative, 0);
    return LLVMBuildGEP2(emitter->builder, payload_array_type, payload_storage, indices, 2U, name);
}

/* Emits the LLVM tagged construct. */
__LLVM_Value__ __LLVM_Emit_Tagged_Construct__(__LLVM_Emitter__ *emitter,
                                                     __Ast_Type__ *target_type,
                                                     size_t constructor_index,
                                                     __Ast_Expression__ **arguments,
                                                     size_t argument_count)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the LLVM type. */
    LLVMTypeRef llvm_type;
    /* Stores the storage. */
    LLVMValueRef storage;
    /* Stores the tag address. */
    LLVMValueRef tag_address;
    /* Tracks the payload index. */
    size_t payload_index;
    /* Stores the payload capacity. */
    size_t payload_capacity = 0U;

    if (target_type == NULL ||
        !__LLVM_Tagged_Layout__(emitter, target_type, &payload_capacity, NULL, NULL))
    {
        return result;
    }
    llvm_type = __LLVM_Type__(emitter, target_type);
    if (llvm_type == NULL)
    {
        return result;
    }
    storage = __LLVM_Allocate_Stack__(emitter, llvm_type, "tagged.construct");
    if (storage == NULL)
    {
        __LLVM_Fail__("L2.5 tagged construction storage failed");
        return result;
    }
    LLVMBuildStore(emitter->builder, LLVMConstNull(llvm_type), storage);
    tag_address = LLVMBuildStructGEP2(emitter->builder, llvm_type, storage, 0U, "tagged.tag.place");
    LLVMBuildStore(emitter->builder,
                   LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U),
                                (unsigned long long)constructor_index,
                                0),
                   tag_address);

    for (payload_index = 0U; payload_index < argument_count; ++payload_index)
    {
        /* Stores the canonical offset. */
        size_t canonical_offset = 0U;
        /* Stores the payload size. */
        size_t payload_size = 0U;
        /* Stores the payload alignment. */
        size_t payload_alignment = 1U;
        /* References the payload type. */
        __Ast_Type__ *payload_type = NULL;
        /* Stores the payload LLVM type. */
        LLVMTypeRef payload_llvm_type;
        /* Stores the payload value. */
        __LLVM_Value__ payload_value;
        /* Stores the payload temp. */
        LLVMValueRef payload_temp;
        /* Stores the destination. */
        LLVMValueRef destination;
        /* Stores the byte count. */
        LLVMValueRef byte_count;

        if (!__Layout_Tagged_Payload__(emitter->semantic,
                                       target_type,
                                       constructor_index,
                                       payload_index,
                                       &canonical_offset,
                                       &payload_type) ||
            payload_type == NULL ||
            !__Layout_Type__(emitter->semantic, payload_type, &payload_size, &payload_alignment))
        {
            __LLVM_Fail__("L2.5 constructor payload lacks canonical static layout");
            return __LLVM_Invalid_Value__();
        }
        payload_value = __LLVM_Emit_Expression__(emitter, arguments[payload_index], payload_type);
        if (payload_value.value == NULL)
        {
            return __LLVM_Invalid_Value__();
        }
        payload_value = __LLVM_Coerce__(emitter, payload_value, payload_type);
        if (payload_value.value == NULL)
        {
            return __LLVM_Invalid_Value__();
        }
        payload_llvm_type = __LLVM_Type__(emitter, payload_type);
        if (payload_llvm_type == NULL)
        {
            return __LLVM_Invalid_Value__();
        }
        payload_temp = __LLVM_Allocate_Payload_Stage__(
            emitter, payload_llvm_type, payload_size, payload_alignment, "tagged.payload.temp");
        LLVMBuildStore(emitter->builder, payload_value.value, payload_temp);
        destination = __LLVM_Tagged_Byte_Address__(emitter,
                                                   target_type,
                                                   llvm_type,
                                                   storage,
                                                   canonical_offset,
                                                   payload_size,
                                                   "tagged.payload.place");
        if (destination == NULL)
        {
            return __LLVM_Invalid_Value__();
        }
        byte_count = LLVMConstInt(
            LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)payload_size, 0);
        if (LLVMBuildMemCpy(emitter->builder,
                            destination,
                            (unsigned)payload_alignment,
                            payload_temp,
                            (unsigned)payload_alignment,
                            byte_count) == NULL)
        {
            __LLVM_Fail__("L2.5 payload copy into tagged storage failed");
            return __LLVM_Invalid_Value__();
        }
    }
    (void)payload_capacity;
    result.value = LLVMBuildLoad2(emitter->builder, llvm_type, storage, "tagged.value");
    result.type = target_type;
    return result;
}

/* Returns the LLVM store tagged payload value. */
static int __LLVM_Store_Tagged_Payload_Value__(__LLVM_Emitter__ *emitter,
                                               __Ast_Type__ *tagged_type,
                                               LLVMTypeRef tagged_llvm_type,
                                               LLVMValueRef tagged_storage,
                                               size_t constructor_index,
                                               size_t payload_index,
                                               __LLVM_Value__ payload)
{
    /* Stores the canonical offset. */
    size_t canonical_offset = 0U;
    /* Stores the payload size. */
    size_t payload_size = 0U;
    /* Stores the payload alignment. */
    size_t payload_alignment = 1U;
    /* References the payload type. */
    __Ast_Type__ *payload_type = NULL;
    /* Stores the payload LLVM type. */
    LLVMTypeRef payload_llvm_type;
    /* Stores the payload temp. */
    LLVMValueRef payload_temp;
    /* Stores the destination. */
    LLVMValueRef destination;
    /* Stores the byte count. */
    LLVMValueRef byte_count;

    if (!__Layout_Tagged_Payload__(emitter->semantic,
                                   tagged_type,
                                   constructor_index,
                                   payload_index,
                                   &canonical_offset,
                                   &payload_type) ||
        payload_type == NULL ||
        !__Layout_Type__(emitter->semantic, payload_type, &payload_size, &payload_alignment))
        return __LLVM_Fail__("L2.8 runtime Result payload lacks canonical tagged layout");
    payload = __LLVM_Coerce__(emitter, payload, payload_type);
    if (payload.value == NULL)
        return 0;
    payload_llvm_type = __LLVM_Type__(emitter, payload_type);
    if (payload_llvm_type == NULL)
        return 0;
    payload_temp = __LLVM_Allocate_Payload_Stage__(
        emitter, payload_llvm_type, payload_size, payload_alignment, "runtime.result.payload.temp");
    if (payload_temp == NULL)
        return 0;
    LLVMBuildStore(emitter->builder, payload.value, payload_temp);
    destination = __LLVM_Tagged_Byte_Address__(emitter,
                                               tagged_type,
                                               tagged_llvm_type,
                                               tagged_storage,
                                               canonical_offset,
                                               payload_size,
                                               "runtime.result.payload.place");
    if (destination == NULL)
        return 0;
    byte_count = LLVMConstInt(
        LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)payload_size, 0);
    if (LLVMBuildMemCpy(emitter->builder,
                        destination,
                        (unsigned)payload_alignment,
                        payload_temp,
                        (unsigned)payload_alignment,
                        byte_count) == NULL)
        return __LLVM_Fail__("L2.8 runtime Result payload copy failed");
    return 1;
}

/* Writes the LLVM tagged arm. */
int __LLVM_Write_Tagged_Arm__(__LLVM_Emitter__ *emitter,
                                     __Ast_Type__ *tagged_type,
                                     LLVMTypeRef tagged_llvm_type,
                                     LLVMValueRef tagged_storage,
                                     size_t constructor_index,
                                     __LLVM_Value__ payload)
{
    /* Stores the tag address. */
    LLVMValueRef tag_address = LLVMBuildStructGEP2(
        emitter->builder, tagged_llvm_type, tagged_storage, 0U, "runtime.result.tag.place");
    if (tag_address == NULL)
        return __LLVM_Fail__("L2.8 runtime Result tag place failed");
    LLVMBuildStore(emitter->builder,
                   LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U),
                                (unsigned long long)constructor_index,
                                0),
                   tag_address);
    return __LLVM_Store_Tagged_Payload_Value__(
        emitter, tagged_type, tagged_llvm_type, tagged_storage, constructor_index, 0U, payload);
}

/* Copies the LLVM tagged payload to storage. */
LLVMValueRef __LLVM_Copy_Tagged_Payload_To_Storage__(__LLVM_Emitter__ *emitter,
                                                            __Ast_Type__ *tagged_type,
                                                            LLVMTypeRef llvm_tagged_type,
                                                            LLVMValueRef tagged_storage,
                                                            size_t constructor_index,
                                                            size_t payload_index,
                                                            __Ast_Type__ **out_payload_type,
                                                            LLVMTypeRef *out_payload_llvm_type)
{
    /* Stores the canonical offset. */
    size_t canonical_offset = 0U;
    /* Stores the payload size. */
    size_t payload_size = 0U;
    /* Stores the payload alignment. */
    size_t payload_alignment = 1U;
    /* References the payload type. */
    __Ast_Type__ *payload_type = NULL;
    /* Stores the payload LLVM type. */
    LLVMTypeRef payload_llvm_type;
    /* Stores the source. */
    LLVMValueRef source;
    /* Stores the destination. */
    LLVMValueRef destination;
    /* Stores the byte count. */
    LLVMValueRef byte_count;

    if (!__Layout_Tagged_Payload__(emitter->semantic,
                                   tagged_type,
                                   constructor_index,
                                   payload_index,
                                   &canonical_offset,
                                   &payload_type) ||
        payload_type == NULL ||
        !__Layout_Type__(emitter->semantic, payload_type, &payload_size, &payload_alignment))
    {
        __LLVM_Fail__("L2.5 matched payload lacks canonical layout");
        return NULL;
    }
    payload_llvm_type = __LLVM_Type__(emitter, payload_type);
    if (payload_llvm_type == NULL)
    {
        return NULL;
    }
    source = __LLVM_Tagged_Byte_Address__(emitter,
                                          tagged_type,
                                          llvm_tagged_type,
                                          tagged_storage,
                                          canonical_offset,
                                          payload_size,
                                          "match.payload.source");
    if (source == NULL)
    {
        return NULL;
    }
    destination = __LLVM_Allocate_Payload_Stage__(
        emitter, payload_llvm_type, payload_size, payload_alignment, "match.payload.value");
    byte_count = LLVMConstInt(
        LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)payload_size, 0);
    if (LLVMBuildMemCpy(emitter->builder,
                        destination,
                        (unsigned)payload_alignment,
                        source,
                        (unsigned)payload_alignment,
                        byte_count) == NULL)
    {
        __LLVM_Fail__("L2.5 payload extraction copy failed");
        return NULL;
    }
    if (out_payload_type != NULL)
        *out_payload_type = payload_type;
    if (out_payload_llvm_type != NULL)
        *out_payload_llvm_type = payload_llvm_type;
    return destination;
}

static int __LLVM_Is_Concrete_Aggregate_Entry__(const __Semantic_Type_Entry__ *entry)
{
    return entry != NULL && !entry->__Is_Generic_Template__ && entry->__Declaration__ != NULL &&
           (entry->__Declaration__->__Kind__ == __Ast_Type_Decl_Struct__ ||
            entry->__Declaration__->__Kind__ == __Ast_Type_Decl_Enum__);
}

/* Returns the LLVM predeclare aggregate types. */
int __LLVM_Predeclare_Aggregate_Types__(__LLVM_Emitter__ *emitter)
{
    size_t index;
    size_t count = 0U;
    for (index = 0U; index < emitter->semantic->__Types__.__Count__; ++index)
    {
        __Semantic_Type_Entry__ *entry =
            (__Semantic_Type_Entry__ *)__Vector_At__(&emitter->semantic->__Types__, index);
        if (__LLVM_Is_Concrete_Aggregate_Entry__(entry))
            ++count;
    }
    for (index = 0U; index < emitter->semantic->__Generic_Types__.__Count__; ++index)
    {
        __Semantic_Type_Entry__ **slot = (__Semantic_Type_Entry__ **)__Vector_At__(
            &emitter->semantic->__Generic_Types__, index);
        if (slot != NULL && __LLVM_Is_Concrete_Aggregate_Entry__(*slot))
            ++count;
    }
    emitter->aggregate_type_count = count;
    if (count == 0U)
        return 1;
    emitter->aggregate_types =
        (__LLVM_Aggregate_Type__ *)calloc(count, sizeof(*emitter->aggregate_types));
    if (emitter->aggregate_types == NULL)
        return __LLVM_Fail__("out of memory while predeclaring L2.5 aggregate Types");

    count = 0U;
    for (index = 0U; index < emitter->semantic->__Types__.__Count__; ++index)
    {
        __Semantic_Type_Entry__ *entry =
            (__Semantic_Type_Entry__ *)__Vector_At__(&emitter->semantic->__Types__, index);
        char symbol[80];
        if (!__LLVM_Is_Concrete_Aggregate_Entry__(entry))
            continue;
        snprintf(symbol, sizeof(symbol),
                 entry->__Declaration__->__Kind__ == __Ast_Type_Decl_Enum__
                     ? "sultanc.enum.%zu"
                     : "sultanc.record.%zu",
                 index);
        emitter->aggregate_types[count].semantic = entry;
        emitter->aggregate_types[count].type = LLVMStructCreateNamed(emitter->context, symbol);
        if (emitter->aggregate_types[count].type == NULL)
            return __LLVM_Fail__("LLVM named aggregate declaration failed for L2.5");
        ++count;
    }
    for (index = 0U; index < emitter->semantic->__Generic_Types__.__Count__; ++index)
    {
        __Semantic_Type_Entry__ **slot = (__Semantic_Type_Entry__ **)__Vector_At__(
            &emitter->semantic->__Generic_Types__, index);
        __Semantic_Type_Entry__ *entry = slot == NULL ? NULL : *slot;
        char symbol[80];
        if (!__LLVM_Is_Concrete_Aggregate_Entry__(entry))
            continue;
        snprintf(symbol, sizeof(symbol),
                 entry->__Declaration__->__Kind__ == __Ast_Type_Decl_Enum__
                     ? "sultanc.enum.generic.%zu"
                     : "sultanc.record.generic.%zu",
                 index);
        emitter->aggregate_types[count].semantic = entry;
        emitter->aggregate_types[count].type = LLVMStructCreateNamed(emitter->context, symbol);
        if (emitter->aggregate_types[count].type == NULL)
            return __LLVM_Fail__("LLVM generic aggregate declaration failed for L2.5");
        ++count;
    }
    return 1;
}


/* Ensures a concrete semantic aggregate has its canonical lazy layout. */
static int __LLVM_Ensure_Aggregate_Layout__(__LLVM_Emitter__ *emitter,
                                            __Semantic_Type_Entry__ *entry)
{
    __Ast_Type__ named_type;
    size_t size = 0U;
    size_t alignment = 1U;
    if (emitter == NULL || emitter->semantic == NULL || entry == NULL ||
        entry->__Declaration__ == NULL)
    {
        return 0;
    }
    if (entry->__Layout_State__ == 2)
    {
        return 1;
    }
    memset(&named_type, 0, sizeof(named_type));
    named_type.__Kind__ = __Ast_Type_Named__;
    named_type.__As__.__Named__.__Name__ = entry->__Name__;
    named_type.__As__.__Named__.__Arguments__ = entry->__Type_Arguments__;
    named_type.__As__.__Named__.__Argument_Count__ = entry->__Type_Argument_Count__;
    emitter->semantic->__Active_Unit__ = entry->__Unit__;
    return __Layout_Type__(emitter->semantic, &named_type, &size, &alignment) &&
           entry->__Layout_State__ == 2;
}

/* Returns the LLVM define aggregate types. */
int __LLVM_Define_Aggregate_Types__(__LLVM_Emitter__ *emitter)
{
    /* Tracks the aggregate index. */
    size_t aggregate_index;
    for (aggregate_index = 0U; aggregate_index < emitter->aggregate_type_count; ++aggregate_index)
    {
        /* References the aggregate. */
        __LLVM_Aggregate_Type__ *aggregate = &emitter->aggregate_types[aggregate_index];
        /* References the entry. */
        __Semantic_Type_Entry__ *entry = aggregate->semantic;
        /* References the declaration. */
        __Ast_Type_Declaration__ *declaration = entry != NULL ? entry->__Declaration__ : NULL;
        if (entry == NULL || declaration == NULL ||
            !__LLVM_Ensure_Aggregate_Layout__(emitter, entry))
        {
            return __LLVM_Fail__("L2.5 aggregate lacks canonical semantic layout");
        }
        emitter->semantic->__Active_Unit__ = entry->__Unit__;
        if (declaration->__Kind__ == __Ast_Type_Decl_Struct__)
        {
            /* References the field types. */
            LLVMTypeRef *field_types = NULL;
            /* Tracks the field index. */
            size_t field_index;
            if (declaration->__As__.__Struct__.__Count__ != 0U)
            {
                field_types = (LLVMTypeRef *)calloc(declaration->__As__.__Struct__.__Count__,
                                                    sizeof(*field_types));
                if (field_types == NULL)
                {
                    return __LLVM_Fail__("out of memory while defining L2.4 record Type");
                }
            }
            for (field_index = 0U; field_index < declaration->__As__.__Struct__.__Count__;
                 ++field_index)
            {
                /* References the field. */
                __Ast_Struct_Field__ *field =
                    &declaration->__As__.__Struct__.__Fields__[field_index];
                /* Stores the canonical offset. */
                size_t canonical_offset = 0U;
                /* References the canonical type. */
                __Ast_Type__ *canonical_type = NULL;
                if (!__Layout_Struct_Field__(emitter->semantic,
                                             entry,
                                             field->__Name__,
                                             &canonical_offset,
                                             &canonical_type) ||
                    canonical_type != field->__Slot__.__Type__)
                {
                    free(field_types);
                    return __LLVM_Fail__("L2.4 could not consume canonical struct field layout");
                }
                (void)canonical_offset;
                field_types[field_index] = __LLVM_Type__(emitter, canonical_type);
                if (field_types[field_index] == NULL)
                {
                    free(field_types);
                    return 0;
                }
            }
            LLVMStructSetBody(aggregate->type,
                              field_types,
                              (unsigned)declaration->__As__.__Struct__.__Count__,
                              0);
            free(field_types);
        }
        else if (declaration->__Kind__ == __Ast_Type_Decl_Enum__)
        {
            /* Stores the elements. */
            LLVMTypeRef elements[2];
            /* Stores the payload size. */
            size_t payload_size = 0U;
            if (!__LLVM_Tagged_Entry_Layout__(emitter, entry, &payload_size, NULL, NULL))
            {
                return 0;
            }
            elements[0] = LLVMIntTypeInContext(emitter->context, 64U);
            elements[1] =
                LLVMArrayType(LLVMIntTypeInContext(emitter->context, 8U), (unsigned)payload_size);
            LLVMStructSetBody(aggregate->type, elements, 2U, 0);
        }
        else
        {
            return __LLVM_Fail__("L2.5 unknown canonical aggregate declaration kind");
        }
    }
    return 1;
}

/* === Drop elaboration: recursive destruction of owned values. ===========
 * This extends the existing aggregate/tagged owner rather than adding a
 * parallel one: struct-field GEP indices mirror __LLVM_Define_Aggregate_Types__
 * exactly, and tagged-payload addressing reuses __LLVM_Tagged_Byte_Address__
 * (above) instead of recomputing offsets. Every entry point here is only
 * ever invoked where __Memory_Type_Is_Owned__ already held for the static
 * type in question (checked by the caller), so this file never re-derives
 * ownership classification. */

/* Returns the tagged constructor count for either a Named enum declaration
 * or a builtin tagged type (Option/Result), uniformly. */
static int __LLVM_Drop_Tagged_Constructor_Count__(__Ast_Type__ *base,
                                                   const __Resolved_Type__ *resolved,
                                                   size_t *out_count)
{
    if (__Type_Is_Builtin_Tagged__(base))
    {
        *out_count = __Type_Tagged_Constructor_Count__(base);
        return 1;
    }
    if (resolved->__Kind__ == __Resolved_Type_Enum__ && resolved->__Named__ != NULL &&
        resolved->__Named__->__Declaration__ != NULL &&
        resolved->__Named__->__Declaration__->__Kind__ == __Ast_Type_Decl_Enum__)
    {
        *out_count = resolved->__Named__->__Declaration__->__As__.__Enum__.__Count__;
        return 1;
    }
    return 0;
}

/* Returns the payload-slot count of one constructor, for either shape. */
static size_t __LLVM_Drop_Tagged_Payload_Count__(__Ast_Type__ *base,
                                                 const __Resolved_Type__ *resolved,
                                                 size_t constructor_index)
{
    if (__Type_Is_Builtin_Tagged__(base))
    {
        __Type_Tagged_Constructor__ constructor;
        if (!__Type_Tagged_Constructor_At__(base, constructor_index, &constructor))
            return 0U;
        return constructor.__Payload_Count__;
    }
    if (resolved->__Named__ != NULL && resolved->__Named__->__Declaration__ != NULL &&
        constructor_index < resolved->__Named__->__Declaration__->__As__.__Enum__.__Count__)
    {
        return resolved->__Named__->__Declaration__->__As__.__Enum__
            .__Constructors__[constructor_index]
            .__Payload_Count__;
    }
    return 0U;
}

static int __LLVM_Drop_Value_At__(__LLVM_Emitter__ *emitter, LLVMValueRef address, __Ast_Type__ *type);

/* Performs the actual struct-field-by-field or tagged-constructor-switch
 * release work for one Named/declaration-backed owned value -- the real
 * body shared by both the memoized drop-function path and the
 * no-canonical-key inline fallback in __LLVM_Drop_Value_At__. Declared and
 * defined ahead of __LLVM_Get_Or_Create_Named_Drop_Function__ (below),
 * which is the only place besides __LLVM_Drop_Value_At__ that calls it. */
static int __LLVM_Drop_Named_Body__(__LLVM_Emitter__ *emitter,
                                    LLVMValueRef address,
                                    __Ast_Type__ *type,
                                    LLVMTypeRef llvm_type,
                                    const __Resolved_Type__ *resolved);

/* Finds an already-registered drop function for a canonical Named-type
 * entry, or NULL if none exists yet. */
static __LLVM_Drop_Function__ *__LLVM_Find_Drop_Function__(__LLVM_Emitter__ *emitter,
                                                            __Semantic_Type_Entry__ *key)
{
    size_t index;
    for (index = 0U; index < emitter->drop_function_count; ++index)
    {
        if (emitter->drop_functions[index].key == key)
        {
            return &emitter->drop_functions[index];
        }
    }
    return NULL;
}

/* Registers a new drop function slot (function already created, body not
 * yet built) for a canonical Named-type entry. Mirrors the growth pattern
 * __LLVM_Add_Local__ uses for emitter->locals. */
static __LLVM_Drop_Function__ *__LLVM_Register_Drop_Function__(__LLVM_Emitter__ *emitter,
                                                                __Semantic_Type_Entry__ *key,
                                                                LLVMTypeRef llvm_type,
                                                                LLVMValueRef function)
{
    __LLVM_Drop_Function__ *entry;
    if (emitter->drop_function_count == emitter->drop_function_capacity)
    {
        size_t capacity = emitter->drop_function_capacity == 0U ? 8U : emitter->drop_function_capacity * 2U;
        __LLVM_Drop_Function__ *grown =
            (__LLVM_Drop_Function__ *)realloc(emitter->drop_functions, capacity * sizeof(*grown));
        if (grown == NULL)
        {
            __LLVM_Fail__("out of memory while recording L2 drop functions");
            return NULL;
        }
        emitter->drop_functions = grown;
        emitter->drop_function_capacity = capacity;
    }
    entry = &emitter->drop_functions[emitter->drop_function_count++];
    entry->key = key;
    entry->llvm_type = llvm_type;
    entry->function = function;
    entry->defined = 0;
    return entry;
}

/* Returns a callable drop function for one Named (Struct/Enum, or a
 * builtin-tagged Option/Result instance that does carry a canonical
 * entry) owned type, creating and memoizing it on first use. The function
 * is registered (forward-declared) in the emitter's registry *before*
 * its body is built, so a self- or mutually-recursive type -- a struct
 * holding `box` of itself, or two structs boxing each other -- resolves
 * the inner occurrence to this same forward declaration and emits an
 * ordinary recursive call instead of the C-level code generator
 * recursing into itself without bound. Only Named types need this: Box,
 * Vector, String and tagged dispatch stay fully inlined at every call
 * site (see __LLVM_Drop_Value_At__) because a type-level cycle can only
 * close by reaching a Named declaration again. */
static LLVMValueRef __LLVM_Get_Or_Create_Named_Drop_Function__(__LLVM_Emitter__ *emitter,
                                                                __Semantic_Type_Entry__ *key,
                                                                __Ast_Type__ *type,
                                                                LLVMTypeRef llvm_type)
{
    __LLVM_Drop_Function__ *existing;
    LLVMTypeRef pointer_type;
    LLVMTypeRef parameter_types[1];
    LLVMTypeRef function_type;
    LLVMValueRef function;
    LLVMBasicBlockRef saved_block;
    LLVMBasicBlockRef saved_allocation_block;
    LLVMBasicBlockRef entry_block;
    LLVMBasicBlockRef allocation_block;
    LLVMValueRef address;
    char function_name[64];
    static unsigned long long counter = 0ULL;

    existing = __LLVM_Find_Drop_Function__(emitter, key);
    if (existing != NULL)
    {
        return existing->function;
    }
    pointer_type = LLVMPointerType(llvm_type, 0U);
    parameter_types[0] = pointer_type;
    function_type = LLVMFunctionType(LLVMVoidTypeInContext(emitter->context), parameter_types, 1U, 0);
    (void)snprintf(function_name, sizeof(function_name), "__sultanc_l2_drop_%llu", ++counter);
    function = LLVMAddFunction(emitter->module, function_name, function_type);
    if (function == NULL)
    {
        __LLVM_Fail__("failed to create L2 named-type drop function");
        return NULL;
    }
    LLVMSetLinkage(function, LLVMInternalLinkage);
    /* Registered *before* the body is built: a recursive occurrence
     * reached while building the body below finds this entry and emits a
     * call, rather than recursing in C to build another one. Note: this
     * does NOT keep the returned __LLVM_Drop_Function__* live across the
     * body-building call below -- building the body can itself register
     * further (nested) drop functions, which can grow (realloc, and so
     * relocate) this same emitter->drop_functions array, which would
     * leave a pointer held from here dangling. Every later use re-looks
     * the entry up by key instead of reusing a saved pointer. */
    if (__LLVM_Register_Drop_Function__(emitter, key, llvm_type, function) == NULL)
    {
        return NULL;
    }

    saved_block = LLVMGetInsertBlock(emitter->builder);
    /* Every SultanC function has its own private allocation block/builder
     * context (set up by __LLVM_Emit_Function_Body__ for ordinary
     * functions), which is what __LLVM_Allocate_Stack__ targets. This new
     * drop function is itself a distinct LLVM function, so without the
     * same setup here, any alloca requested while building its body (for
     * instance __LLVM_Drop_Vector__'s loop-index alloca) would be inserted
     * into whatever OTHER function's allocation block happened to be
     * ambient when this drop function was created -- invalid cross-
     * function IR that no verifier here catches, and undefined behavior
     * at runtime. The caller's allocation context is saved and restored
     * exactly like emitter->builder's position is, below. */
    saved_allocation_block = emitter->allocation_block;
    allocation_block = LLVMAppendBasicBlockInContext(emitter->context, function, "stack.allocations");
    entry_block = LLVMAppendBasicBlockInContext(emitter->context, function, "entry");
    if (allocation_block == NULL || entry_block == NULL)
    {
        emitter->allocation_block = saved_allocation_block;
        __LLVM_Fail__("failed to create L2 named-type drop function entry block");
        return NULL;
    }
    emitter->allocation_block = allocation_block;
    LLVMPositionBuilderAtEnd(emitter->allocation_builder, allocation_block);
    LLVMPositionBuilderAtEnd(emitter->builder, entry_block);
    address = LLVMGetParam(function, 0U);
    {
        /* Re-resolves rather than threading the caller's __Resolved_Type__
         * through: the body is the actual struct-field/tagged-switch
         * work (__LLVM_Drop_Named_Body__), never the dispatcher
         * (__LLVM_Drop_Value_At__) -- calling the dispatcher here would
         * just re-discover this same now-registered function and emit a
         * call to itself with no base case, an infinite loop at runtime
         * instead of the unbounded C-level recursion this exists to fix. */
        __Resolved_Type__ body_resolved;
        if (!__Type_Resolve__(emitter->semantic, type, &body_resolved) ||
            !__LLVM_Drop_Named_Body__(emitter, address, type, llvm_type, &body_resolved))
        {
            emitter->allocation_block = saved_allocation_block;
            if (saved_block != NULL)
                LLVMPositionBuilderAtEnd(emitter->builder, saved_block);
            return NULL;
        }
    }
    LLVMBuildRetVoid(emitter->builder);
    {
        /* Re-looks the entry up rather than reusing a pointer saved
         * before the body was built, per the note above. */
        __LLVM_Drop_Function__ *completed = __LLVM_Find_Drop_Function__(emitter, key);
        if (completed != NULL)
        {
            completed->defined = 1;
        }
    }
    /* Terminates this function's own allocation block with a branch into
     * its entry block -- the same join __LLVM_Emit_Function_Body__ performs
     * for ordinary functions -- before restoring the caller's allocation
     * context. */
    if (LLVMGetBasicBlockTerminator(allocation_block) != NULL)
    {
        emitter->allocation_block = saved_allocation_block;
        if (saved_block != NULL)
            LLVMPositionBuilderAtEnd(emitter->builder, saved_block);
        __LLVM_Fail__("L2 named-type drop function allocation owner was terminated early");
        return NULL;
    }
    LLVMPositionBuilderAtEnd(emitter->allocation_builder, allocation_block);
    if (LLVMBuildBr(emitter->allocation_builder, entry_block) == NULL)
    {
        emitter->allocation_block = saved_allocation_block;
        if (saved_block != NULL)
            LLVMPositionBuilderAtEnd(emitter->builder, saved_block);
        __LLVM_Fail__("L2 named-type drop function could not connect allocations to body");
        return NULL;
    }
    /* Restores the caller's allocation context and the builder's
     * position, since building this function's body temporarily
     * repositioned both. */
    emitter->allocation_block = saved_allocation_block;
    if (saved_allocation_block != NULL)
        LLVMPositionBuilderAtEnd(emitter->allocation_builder, saved_allocation_block);
    if (saved_block != NULL)
        LLVMPositionBuilderAtEnd(emitter->builder, saved_block);
    return function;
}

/* Recursively drops every owned payload slot of one tagged constructor. */
static int __LLVM_Drop_Tagged_Constructor_Payloads__(__LLVM_Emitter__ *emitter,
                                                      __Ast_Type__ *tagged_type,
                                                      LLVMTypeRef llvm_tagged_type,
                                                      LLVMValueRef tagged_address,
                                                      size_t constructor_index,
                                                      size_t payload_count)
{
    size_t payload_index;
    for (payload_index = 0U; payload_index < payload_count; ++payload_index)
    {
        /* Stores the canonical offset. */
        size_t canonical_offset = 0U;
        /* References the payload type. */
        __Ast_Type__ *payload_type = NULL;
        /* Stores the payload size/alignment. */
        size_t payload_size = 0U;
        size_t payload_alignment = 1U;
        /* Stores the payload LLVM type. */
        LLVMTypeRef payload_llvm_type;
        /* Stores the raw byte-array address and the typed cast of it. */
        LLVMValueRef byte_address;
        LLVMValueRef typed_address;
        if (!__Layout_Tagged_Payload__(emitter->semantic,
                                       tagged_type,
                                       constructor_index,
                                       payload_index,
                                       &canonical_offset,
                                       &payload_type) ||
            payload_type == NULL ||
            !__Layout_Type__(emitter->semantic, payload_type, &payload_size, &payload_alignment))
        {
            return __LLVM_Fail__("L2 drop elaboration lost canonical tagged payload layout");
        }
        if (!__Memory_Type_Is_Owned__(emitter->semantic, payload_type))
        {
            continue;
        }
        payload_llvm_type = __LLVM_Type__(emitter, payload_type);
        if (payload_llvm_type == NULL)
        {
            return 0;
        }
        byte_address = __LLVM_Tagged_Byte_Address__(emitter,
                                                     tagged_type,
                                                     llvm_tagged_type,
                                                     tagged_address,
                                                     canonical_offset,
                                                     payload_size,
                                                     "drop.tagged.payload.bytes");
        if (byte_address == NULL)
        {
            return 0;
        }
        typed_address = LLVMBuildPointerCast(emitter->builder,
                                             byte_address,
                                             LLVMPointerType(payload_llvm_type, 0U),
                                             "drop.tagged.payload.typed");
        if (!__LLVM_Drop_Value_At__(emitter, typed_address, payload_type))
        {
            return 0;
        }
    }
    return 1;
}

/* Recursively drops a tagged (Named enum / Option / Result) value: switches
 * on the live tag and, for each constructor that actually carries an owned
 * payload, drops that payload. Constructors with nothing owned need no
 * case at all. */
static int __LLVM_Drop_Tagged__(__LLVM_Emitter__ *emitter,
                                LLVMValueRef address,
                                __Ast_Type__ *type,
                                LLVMTypeRef llvm_type,
                                const __Resolved_Type__ *resolved)
{
    __Ast_Type__ *base = __Type_Unwrap_Mutable__(type);
    size_t constructor_count = 0U;
    size_t constructor_index;
    LLVMValueRef tag_address;
    LLVMValueRef tag_value;
    LLVMBasicBlockRef current;
    LLVMValueRef function;
    LLVMBasicBlockRef merge_block;
    LLVMValueRef switch_instruction;
    size_t live_case_count = 0U;

    if (!__LLVM_Drop_Tagged_Constructor_Count__(base, resolved, &constructor_count))
    {
        return __LLVM_Fail__("L2 drop elaboration lost canonical tagged constructor shape");
    }
    /* Count constructors that actually own something; an all-borrowed
     * tagged value should never reach here (the caller already checked
     * __Memory_Type_Is_Owned__), but a defensive count keeps this correct
     * even if a future owning shape adds constructors with nothing owned. */
    for (constructor_index = 0U; constructor_index < constructor_count; ++constructor_index)
    {
        size_t payload_index;
        size_t payload_count = __LLVM_Drop_Tagged_Payload_Count__(base, resolved, constructor_index);
        for (payload_index = 0U; payload_index < payload_count; ++payload_index)
        {
            size_t canonical_offset = 0U;
            __Ast_Type__ *payload_type = NULL;
            if (__Layout_Tagged_Payload__(emitter->semantic,
                                          type,
                                          constructor_index,
                                          payload_index,
                                          &canonical_offset,
                                          &payload_type) &&
                payload_type != NULL && __Memory_Type_Is_Owned__(emitter->semantic, payload_type))
            {
                ++live_case_count;
                break;
            }
        }
    }
    if (live_case_count == 0U)
    {
        return 1;
    }
    current = LLVMGetInsertBlock(emitter->builder);
    if (current == NULL || (function = LLVMGetBasicBlockParent(current)) == NULL)
    {
        return __LLVM_Fail__("cannot locate function for L2 tagged drop elaboration");
    }
    tag_address = LLVMBuildStructGEP2(emitter->builder, llvm_type, address, 0U, "drop.tagged.tag.place");
    tag_value = LLVMBuildLoad2(
        emitter->builder, LLVMIntTypeInContext(emitter->context, 64U), tag_address, "drop.tagged.tag");
    merge_block = LLVMAppendBasicBlockInContext(emitter->context, function, "drop.tagged.end");
    switch_instruction = LLVMBuildSwitch(emitter->builder, tag_value, merge_block, (unsigned)live_case_count);
    for (constructor_index = 0U; constructor_index < constructor_count; ++constructor_index)
    {
        size_t payload_count = __LLVM_Drop_Tagged_Payload_Count__(base, resolved, constructor_index);
        size_t payload_index;
        int has_owned_payload = 0;
        for (payload_index = 0U; payload_index < payload_count; ++payload_index)
        {
            size_t canonical_offset = 0U;
            __Ast_Type__ *payload_type = NULL;
            if (__Layout_Tagged_Payload__(emitter->semantic,
                                          type,
                                          constructor_index,
                                          payload_index,
                                          &canonical_offset,
                                          &payload_type) &&
                payload_type != NULL && __Memory_Type_Is_Owned__(emitter->semantic, payload_type))
            {
                has_owned_payload = 1;
                break;
            }
        }
        if (has_owned_payload)
        {
            /* Stores the case block. */
            LLVMBasicBlockRef case_block = LLVMAppendBasicBlockInContext(
                emitter->context, function, "drop.tagged.case");
            LLVMAddCase(switch_instruction,
                       LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U),
                                    (unsigned long long)constructor_index,
                                    0),
                       case_block);
            LLVMPositionBuilderAtEnd(emitter->builder, case_block);
            if (!__LLVM_Drop_Tagged_Constructor_Payloads__(
                    emitter, type, llvm_type, address, constructor_index, payload_count))
            {
                return 0;
            }
            LLVMBuildBr(emitter->builder, merge_block);
        }
    }
    LLVMPositionBuilderAtEnd(emitter->builder, merge_block);
    return 1;
}

/* Recursively drops a Vector's elements (when owned) then releases its
 * heap-backed storage. */
/* Releases `pointer` only when `capacity` (an i64) is non-zero. Vector and
 * String share the existing capacity==0 convention (Initialize_Vector,
 * and the append/grow path's "append.capacity.zero" check in
 * llvm_builtins.c) for "nothing was ever heap-allocated here" -- a
 * string literal's data pointer is a non-NULL global address with
 * capacity fixed at 0, so checking capacity rather than just nullness is
 * required to never call free() on static/borrowed storage. */
static int __LLVM_Free_If_Capacity__(__LLVM_Emitter__ *emitter,
                                     LLVMValueRef pointer,
                                     LLVMValueRef capacity)
{
    LLVMBasicBlockRef current = LLVMGetInsertBlock(emitter->builder);
    LLVMValueRef function;
    LLVMBasicBlockRef free_block;
    LLVMBasicBlockRef merge_block;
    LLVMValueRef has_capacity;
    if (current == NULL || (function = LLVMGetBasicBlockParent(current)) == NULL)
    {
        return __LLVM_Fail__("cannot locate function for L2 capacity-gated drop elaboration");
    }
    has_capacity = LLVMBuildICmp(emitter->builder,
                                 LLVMIntNE,
                                 capacity,
                                 LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), 0U, 0),
                                 "drop.has.capacity");
    free_block = LLVMAppendBasicBlockInContext(emitter->context, function, "drop.free");
    merge_block = LLVMAppendBasicBlockInContext(emitter->context, function, "drop.free.end");
    if (free_block == NULL || merge_block == NULL)
    {
        return __LLVM_Fail__("L2 capacity-gated drop elaboration block creation failed");
    }
    LLVMBuildCondBr(emitter->builder, has_capacity, free_block, merge_block);
    LLVMPositionBuilderAtEnd(emitter->builder, free_block);
    if (!__LLVM_Free_Bytes__(emitter, pointer))
    {
        return 0;
    }
    LLVMBuildBr(emitter->builder, merge_block);
    LLVMPositionBuilderAtEnd(emitter->builder, merge_block);
    return 1;
}

static int __LLVM_Drop_Vector__(__LLVM_Emitter__ *emitter,
                                LLVMValueRef address,
                                LLVMTypeRef llvm_type,
                                __Ast_Type__ *element_type)
{
    LLVMTypeRef i64_type = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef element_llvm_type = __LLVM_Type__(emitter, element_type);
    LLVMTypeRef element_pointer_type;
    LLVMValueRef data_place;
    LLVMValueRef length_place;
    LLVMValueRef capacity_place;
    LLVMValueRef data;
    LLVMValueRef length;
    LLVMValueRef capacity;
    if (element_llvm_type == NULL)
    {
        return 0;
    }
    element_pointer_type = LLVMPointerType(element_llvm_type, 0U);
    data_place = LLVMBuildStructGEP2(emitter->builder, llvm_type, address, 0U, "drop.vector.data.place");
    length_place =
        LLVMBuildStructGEP2(emitter->builder, llvm_type, address, 1U, "drop.vector.length.place");
    capacity_place =
        LLVMBuildStructGEP2(emitter->builder, llvm_type, address, 2U, "drop.vector.capacity.place");
    data = LLVMBuildLoad2(emitter->builder, element_pointer_type, data_place, "drop.vector.data");
    length = LLVMBuildLoad2(emitter->builder, i64_type, length_place, "drop.vector.length");
    capacity = LLVMBuildLoad2(emitter->builder, i64_type, capacity_place, "drop.vector.capacity");
    if (__Memory_Type_Is_Owned__(emitter->semantic, element_type))
    {
        /* Stores the current/function. */
        LLVMBasicBlockRef current = LLVMGetInsertBlock(emitter->builder);
        LLVMValueRef function;
        LLVMBasicBlockRef condition_block;
        LLVMBasicBlockRef body_block;
        LLVMBasicBlockRef exit_block;
        LLVMValueRef index_address;
        LLVMValueRef condition;
        LLVMValueRef index_value;
        LLVMValueRef element_address;
        LLVMValueRef next_index;
        if (current == NULL || (function = LLVMGetBasicBlockParent(current)) == NULL)
        {
            return __LLVM_Fail__("cannot locate function for L2 vector drop elaboration");
        }
        index_address = __LLVM_Allocate_Stack__(emitter, i64_type, "drop.vector.index");
        if (index_address == NULL)
        {
            return 0;
        }
        LLVMBuildStore(emitter->builder, LLVMConstInt(i64_type, 0U, 0), index_address);
        condition_block =
            LLVMAppendBasicBlockInContext(emitter->context, function, "drop.vector.condition");
        body_block = LLVMAppendBasicBlockInContext(emitter->context, function, "drop.vector.body");
        exit_block = LLVMAppendBasicBlockInContext(emitter->context, function, "drop.vector.exit");
        LLVMBuildBr(emitter->builder, condition_block);

        LLVMPositionBuilderAtEnd(emitter->builder, condition_block);
        index_value = LLVMBuildLoad2(emitter->builder, i64_type, index_address, "drop.vector.index.load");
        condition = LLVMBuildICmp(emitter->builder, LLVMIntULT, index_value, length, "drop.vector.has.more");
        LLVMBuildCondBr(emitter->builder, condition, body_block, exit_block);

        LLVMPositionBuilderAtEnd(emitter->builder, body_block);
        element_address = LLVMBuildGEP2(
            emitter->builder, element_llvm_type, data, &index_value, 1U, "drop.vector.element");
        if (!__LLVM_Drop_Value_At__(emitter, element_address, element_type))
        {
            return 0;
        }
        next_index = LLVMBuildAdd(
            emitter->builder, index_value, LLVMConstInt(i64_type, 1U, 0), "drop.vector.index.next");
        LLVMBuildStore(emitter->builder, next_index, index_address);
        LLVMBuildBr(emitter->builder, condition_block);

        LLVMPositionBuilderAtEnd(emitter->builder, exit_block);
    }
    /* capacity == 0 is the existing, already-relied-on sentinel (see
     * Initialize_Vector and the append/grow path in llvm_builtins.c) for
     * "nothing was ever heap-allocated for this value" -- an empty vector
     * literal has data == NULL and capacity == 0, but a *non-empty* vector
     * literal/borrowed slice could have a non-NULL data pointing at
     * something that was never malloc'd (e.g. a global), so the guard
     * must check capacity, not just nullness, before releasing. */
    return __LLVM_Free_If_Capacity__(emitter, data, capacity);
}

/* Recursively drops the boxed inner value, then releases the box's heap
 * storage. */
static int __LLVM_Drop_Box__(__LLVM_Emitter__ *emitter,
                             LLVMValueRef address,
                             LLVMTypeRef llvm_type,
                             __Ast_Type__ *inner_type)
{
    /* Stores the boxed pointer. */
    LLVMValueRef pointer = LLVMBuildLoad2(emitter->builder, llvm_type, address, "drop.box.pointer");
    if (__Memory_Type_Is_Owned__(emitter->semantic, inner_type))
    {
        if (!__LLVM_Drop_Value_At__(emitter, pointer, inner_type))
        {
            return 0;
        }
    }
    return __LLVM_Free_Bytes__(emitter, pointer);
}

/* Struct-field-by-field or tagged-constructor-switch release work for one
 * Named/declaration-backed owned value; see the forward declaration above
 * for why this is split out from __LLVM_Drop_Value_At__. Nested owned
 * fields/payloads still recurse through __LLVM_Drop_Value_At__, so a
 * nested Named type is itself wrapped in the memoized call mechanism --
 * only this function's own two shapes (Struct, tagged) are exempt from
 * re-wrapping, since the wrapping already happened at the call site that
 * is invoking this body. */
static int __LLVM_Drop_Named_Body__(__LLVM_Emitter__ *emitter,
                                    LLVMValueRef address,
                                    __Ast_Type__ *type,
                                    LLVMTypeRef llvm_type,
                                    const __Resolved_Type__ *resolved)
{
    if (resolved->__Kind__ == __Resolved_Type_Struct__)
    {
        /* References the declaration. */
        __Ast_Type_Declaration__ *declaration =
            resolved->__Named__ != NULL ? resolved->__Named__->__Declaration__ : NULL;
        size_t field_index;
        if (declaration == NULL || declaration->__Kind__ != __Ast_Type_Decl_Struct__)
        {
            return __LLVM_Fail__("L2 drop elaboration requires a canonical struct declaration");
        }
        for (field_index = 0U; field_index < declaration->__As__.__Struct__.__Count__; ++field_index)
        {
            __Ast_Type__ *field_type =
                declaration->__As__.__Struct__.__Fields__[field_index].__Slot__.__Type__;
            if (__Memory_Type_Is_Owned__(emitter->semantic, field_type))
            {
                /* Stores the field address. */
                LLVMValueRef field_address = LLVMBuildStructGEP2(emitter->builder,
                                                                 llvm_type,
                                                                 address,
                                                                 (unsigned)field_index,
                                                                 "drop.struct.field");
                if (!__LLVM_Drop_Value_At__(emitter, field_address, field_type))
                {
                    return 0;
                }
            }
        }
        return 1;
    }
    /* __Resolved_Type_Enum__ / __Resolved_Type_Option__ / __Resolved_Type_Result__. */
    return __LLVM_Drop_Tagged__(emitter, address, type, llvm_type, resolved);
}

/* Recursively drops the value of static type `type` stored at `address`.
 * Callers only invoke this where __Memory_Type_Is_Owned__ already held, so
 * this never recomputes ownership classification; it only dispatches on
 * the already-known-owned shape. */
static int __LLVM_Drop_Value_At__(__LLVM_Emitter__ *emitter, LLVMValueRef address, __Ast_Type__ *type)
{
    __Resolved_Type__ resolved;
    LLVMTypeRef llvm_type;
    if (emitter == NULL || address == NULL || type == NULL)
    {
        return __LLVM_Fail__("L2 drop elaboration is missing canonical storage facts");
    }
    if (!__Type_Resolve__(emitter->semantic, type, &resolved))
    {
        return __LLVM_Fail__("L2 drop elaboration lost canonical resolved Type");
    }
    llvm_type = __LLVM_Type__(emitter, type);
    if (llvm_type == NULL)
    {
        return 0;
    }
    switch (resolved.__Kind__)
    {
        case __Resolved_Type_Vector__:
            return __LLVM_Drop_Vector__(emitter, address, llvm_type, resolved.__Inner__);
        case __Resolved_Type_String__:
        {
            /* Stores the i64 type. */
            LLVMTypeRef i64_type = LLVMIntTypeInContext(emitter->context, 64U);
            /* Stores the data place. */
            LLVMTypeRef byte_pointer_type = LLVMPointerType(LLVMIntTypeInContext(emitter->context, 8U), 0U);
            LLVMValueRef data_place =
                LLVMBuildStructGEP2(emitter->builder, llvm_type, address, 0U, "drop.string.data.place");
            LLVMValueRef capacity_place = LLVMBuildStructGEP2(
                emitter->builder, llvm_type, address, 2U, "drop.string.capacity.place");
            LLVMValueRef data =
                LLVMBuildLoad2(emitter->builder, byte_pointer_type, data_place, "drop.string.data");
            LLVMValueRef capacity =
                LLVMBuildLoad2(emitter->builder, i64_type, capacity_place, "drop.string.capacity");
            /* A string literal has capacity fixed at 0 and a non-NULL
             * global data pointer (see __LLVM_Emit_String_Literal__); the
             * capacity guard is what keeps that pointer away from free(). */
            return __LLVM_Free_If_Capacity__(emitter, data, capacity);
        }
        case __Resolved_Type_Box__:
            return __LLVM_Drop_Box__(emitter, address, llvm_type, resolved.__Inner__);
        case __Resolved_Type_Struct__:
        case __Resolved_Type_Enum__:
        case __Resolved_Type_Option__:
        case __Resolved_Type_Result__:
        {
            /* Named (and declaration-backed tagged) shapes are the only
             * ones that can introduce a type-level cycle (a struct
             * boxing itself, two enums boxing each other), so they alone
             * go through the memoized drop-function wrapper: get-or-
             * create a real LLVM function for this canonical declaration
             * and call it, rather than inlining its body here. A
             * recursive occurrence reached while that function's own
             * body is being built (see
             * __LLVM_Get_Or_Create_Named_Drop_Function__) finds the same
             * forward-declared function and becomes an ordinary call,
             * not another round of C-level inlining. Option/Result
             * instances without a canonical entry (resolved.__Named__
             * NULL) have no stable memoization key; they fall back to
             * being inlined directly here, which is safe because they
             * cannot by themselves be the declaration that closes a
             * cycle -- any cycle still bottoms out at a Named Struct/Enum
             * reached from their payload, and that reach is memoized. */
            if (resolved.__Named__ != NULL)
            {
                LLVMValueRef function = __LLVM_Get_Or_Create_Named_Drop_Function__(
                    emitter, resolved.__Named__, type, llvm_type);
                LLVMValueRef arguments[1];
                if (function == NULL)
                {
                    return 0;
                }
                arguments[0] = address;
                LLVMBuildCall2(emitter->builder,
                               LLVMGlobalGetValueType(function),
                               function,
                               arguments,
                               1U,
                               "");
                return 1;
            }
            return __LLVM_Drop_Named_Body__(emitter, address, type, llvm_type, &resolved);
        }
        default:
            /* Scalars, references, pointers, functions: nothing owned to
             * release. Callers only reach non-owned shapes defensively
             * (e.g. a struct field that is not itself owned is skipped
             * before calling this at all), so this is never hit for a
             * type __Memory_Type_Is_Owned__ called false on. */
            return 1;
    }
}

/* Releases an owned local or temporary value, given its storage address and
 * static type. The only entry point this file exposes to the rest of LLVM
 * lowering; keeps the recursive dispatch above private. */
int __LLVM_Drop_Owned_At__(__LLVM_Emitter__ *emitter, LLVMValueRef address, __Ast_Type__ *type)
{
    return __LLVM_Drop_Value_At__(emitter, address, type);
}
