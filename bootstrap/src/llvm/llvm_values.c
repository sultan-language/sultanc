/* Owns Stage0 LLVM locals, places, loads, stores, and coercions. */

#include "llvm/llvm_values.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_types.h"
#include "kernel/layout/layout.h"
#include "kernel/type/conversion.h"
#include "kernel/type/type.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Allocates the LLVM stack. */
LLVMValueRef
__LLVM_Allocate_Stack__(__LLVM_Emitter__ *emitter, LLVMTypeRef type, const char *name)
{
    /* Stores the value. */
    LLVMValueRef value;
    if (emitter == NULL || emitter->allocation_builder == NULL ||
        emitter->allocation_block == NULL || type == NULL)
    {
        __LLVM_Fail__("Direct LLVM stack allocation has no active function-entry owner");
        return NULL;
    }
    value = LLVMBuildAlloca(emitter->allocation_builder, type, name);
    if (value == NULL)
    {
        __LLVM_Fail__("Direct LLVM function-entry stack allocation failed");
    }
    return value;
}

/* Compares the LLVM text values. */
LLVMValueRef __LLVM_Compare_Text_Values__(__LLVM_Emitter__ *emitter,
                                                 LLVMValueRef left,
                                                 LLVMValueRef right,
                                                 __Ast_Binary_Operation__ operation)
{
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the left data. */
    LLVMValueRef left_data =
        LLVMBuildExtractValue(emitter->builder, left, 0U, "text.cmp.left.data");
    /* Stores the right data. */
    LLVMValueRef right_data =
        LLVMBuildExtractValue(emitter->builder, right, 0U, "text.cmp.right.data");
    /* Stores the left length. */
    LLVMValueRef left_length =
        LLVMBuildExtractValue(emitter->builder, left, 1U, "text.cmp.left.length");
    /* Stores the right length. */
    LLVMValueRef right_length =
        LLVMBuildExtractValue(emitter->builder, right, 1U, "text.cmp.right.length");
    /* Tracks whether the left is shorter. */
    LLVMValueRef left_shorter = LLVMBuildICmp(
        emitter->builder, LLVMIntULT, left_length, right_length, "text.cmp.left.shorter");
    /* Stores the common length. */
    LLVMValueRef common_length = LLVMBuildSelect(
        emitter->builder, left_shorter, left_length, right_length, "text.cmp.common.length");
    /* Stores the call arguments. */
    LLVMValueRef args[3];
    /* Stores the comparison result. */
    LLVMValueRef compare;
    /* Stores the zero i32 constant. */
    LLVMValueRef zero32 = LLVMConstInt(i32, 0U, 0);
    /* Tracks whether the prefix is equal. */
    LLVMValueRef prefix_equal;
    /* Tracks whether the length is equal. */
    LLVMValueRef length_equal;

    (void)i64;
    if (emitter->memcmp_function == NULL)
    {
        /* Stores the parameter values. */
        LLVMTypeRef params[3];
        params[0] = i8_pointer;
        params[1] = i8_pointer;
        params[2] = LLVMIntTypeInContext(emitter->context, 64U);
        emitter->memcmp_type = LLVMFunctionType(i32, params, 3U, 0);
        emitter->memcmp_function = LLVMGetNamedFunction(emitter->module, "memcmp");
        if (emitter->memcmp_function == NULL)
            emitter->memcmp_function =
                LLVMAddFunction(emitter->module, "memcmp", emitter->memcmp_type);
        if (emitter->memcmp_function == NULL)
        {
            __LLVM_Fail__("L2.7 could not declare bytewise text comparison boundary");
            return NULL;
        }
    }
    args[0] = LLVMBuildPointerCast(emitter->builder, left_data, i8_pointer, "text.cmp.left.bytes");
    args[1] =
        LLVMBuildPointerCast(emitter->builder, right_data, i8_pointer, "text.cmp.right.bytes");
    args[2] = common_length;
    compare = LLVMBuildCall2(emitter->builder,
                             emitter->memcmp_type,
                             emitter->memcmp_function,
                             args,
                             3U,
                             "text.cmp.bytes");
    if (compare == NULL)
        return NULL;
    prefix_equal =
        LLVMBuildICmp(emitter->builder, LLVMIntEQ, compare, zero32, "text.cmp.prefix.equal");
    length_equal = LLVMBuildICmp(
        emitter->builder, LLVMIntEQ, left_length, right_length, "text.cmp.length.equal");

    switch (operation)
    {
        case __Binary_Equal__:
            return LLVMBuildAnd(emitter->builder, prefix_equal, length_equal, "text.equal");
        case __Binary_Not_Equal__:
        {
            /* Tracks whether the prefix differs. */
            LLVMValueRef prefix_not_equal = LLVMBuildICmp(
                emitter->builder, LLVMIntNE, compare, zero32, "text.cmp.prefix.not.equal");
            /* Tracks whether the length differs. */
            LLVMValueRef length_not_equal = LLVMBuildICmp(emitter->builder,
                                                          LLVMIntNE,
                                                          left_length,
                                                          right_length,
                                                          "text.cmp.length.not.equal");
            return LLVMBuildOr(
                emitter->builder, prefix_not_equal, length_not_equal, "text.not.equal");
        }
        case __Binary_Less_Than__:
        case __Binary_Less_Or_Equal__:
        case __Binary_Greater_Than__:
        case __Binary_Greater_Or_Equal__:
        {
            /* Stores the strict predicate. */
            LLVMIntPredicate strict_predicate =
                (operation == __Binary_Less_Than__ || operation == __Binary_Less_Or_Equal__)
                    ? LLVMIntSLT
                    : LLVMIntSGT;
            /* Stores the length predicate. */
            LLVMIntPredicate length_predicate;
            /* Stores the prefix strict. */
            LLVMValueRef prefix_strict = LLVMBuildICmp(
                emitter->builder, strict_predicate, compare, zero32, "text.cmp.prefix.order");
            /* Stores the length order. */
            LLVMValueRef length_order;
            /* Stores the equal prefix length order. */
            LLVMValueRef equal_prefix_length_order;
            if (operation == __Binary_Less_Than__)
                length_predicate = LLVMIntULT;
            else if (operation == __Binary_Less_Or_Equal__)
                length_predicate = LLVMIntULE;
            else if (operation == __Binary_Greater_Than__)
                length_predicate = LLVMIntUGT;
            else
                length_predicate = LLVMIntUGE;
            length_order = LLVMBuildICmp(emitter->builder,
                                         length_predicate,
                                         left_length,
                                         right_length,
                                         "text.cmp.length.order");
            equal_prefix_length_order = LLVMBuildAnd(
                emitter->builder, prefix_equal, length_order, "text.cmp.equal.prefix.length.order");
            return LLVMBuildOr(
                emitter->builder, prefix_strict, equal_prefix_length_order, "text.cmp.order");
        }
        default:
            __LLVM_Fail__("L2.7 text comparison received a non-comparison operation");
            return NULL;
    }
}

