/* Owns Stage0 LLVM aggregate and tagged-value representation. */

#include "llvm/llvm_aggregates.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "kernel/layout/layout.h"
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
        payload_temp = __LLVM_Allocate_Stack__(emitter, payload_llvm_type, "tagged.payload.temp");
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
    payload_temp =
        __LLVM_Allocate_Stack__(emitter, payload_llvm_type, "runtime.result.payload.temp");
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
    destination = __LLVM_Allocate_Stack__(emitter, payload_llvm_type, "match.payload.value");
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

/* Returns the LLVM predeclare aggregate types. */
int __LLVM_Predeclare_Aggregate_Types__(__LLVM_Emitter__ *emitter)
{
    /* Tracks the index. */
    size_t index;
    /* Stores the count. */
    size_t count = 0U;
    for (index = 0U; index < emitter->semantic->__Types__.__Count__; ++index)
    {
        /* References the entry. */
        __Semantic_Type_Entry__ *entry =
            (__Semantic_Type_Entry__ *)__Vector_At__(&emitter->semantic->__Types__, index);
        if (entry != NULL && entry->__Declaration__ != NULL &&
            (entry->__Declaration__->__Kind__ == __Ast_Type_Decl_Struct__ ||
             entry->__Declaration__->__Kind__ == __Ast_Type_Decl_Enum__))
        {
            ++count;
        }
    }
    emitter->aggregate_type_count = count;
    if (count == 0U)
    {
        return 1;
    }
    emitter->aggregate_types =
        (__LLVM_Aggregate_Type__ *)calloc(count, sizeof(*emitter->aggregate_types));
    if (emitter->aggregate_types == NULL)
    {
        return __LLVM_Fail__("out of memory while predeclaring L2.5 aggregate Types");
    }
    count = 0U;
    for (index = 0U; index < emitter->semantic->__Types__.__Count__; ++index)
    {
        /* References the entry. */
        __Semantic_Type_Entry__ *entry =
            (__Semantic_Type_Entry__ *)__Vector_At__(&emitter->semantic->__Types__, index);
        /* Stores the symbol. */
        char symbol[64];
        if (entry == NULL || entry->__Declaration__ == NULL ||
            (entry->__Declaration__->__Kind__ != __Ast_Type_Decl_Struct__ &&
             entry->__Declaration__->__Kind__ != __Ast_Type_Decl_Enum__))
        {
            continue;
        }
        snprintf(symbol,
                 sizeof(symbol),
                 entry->__Declaration__->__Kind__ == __Ast_Type_Decl_Enum__ ? "sultanc.enum.%zu"
                                                                            : "sultanc.record.%zu",
                 index);
        emitter->aggregate_types[count].semantic = entry;
        emitter->aggregate_types[count].type = LLVMStructCreateNamed(emitter->context, symbol);
        if (emitter->aggregate_types[count].type == NULL)
        {
            return __LLVM_Fail__("LLVM named aggregate declaration failed for L2.5");
        }
        ++count;
    }
    return 1;
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
        if (entry == NULL || declaration == NULL || entry->__Layout_State__ != 2)
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
            /* Stores the named type. */
            __Ast_Type__ named_type;
            /* Stores the elements. */
            LLVMTypeRef elements[2];
            /* Stores the payload size. */
            size_t payload_size = 0U;
            memset(&named_type, 0, sizeof(named_type));
            named_type.__Kind__ = __Ast_Type_Named__;
            named_type.__As__.__Named__.__Name__ = entry->__Name__;
            if (!__LLVM_Tagged_Layout__(emitter, &named_type, &payload_size, NULL, NULL))
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