/* Emits the LLVM string literal. */
__LLVM_Value__
__LLVM_Emit_String_Literal__(__LLVM_Emitter__ *emitter, __Text_Slice__ text, __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* References the type. */
    __Ast_Type__ *type = expected != NULL ? expected : &__LLVM_String_Type__;
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    /* Stores the string type. */
    LLVMTypeRef string_type;
    /* Stores the byte type. */
    LLVMTypeRef byte_type;
    /* Stores the array type. */
    LLVMTypeRef array_type;
    /* Stores the global. */
    LLVMValueRef global;
    /* Stores the initializer. */
    LLVMValueRef initializer;
    /* Stores the indices. */
    LLVMValueRef indices[2];
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the value. */
    LLVMValueRef value;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the name. */
    char name[64];
    /* Stores the array length. */
    unsigned array_length;

    if (!__Type_Resolve__(emitter->semantic, type, &resolved) ||
        resolved.__Kind__ != __Resolved_Type_String__ || text.__Length__ > (size_t)UINT_MAX)
    {
        __LLVM_Fail__("L2.7 string literal requires canonical text Type");
        return result;
    }
    string_type = __LLVM_Type__(emitter, type);
    byte_type = LLVMIntTypeInContext(emitter->context, 8U);
    if (string_type == NULL)
    {
        return result;
    }
    array_length = text.__Length__ == 0U ? 1U : (unsigned)text.__Length__;
    array_type = LLVMArrayType(byte_type, array_length);
    snprintf(name, sizeof(name), "sultanc.text.%zu", emitter->string_literal_count++);
    global = LLVMAddGlobal(emitter->module, array_type, name);
    if (global == NULL)
    {
        __LLVM_Fail__("L2.7 could not materialize text literal data");
        return result;
    }
    if (text.__Length__ == 0U)
    {
        initializer = LLVMConstNull(array_type);
    }
    else
    {
        initializer =
            LLVMConstStringInContext(emitter->context, text.__Data__, (unsigned)text.__Length__, 1);
    }
    LLVMSetInitializer(global, initializer);
    LLVMSetGlobalConstant(global, 1);
    indices[0] = LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), 0U, 0);
    indices[1] = LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), 0U, 0);
    data = LLVMBuildGEP2(emitter->builder, array_type, global, indices, 2U, "text.data");
    length = LLVMConstInt(
        LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)text.__Length__, 0);
    value = LLVMConstNull(string_type);
    value = LLVMBuildInsertValue(emitter->builder, value, data, 0U, "text.with.data");
    value = LLVMBuildInsertValue(emitter->builder, value, length, 1U, "text.with.length");
    value = LLVMBuildInsertValue(emitter->builder,
                                 value,
                                 LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), 0U, 0),
                                 2U,
                                 "text.with.capacity");
    result.value = value;
    result.type = type;
    return result;
}

/* Finds the LLVM local. */
__LLVM_Local__ *__LLVM_Find_Local__(__LLVM_Emitter__ *emitter, __Text_Slice__ name)
{
    /* Tracks the index. */
    size_t index;
    for (index = emitter->local_count; index > 0U; --index)
    {
        /* References the local. */
        __LLVM_Local__ *local = &emitter->locals[index - 1U];
        if (local->name_kind == __Ast_Lvalue_Base_Identifier__ &&
            __LLVM_Text_Equals__(local->name, name))
        {
            return local;
        }
    }
    return NULL;
}

/* Finds the LLVM local base. */
static __LLVM_Local__ *__LLVM_Find_Local_Base__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue)
{
    /* Tracks the index. */
    size_t index;
    if (lvalue == NULL || lvalue->__Kind__ != __Ast_Lvalue_Base__)
        return NULL;
    for (index = emitter->local_count; index > 0U; --index)
    {
        /* References the local. */
        __LLVM_Local__ *local = &emitter->locals[index - 1U];
        if (local->name_kind != lvalue->__As__.__Base__.__Kind__)
            continue;
        if (local->name_kind == __Ast_Lvalue_Base_Identifier__)
        {
            if (__LLVM_Text_Equals__(local->name, lvalue->__As__.__Base__.__As__.__Identifier__))
                return local;
        }
        else if (local->temporary == lvalue->__As__.__Base__.__As__.__Temporary__)
        {
            return local;
        }
    }
    return NULL;
}

/* Adds the LLVM local identity. */
__LLVM_Local__ *__LLVM_Add_Local_Identity__(__LLVM_Emitter__ *emitter,
                                                   __Ast_Lvalue_Base_Kind__ name_kind,
                                                   __Text_Slice__ name,
                                                   __Temporary_Id__ temporary,
                                                   __Ast_Type__ *type)
{
    /* References the local. */
    __LLVM_Local__ *local;
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    /* Stores the LLVM type. */
    LLVMTypeRef llvm_type = __LLVM_Type__(emitter, type);
    if (llvm_type == NULL || !__Type_Resolve__(emitter->semantic, type, &resolved))
    {
        return NULL;
    }
    if (emitter->local_count == emitter->local_capacity)
    {
        /* Stores the capacity. */
        size_t capacity = emitter->local_capacity == 0U ? 8U : emitter->local_capacity * 2U;
        /* References the grown. */
        __LLVM_Local__ *grown =
            (__LLVM_Local__ *)realloc(emitter->locals, capacity * sizeof(*grown));
        if (grown == NULL)
        {
            __LLVM_Fail__("out of memory while recording LLVM locals");
            return NULL;
        }
        emitter->locals = grown;
        emitter->local_capacity = capacity;
    }
    local = &emitter->locals[emitter->local_count++];
    memset(local, 0, sizeof(*local));
    local->name_kind = name_kind;
    local->name = name;
    local->temporary = temporary;
    local->type = type;
    local->llvm_type = llvm_type;
    if (resolved.__Kind__ != __Resolved_Type_Void__)
    {
        local->address = __LLVM_Allocate_Stack__(emitter, llvm_type, "local");
    }
    return local;
}

/* Adds the LLVM local. */
__LLVM_Local__ *
__LLVM_Add_Local__(__LLVM_Emitter__ *emitter, __Text_Slice__ name, __Ast_Type__ *type)
{
    return __LLVM_Add_Local_Identity__(emitter, __Ast_Lvalue_Base_Identifier__, name, 0U, type);
}

/* Returns the LLVM invalid value. */
__LLVM_Value__ __LLVM_Invalid_Value__(void)
{
    /* Stores the value. */
    __LLVM_Value__ value;
    memset(&value, 0, sizeof(value));
    return value;
}

/* Returns the LLVM coerce. */
__LLVM_Value__
__LLVM_Coerce__(__LLVM_Emitter__ *emitter, __LLVM_Value__ source, __Ast_Type__ *target)
{
    /* Stores the source bits. */
    unsigned source_bits, target_bits;
    /* Tracks whether the source is signed. */
    int source_signed, target_signed;
    /* Stores the target type. */
    LLVMTypeRef target_type;
    /* Stores the operation result. */
    __LLVM_Value__ result = source;
    if (source.value == NULL || source.type == NULL || target == NULL)
    {
        return __LLVM_Invalid_Value__();
    }
    if (__Type_Compatible__(emitter->semantic, source.type, target))
    {
        result.type = target;
        return result;
    }
    {
        /* Stores the source resolved. */
        __Resolved_Type__ source_resolved;
        /* Stores the target resolved. */
        __Resolved_Type__ target_resolved;
        /* Stores the conversion. */
        __Type_Conversion_Class__ conversion =
            __Type_Conversion_Classify__(emitter->semantic, source.type, target);
        if (__Type_Resolve__(emitter->semantic, source.type, &source_resolved) &&
            __Type_Resolve__(emitter->semantic, target, &target_resolved) &&
            source_resolved.__Kind__ == __Resolved_Type_Reference__ &&
            target_resolved.__Kind__ == __Resolved_Type_Reference__ &&
            conversion == __Type_Conversion_Implicit_Safe__)
        {
            /* Stores the target pointer. */
            LLVMTypeRef target_pointer = __LLVM_Type__(emitter, target);
            if (target_pointer == NULL)
                return __LLVM_Invalid_Value__();
            result.value = LLVMBuildPointerCast(
                emitter->builder, source.value, target_pointer, "reference.coerce");
            result.type = target;
            return result;
        }
    }
    if (!__LLVM_Resolve_Integer__(emitter, source.type, &source_bits, &source_signed) ||
        !__LLVM_Resolve_Integer__(emitter, target, &target_bits, &target_signed))
    {
        __LLVM_Fail__("Direct LLVM conversion is outside canonical representable conversions");
        return __LLVM_Invalid_Value__();
    }
    (void)target_signed;
    target_type = LLVMIntTypeInContext(emitter->context, target_bits);
    if (source_bits < target_bits)
    {
        result.value = source_signed
                           ? LLVMBuildSExt(emitter->builder, source.value, target_type, "sext")
                           : LLVMBuildZExt(emitter->builder, source.value, target_type, "zext");
    }
    else if (source_bits > target_bits)
    {
        result.value = LLVMBuildTrunc(emitter->builder, source.value, target_type, "trunc");
    }
    result.type = target;
    return result;
}

/* Returns the LLVM lvalue type. */
__Ast_Type__ *__LLVM_Lvalue_Type__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue)
{
    /* References the local. */
    __LLVM_Local__ *local;
    /* References the parent type. */
    __Ast_Type__ *parent_type;
    /* References the field type. */
    __Ast_Type__ *field_type = NULL;
    if (lvalue == NULL)
    {
        return NULL;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Base__)
    {
        local = __LLVM_Find_Local_Base__(emitter, lvalue);
        return local != NULL ? local->type : NULL;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Field__)
    {
        parent_type = __LLVM_Lvalue_Type__(emitter, lvalue->__As__.__Field__.__Parent__);
        if (parent_type == NULL ||
            !__LLVM_Resolve_Struct_Field__(
                emitter, parent_type, lvalue->__As__.__Field__.__Field__, NULL, NULL, &field_type))
        {
            return NULL;
        }
        return field_type;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Dereference__)
    {
        /* Stores the resolved. */
        __Resolved_Type__ resolved;
        parent_type = __LLVM_Lvalue_Type__(emitter, lvalue->__As__.__Dereference_Parent__);
        if (parent_type == NULL || !__Type_Resolve__(emitter->semantic, parent_type, &resolved) ||
            (resolved.__Kind__ != __Resolved_Type_Reference__ &&
             resolved.__Kind__ != __Resolved_Type_Box__))
        {
            return NULL;
        }
        return resolved.__Inner__;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Index__)
    {
        /* Stores the resolved. */
        __Resolved_Type__ resolved;
        parent_type = __LLVM_Lvalue_Type__(emitter, lvalue->__As__.__Index__.__Parent__);
        if (parent_type == NULL || !__Type_Resolve__(emitter->semantic, parent_type, &resolved))
        {
            return NULL;
        }
        if (resolved.__Kind__ == __Resolved_Type_String__)
            return &__LLVM_U8_Type__;
        if (resolved.__Kind__ == __Resolved_Type_Vector__ ||
            resolved.__Kind__ == __Resolved_Type_Box__)
            return resolved.__Inner__;
    }
    return NULL;
}

/* Returns the LLVM invalid place. */
static __LLVM_Place__ __LLVM_Invalid_Place__(void)
{
    /* Stores the place. */
    __LLVM_Place__ place;
    memset(&place, 0, sizeof(place));
    return place;
}

/* Emits the LLVM place. */
__LLVM_Place__ __LLVM_Emit_Place__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue)
{
    /* Stores the place. */
    __LLVM_Place__ place = __LLVM_Invalid_Place__();
    if (lvalue == NULL)
    {
        __LLVM_Fail__("missing L2.4 aggregate place");
        return place;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Base__)
    {
        /* References the local. */
        __LLVM_Local__ *local = __LLVM_Find_Local_Base__(emitter, lvalue);
        if (local == NULL || local->address == NULL)
        {
            __LLVM_Fail__("L2.7 could not find value-producing local storage");
            return place;
        }
        place.address = local->address;
        place.llvm_type = local->llvm_type;
        place.type = local->type;
        return place;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Field__)
    {
        /* Stores the parent. */
        __LLVM_Place__ parent = __LLVM_Emit_Place__(emitter, lvalue->__As__.__Field__.__Parent__);
        /* Tracks the field index. */
        size_t field_index = 0U;
        /* Stores the field offset. */
        size_t field_offset = 0U;
        /* References the field type. */
        __Ast_Type__ *field_type = NULL;
        /* Stores the field LLVM type. */
        LLVMTypeRef field_llvm_type;
        if (parent.address == NULL || parent.type == NULL ||
            !__LLVM_Resolve_Struct_Field__(emitter,
                                           parent.type,
                                           lvalue->__As__.__Field__.__Field__,
                                           &field_index,
                                           &field_offset,
                                           &field_type))
        {
            __LLVM_Fail__("L2.4 field place is not a canonical struct field");
            return __LLVM_Invalid_Place__();
        }
        (void)field_offset; /* LLVM GEP uses the resolved static member index. */
        field_llvm_type = __LLVM_Type__(emitter, field_type);
        if (field_llvm_type == NULL)
        {
            return __LLVM_Invalid_Place__();
        }
        place.address = LLVMBuildStructGEP2(emitter->builder,
                                            parent.llvm_type,
                                            parent.address,
                                            (unsigned)field_index,
                                            "field.place");
        place.llvm_type = field_llvm_type;
        place.type = field_type;
        return place;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Dereference__)
    {
        /* Stores the pointer. */
        __LLVM_Value__ pointer =
            __LLVM_Emit_Lvalue__(emitter, lvalue->__As__.__Dereference_Parent__);
        /* Stores the resolved. */
        __Resolved_Type__ resolved;
        /* Stores the inner type. */
        LLVMTypeRef inner_type;
        if (pointer.value == NULL || pointer.type == NULL ||
            !__Type_Resolve__(emitter->semantic, pointer.type, &resolved) ||
            (resolved.__Kind__ != __Resolved_Type_Reference__ &&
             resolved.__Kind__ != __Resolved_Type_Box__) ||
            resolved.__Inner__ == NULL)
        {
            __LLVM_Fail__("L2.6 dereference requires canonical reference/box Type");
            return place;
        }
        inner_type = __LLVM_Type__(emitter, resolved.__Inner__);
        if (inner_type == NULL)
            return place;
        place.address = pointer.value;
        place.llvm_type = inner_type;
        place.type = resolved.__Inner__;
        return place;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Index__)
    {
        /* Stores the parent. */
        __LLVM_Place__ parent = __LLVM_Emit_Place__(emitter, lvalue->__As__.__Index__.__Parent__);
        /* Stores the resolved. */
        __Resolved_Type__ resolved;
        /* References the element type. */
        __Ast_Type__ *element_type = NULL;
        /* Stores the element LLVM type. */
        LLVMTypeRef element_llvm_type;
        /* Tracks the index. */
        __LLVM_Value__ index;
        /* Stores the index 64. */
        LLVMValueRef index64;
        /* Stores the base. */
        LLVMValueRef base = NULL;
        if (parent.address == NULL || parent.type == NULL ||
            !__Type_Resolve__(emitter->semantic, parent.type, &resolved))
        {
            __LLVM_Fail__("L2.7 index parent has no canonical Type");
            return place;
        }
        index = __LLVM_Emit_Atom__(
            emitter, &lvalue->__As__.__Index__.__Index__, &__LLVM_Integer_Type__);
        if (index.value == NULL)
            return place;
        index = __LLVM_Coerce__(emitter, index, &__LLVM_Integer_Type__);
        if (index.value == NULL)
            return place;
        index64 = index.value;

        if (resolved.__Kind__ == __Resolved_Type_Vector__ ||
            resolved.__Kind__ == __Resolved_Type_String__)
        {
            /* Stores the data place. */
            LLVMValueRef data_place;
            /* Stores the length place. */
            LLVMValueRef length_place;
            /* Stores the length. */
            LLVMValueRef length;
            /* Stores the negative. */
            LLVMValueRef negative;
            /* Stores the too large. */
            LLVMValueRef too_large;
            /* Stores the invalid. */
            LLVMValueRef invalid;
            /* Stores the data offset. */
            size_t data_offset = resolved.__Kind__ == __Resolved_Type_Vector__
                                     ? __Layout_Vector_Data_Offset__()
                                     : __Layout_String_Data_Offset__();
            /* Stores the length offset. */
            size_t length_offset = resolved.__Kind__ == __Resolved_Type_Vector__
                                       ? __Layout_Vector_Length_Offset__()
                                       : __Layout_String_Length_Offset__();
            if (data_offset != 0U || length_offset != 8U)
            {
                __LLVM_Fail__("L2.7 canonical sequence projection is not representable");
                return place;
            }
            data_place = LLVMBuildStructGEP2(
                emitter->builder, parent.llvm_type, parent.address, 0U, "sequence.data.place");
            length_place = LLVMBuildStructGEP2(
                emitter->builder, parent.llvm_type, parent.address, 1U, "sequence.length.place");
            base = LLVMBuildLoad2(
                emitter->builder,
                resolved.__Kind__ == __Resolved_Type_String__
                    ? LLVMPointerType(LLVMIntTypeInContext(emitter->context, 8U), 0U)
                    : LLVMPointerType(__LLVM_Type__(emitter, resolved.__Inner__), 0U),
                data_place,
                "sequence.data");
            length = LLVMBuildLoad2(emitter->builder,
                                    LLVMIntTypeInContext(emitter->context, 64U),
                                    length_place,
                                    "sequence.length");
            negative =
                LLVMBuildICmp(emitter->builder,
                              LLVMIntSLT,
                              index64,
                              LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), 0U, 0),
                              "sequence.index.negative");
            too_large = LLVMBuildICmp(
                emitter->builder, LLVMIntUGE, index64, length, "sequence.index.out.of.range");
            invalid = LLVMBuildOr(emitter->builder, negative, too_large, "sequence.index.invalid");
            if (!__LLVM_Emit_Trap_If__(emitter, invalid, "sequence.bounds.failure"))
                return __LLVM_Invalid_Place__();
            element_type = resolved.__Kind__ == __Resolved_Type_String__ ? &__LLVM_U8_Type__
                                                                         : resolved.__Inner__;
        }
        else if (resolved.__Kind__ == __Resolved_Type_Box__)
        {
            base = LLVMBuildLoad2(emitter->builder, parent.llvm_type, parent.address, "box.data");
            element_type = resolved.__Inner__;
        }
        else
        {
            __LLVM_Fail__("L2.7 indexing requires canonical vector/text/box Type");
            return place;
        }
        element_llvm_type = __LLVM_Type__(emitter, element_type);
        if (base == NULL || element_llvm_type == NULL)
            return place;
        place.address = LLVMBuildGEP2(
            emitter->builder, element_llvm_type, base, &index64, 1U, "sequence.element.place");
        place.llvm_type = element_llvm_type;
        place.type = element_type;
        return place;
    }
    __LLVM_Fail__("place is outside the accepted L2.1-L2.7 profile");
    return place;
}

/* Emits the LLVM lvalue. */
__LLVM_Value__ __LLVM_Emit_Lvalue__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue)
{
    /* Stores the place. */
    __LLVM_Place__ place = __LLVM_Emit_Place__(emitter, lvalue);
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    if (place.address == NULL || place.llvm_type == NULL || place.type == NULL)
    {
        return result;
    }
    result.value = LLVMBuildLoad2(emitter->builder, place.llvm_type, place.address, "load");
    result.type = place.type;
    return result;
}

/* Returns the LLVM make text value. */
__LLVM_Value__ __LLVM_Make_Text_Value__(__LLVM_Emitter__ *emitter,
                                               LLVMValueRef data,
                                               LLVMValueRef length,
                                               LLVMValueRef capacity,
                                               __Ast_Type__ *target_type)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* References the type. */
    __Ast_Type__ *type = target_type != NULL ? target_type : &__LLVM_String_Type__;
    /* Stores the LLVM type. */
    LLVMTypeRef llvm_type = __LLVM_Type__(emitter, type);
    /* Stores the value. */
    LLVMValueRef value;
    if (llvm_type == NULL || data == NULL || length == NULL || capacity == NULL)
        return result;
    value = LLVMConstNull(llvm_type);
    value = LLVMBuildInsertValue(emitter->builder, value, data, 0U, "runtime.text.data");
    value = LLVMBuildInsertValue(emitter->builder, value, length, 1U, "runtime.text.length");
    value = LLVMBuildInsertValue(emitter->builder, value, capacity, 2U, "runtime.text.capacity");
    result.value = value;
    result.type = type;
    return result;
}
