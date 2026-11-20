/* Owns Stage0 LLVM builtin lowering helpers. */

#include "llvm/llvm_builtins.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "llvm/llvm_aggregates.h"
#include "llvm/runtime/llvm_runtime_memory.h"
#include "kernel/layout/layout.h"
#include "kernel/memory/memory.h"
#include "kernel/name/name.h"
#include "kernel/type/tagged.h"
#include "kernel/type/type.h"
#include "semantic/body_internal.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Returns the LLVM bootstrap target identity code embedded in generated code. */
static int __LLVM_Bootstrap_Host_Identity_Code__(__LLVM_Emitter__ *emitter,
                                                 __Name_Builtin_Function__ builtin,
                                                 unsigned long long *out)
{
    if (emitter == NULL || out == NULL)
        return 0;
    switch (builtin)
    {
        case __Name_Builtin_Host_Architecture__:
            if (emitter->target->architecture == __Bootstrap_Target_Architecture_AArch64__)
            {
                *out = 1ULL;
                return 1;
            }
            if (emitter->target->architecture == __Bootstrap_Target_Architecture_X86_64__)
            {
                *out = 2ULL;
                return 1;
            }
            if (emitter->target->architecture == __Bootstrap_Target_Architecture_Wasm32__)
            {
                *out = 3ULL;
                return 1;
            }
            if (emitter->target->architecture == __Bootstrap_Target_Architecture_Wasm64__)
            {
                *out = 4ULL;
                return 1;
            }
            return __LLVM_Fail__(
                "requested Bootstrap target architecture has no Sultan host-identity code");
        case __Name_Builtin_Host_Platform__:
            if (emitter->target->platform == __Bootstrap_Target_Platform_Darwin__)
            {
                *out = 1ULL;
                return 1;
            }
            if (emitter->target->platform == __Bootstrap_Target_Platform_Linux__)
            {
                *out = 2ULL;
                return 1;
            }
            if (emitter->target->platform == __Bootstrap_Target_Platform_Wasi__)
            {
                *out = 3ULL;
                return 1;
            }
            if (emitter->target->platform == __Bootstrap_Target_Platform_Windows__)
            {
                *out = 5ULL;
                return 1;
            }
            return __LLVM_Fail__(
                "requested Bootstrap target platform has no Sultan host-identity code");
        case __Name_Builtin_Host_Environment__:
            if (emitter->target->environment == __Bootstrap_Target_Environment_Unknown__)
            {
                *out = 0ULL;
                return 1;
            }
            if (emitter->target->environment == __Bootstrap_Target_Environment_GNU__)
            {
                *out = 1ULL;
                return 1;
            }
            if (emitter->target->environment == __Bootstrap_Target_Environment_Musl__)
            {
                *out = 2ULL;
                return 1;
            }
            if (emitter->target->environment == __Bootstrap_Target_Environment_MSVC__)
            {
                *out = 3ULL;
                return 1;
            }
            return __LLVM_Fail__(
                "requested Bootstrap target environment has no Sultan host-identity code");
        default:
            return __LLVM_Fail__("non-host builtin used for Bootstrap host identity");
    }
}

/* Lowers the queried bootstrap target identity. */
static __LLVM_Value__ __LLVM_Emit_Host_Identity__(__LLVM_Emitter__ *emitter,
                                                  __Name_Builtin_Function__ builtin,
                                                  __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the code. */
    unsigned long long code = 0ULL;
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64;
    if (!__LLVM_Bootstrap_Host_Identity_Code__(emitter, builtin, &code))
        return result;
    i64 = LLVMIntTypeInContext(emitter->context, 64U);
    result.value = LLVMConstInt(i64, code, 0);
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}


/* Returns the LLVM builtin call identity. */
__Name_Builtin_Function__ __LLVM_Builtin_Call_Identity__(__LLVM_Emitter__ *emitter,
                                                                __Ast_Expression__ *expression)
{
    /* References the function. */
    __Ast_Lvalue__ *function;
    /* Stores the name. */
    __Text_Slice__ name;
    /* Stores the resolved name. */
    __Text_Slice__ resolved_name;
    /* Stores the direct. */
    __Name_Builtin_Function__ direct;
    /* References the unit. */
    const __Program_Unit__ *unit;
    if (expression == NULL || expression->__Kind__ != __Ast_Expression_Call__)
        return __Name_Builtin_None__;
    function = expression->__As__.__Call__.__Function__;
    if (function == NULL || function->__Kind__ != __Ast_Lvalue_Base__ ||
        function->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__)
        return __Name_Builtin_None__;
    name = function->__As__.__Base__.__As__.__Identifier__;
    if (__LLVM_Find_Local__(emitter, name) != NULL)
        return __Name_Builtin_None__;

    direct = __Name_Find_Builtin_Function__(name);
    if (direct != __Name_Builtin_None__ && __Name_Builtin_Is_Direct_Source__(direct))
        return direct;

    unit = (emitter->current_function == NULL) ? NULL : emitter->current_function->__Unit__;
    resolved_name = name;
    if (unit != NULL)
        (void)__Name_Resolve_Module_Alias_Name__(unit, name, &resolved_name);

    direct = __Name_Find_Builtin_Function__(resolved_name);
    if (direct != __Name_Builtin_None__)
        return direct;

    return __Name_Builtin_None__;
}


/* A reader primitive (length, capacity) of an owned member of a call result
 * receives the member moved out of the temporary: it is destroyed after the
 * read, as Rust destroys the temporary at the end of the statement. */
static int __LLVM_Drop_Read_Temporary_Member__(__LLVM_Emitter__ *emitter,
                                               __Ast_Expression__ *argument,
                                               __LLVM_Value__ value)
{
    __Ast_Lvalue__ *lvalue;
    LLVMTypeRef llvm_type;
    LLVMValueRef storage;
    if (argument == NULL || argument->__Kind__ != __Ast_Expression_Atom__ ||
        argument->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
        (lvalue = argument->__As__.__Atom__.__As__.__Lvalue__) == NULL ||
        lvalue->__Kind__ != __Ast_Lvalue_Field__ || lvalue->__As__.__Field__.__Parent__ == NULL ||
        lvalue->__As__.__Field__.__Parent__->__Kind__ != __Ast_Lvalue_Base__ ||
        lvalue->__As__.__Field__.__Parent__->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Call__ ||
        value.type == NULL || !__Memory_Type_Is_Owned__(emitter->semantic, value.type))
        return 1;
    llvm_type = __LLVM_Type__(emitter, value.type);
    if (llvm_type == NULL)
        return 0;
    storage = __LLVM_Allocate_Stack__(emitter, llvm_type, "temporary.member");
    LLVMBuildStore(emitter->builder, value.value, storage);
    return __LLVM_Drop_Owned_At__(emitter, storage, value.type);
}

/* Resolves the vector place a writing primitive changes: a place, or `*r`
 * through a `&متغير`. */
static int __LLVM_Builtin_Vector_Place__(__LLVM_Emitter__ *emitter,
                                         __Ast_Expression__ *vector_expression,
                                         __LLVM_Place__ *out_place,
                                         __Resolved_Type__ *out_resolved)
{
    memset(out_place, 0, sizeof(*out_place));
    if (vector_expression == NULL)
        return __LLVM_Fail__("vector primitive has no vector operand");
    if (vector_expression->__Kind__ == __Ast_Expression_Atom__ &&
        vector_expression->__As__.__Atom__.__Kind__ == __Ast_Atom_Lvalue__ &&
        vector_expression->__As__.__Atom__.__As__.__Lvalue__ != NULL)
    {
        *out_place = __LLVM_Emit_Place__(emitter, vector_expression->__As__.__Atom__.__As__.__Lvalue__);
    }
    else if (vector_expression->__Kind__ == __Ast_Expression_Unary__ &&
             vector_expression->__As__.__Unary__.__Operation__ == __Unary_Dereference__)
    {
        __LLVM_Value__ reference =
            __LLVM_Emit_Expression__(emitter, vector_expression->__As__.__Unary__.__Operand__, NULL);
        __Resolved_Type__ reference_resolved;
        if (reference.value == NULL || reference.type == NULL ||
            !__Type_Resolve__(emitter->semantic, reference.type, &reference_resolved) ||
            reference_resolved.__Kind__ != __Resolved_Type_Reference__ ||
            reference_resolved.__Inner__ == NULL)
            return __LLVM_Fail__("vector primitive dereference requires a mutable reference");
        out_place->address = reference.value;
        out_place->type = reference_resolved.__Inner__;
        out_place->llvm_type = __LLVM_Type__(emitter, out_place->type);
    }
    else
    {
        return __LLVM_Fail__("vector primitive requires a mutable vector place");
    }
    if (out_place->address == NULL || out_place->type == NULL || out_place->llvm_type == NULL ||
        !__Type_Resolve__(emitter->semantic, out_place->type, out_resolved) ||
        out_resolved->__Kind__ != __Resolved_Type_Vector__ || out_resolved->__Inner__ == NULL)
        return __LLVM_Fail__("vector primitive requires canonical vector storage");
    return 1;
}

/* Grows the buffer of a vector so that it holds at least length + ADDITIONAL
 * elements, allocating exactly that many when it grows (Rust's
 * Vec::reserve_exact): a length overflow traps; DATA and LENGTH are the
 * loaded header fields, DATA_FIELD and CAPACITY_FIELD their addresses. The
 * builder is left in the block after the growth. */
static int __LLVM_Reserve_Vector__(__LLVM_Emitter__ *emitter,
                                   LLVMValueRef function,
                                   LLVMValueRef data_field,
                                   LLVMValueRef capacity_field,
                                   LLVMValueRef data,
                                   LLVMValueRef length,
                                   LLVMValueRef additional,
                                   LLVMTypeRef element_type,
                                   size_t element_size)
{
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMValueRef capacity = LLVMBuildLoad2(emitter->builder, i64, capacity_field, "vector.capacity");
    LLVMValueRef required, overflow, grow, bytes_overflow, bytes, grown;
    LLVMBasicBlockRef grow_block, done_block;
    required = LLVMBuildAdd(emitter->builder, length, additional, "reserve.required");
    overflow = LLVMBuildICmp(emitter->builder, LLVMIntULT, required, length, "reserve.overflow");
    if (!__LLVM_Emit_Trap_If__(emitter, overflow, "vector.length.overflow"))
        return 0;
    grow = LLVMBuildICmp(emitter->builder, LLVMIntUGT, required, capacity, "reserve.grow");
    grow_block = LLVMAppendBasicBlockInContext(emitter->context, function, "reserve.grow");
    done_block = LLVMAppendBasicBlockInContext(emitter->context, function, "reserve.done");
    LLVMBuildCondBr(emitter->builder, grow, grow_block, done_block);
    LLVMPositionBuilderAtEnd(emitter->builder, grow_block);
    bytes_overflow = LLVMBuildICmp(emitter->builder, LLVMIntUGT, required,
                                   LLVMConstInt(i64, UINT64_MAX / (unsigned long long)element_size, 0),
                                   "reserve.bytes.overflow");
    if (!__LLVM_Emit_Trap_If__(emitter, bytes_overflow, "vector.backing.bytes.overflow"))
        return 0;
    bytes = LLVMBuildMul(emitter->builder, required, LLVMConstInt(i64, (unsigned long long)element_size, 0),
                         "reserve.bytes");
    grown = __LLVM_Grow_Sequence_Bytes__(
        emitter, data, capacity,
        LLVMBuildMul(emitter->builder, length, LLVMConstInt(i64, (unsigned long long)element_size, 0),
                     "reserve.live.bytes"),
        bytes, LLVMPointerType(element_type, 0U), "reserve.data");
    if (grown == NULL)
        return 0;
    LLVMBuildStore(emitter->builder, grown, data_field);
    LLVMBuildStore(emitter->builder, required, capacity_field);
    LLVMBuildBr(emitter->builder, done_block);
    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    return 1;
}

/* Emits extend_from (ألحق_من), as the self-hosted LLVM provider does
 * (اخفض_إلحاق_نطاق_متجه_إل_إل_في_إم): the source range is checked (start +
 * count must not wrap and must not pass the source length, else trap), the
 * vector grows once through reserve's lowering, then the range is copied by
 * a loop of element loads and stores (a variable-length memcpy would be a
 * libc call, and SultanC programs link none) and the length grows by count.
 * The source is a reference to a vector header or a text header; both have
 * the {data, length, capacity} layout. */
static __LLVM_Value__ __LLVM_Emit_Extend_From__(__LLVM_Emitter__ *emitter, __Ast_Expression__ *expression)
{
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    __Ast_Expression__ **arguments = expression->__As__.__Call__.__Arguments__;
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMValueRef function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    __LLVM_Place__ place;
    __Resolved_Type__ resolved;
    __Resolved_Type__ source_resolved;
    __Ast_Type__ *source_type;
    LLVMTypeRef element_type;
    LLVMTypeRef source_header_type;
    LLVMValueRef data_field, length_field, capacity_field, data, length;
    LLVMValueRef source_length, end, source_data, destination, origin, counter, index;
    LLVMBasicBlockRef test_block, body_block, end_block;
    __LLVM_Value__ source, start, count;
    size_t element_size = 0U, element_alignment = 1U;
    if (expression->__As__.__Call__.__Argument_Count__ != 4U || function == NULL ||
        !__LLVM_Builtin_Vector_Place__(emitter, arguments[0], &place, &resolved) ||
        (element_type = __LLVM_Type__(emitter, resolved.__Inner__)) == NULL ||
        !__Layout_Type__(emitter->semantic, resolved.__Inner__, &element_size, &element_alignment) ||
        element_size == 0U || (source_type = arguments[1]->__Contextual_Type__) == NULL ||
        !__Type_Resolve__(emitter->semantic, source_type, &source_resolved) ||
        source_resolved.__Kind__ != __Resolved_Type_Reference__ || source_resolved.__Inner__ == NULL ||
        (source_header_type = __LLVM_Type__(emitter, source_resolved.__Inner__)) == NULL)
    {
        __LLVM_Fail__("extend_from has no verified vector and source operands");
        return result;
    }
    (void)element_alignment;
    source = __LLVM_Emit_Expression__(emitter, arguments[1], source_type);
    if (source.value == NULL)
        return result;
    start = __LLVM_Emit_Expression__(emitter, arguments[2], &__Body_Builtin_Uint_Type__);
    if (start.value == NULL || (start = __LLVM_Coerce__(emitter, start, &__Body_Builtin_Uint_Type__)).value == NULL)
        return result;
    count = __LLVM_Emit_Expression__(emitter, arguments[3], &__Body_Builtin_Uint_Type__);
    if (count.value == NULL || (count = __LLVM_Coerce__(emitter, count, &__Body_Builtin_Uint_Type__)).value == NULL)
        return result;
    /* The source range: start + count (a wrap traps) <= source length. */
    source_length = LLVMBuildLoad2(
        emitter->builder, i64,
        LLVMBuildStructGEP2(emitter->builder, source_header_type, source.value, 1U, "extend.source.length.field"),
        "extend.source.length");
    end = LLVMBuildAdd(emitter->builder, start.value, count.value, "extend.end");
    if (!__LLVM_Emit_Trap_If__(emitter, LLVMBuildICmp(emitter->builder, LLVMIntULT, end, start.value, "extend.wrap"),
                               "extend.range.wrap") ||
        !__LLVM_Emit_Trap_If__(emitter, LLVMBuildICmp(emitter->builder, LLVMIntUGT, end, source_length, "extend.past"),
                               "extend.range.past.end"))
        return result;
    /* Grow once: reserve's lowering, with the count as its additional elements. */
    data_field = LLVMBuildStructGEP2(emitter->builder, place.llvm_type, place.address, 0U, "vector.data.field");
    length_field = LLVMBuildStructGEP2(emitter->builder, place.llvm_type, place.address, 1U, "vector.length.field");
    capacity_field = LLVMBuildStructGEP2(emitter->builder, place.llvm_type, place.address, 2U, "vector.capacity.field");
    data = LLVMBuildLoad2(emitter->builder, LLVMPointerType(element_type, 0U), data_field, "vector.data");
    length = LLVMBuildLoad2(emitter->builder, i64, length_field, "vector.length");
    if (!__LLVM_Reserve_Vector__(emitter, function, data_field, capacity_field, data, length, count.value,
                                 element_type, element_size))
        return result;
    /* The buffers after the growth (the source may be the vector itself). */
    data = LLVMBuildLoad2(emitter->builder, LLVMPointerType(element_type, 0U), data_field, "extend.data");
    source_data = LLVMBuildLoad2(
        emitter->builder, LLVMPointerType(element_type, 0U),
        LLVMBuildStructGEP2(emitter->builder, source_header_type, source.value, 0U, "extend.source.data.field"),
        "extend.source.data");
    destination = LLVMBuildGEP2(emitter->builder, element_type, data, &length, 1U, "extend.destination");
    origin = LLVMBuildGEP2(emitter->builder, element_type, source_data, &start.value, 1U, "extend.origin");
    /* for (i = 0; i < count; i++) destination[i] = origin[i]; */
    counter = __LLVM_Allocate_Stack__(emitter, i64, "extend.index");
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, 0U, 0), counter);
    test_block = LLVMAppendBasicBlockInContext(emitter->context, function, "extend.test");
    body_block = LLVMAppendBasicBlockInContext(emitter->context, function, "extend.body");
    end_block = LLVMAppendBasicBlockInContext(emitter->context, function, "extend.end");
    LLVMBuildBr(emitter->builder, test_block);
    LLVMPositionBuilderAtEnd(emitter->builder, test_block);
    index = LLVMBuildLoad2(emitter->builder, i64, counter, "extend.i");
    LLVMBuildCondBr(emitter->builder, LLVMBuildICmp(emitter->builder, LLVMIntULT, index, count.value, "extend.more"),
                    body_block, end_block);
    LLVMPositionBuilderAtEnd(emitter->builder, body_block);
    LLVMBuildStore(
        emitter->builder,
        LLVMBuildLoad2(emitter->builder, element_type,
                       LLVMBuildGEP2(emitter->builder, element_type, origin, &index, 1U, "extend.read"),
                       "extend.element"),
        LLVMBuildGEP2(emitter->builder, element_type, destination, &index, 1U, "extend.write"));
    LLVMBuildStore(emitter->builder, LLVMBuildAdd(emitter->builder, index, LLVMConstInt(i64, 1U, 0), "extend.next"),
                   counter);
    LLVMBuildBr(emitter->builder, test_block);
    LLVMPositionBuilderAtEnd(emitter->builder, end_block);
    LLVMBuildStore(emitter->builder, LLVMBuildAdd(emitter->builder, length, count.value, "extend.length"),
                   length_field);
    result.value = LLVMConstNull(i64);
    result.type = &__LLVM_Void_Type__;
    return result;
}

/* Emits capacity, reserve_exact, pop and truncate on the vector header
 * {data, length, capacity} (docs/en/reference/ownership.md). */
static __LLVM_Value__ __LLVM_Emit_Vector_Primitive__(__LLVM_Emitter__ *emitter,
                                                    __Ast_Expression__ *expression,
                                                    __Ast_Type__ *expected,
                                                    __Name_Builtin_Function__ builtin)
{
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    __LLVM_Place__ place;
    __Resolved_Type__ resolved;
    LLVMTypeRef element_type;
    LLVMValueRef data_field, length_field, capacity_field, data, length;
    LLVMValueRef function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    size_t element_size = 0U, element_alignment = 1U;

    if (builtin == __Name_Builtin_Capacity__)
    {
        __Ast_Type__ *argument_type = __LLVM_Expression_Type__(
            emitter, expression->__As__.__Call__.__Arguments__[0]);
        __LLVM_Value__ sequence;
        if (argument_type == NULL)
        {
            __LLVM_Fail__("capacity has no verified operand type");
            return result;
        }
        sequence = __LLVM_Emit_Expression__(emitter, expression->__As__.__Call__.__Arguments__[0],
                                            argument_type);
        if (sequence.value == NULL)
            return result;
        result.value = LLVMBuildExtractValue(emitter->builder, sequence.value, 2U, "sequence.capacity");
        if (!__LLVM_Drop_Read_Temporary_Member__(emitter, expression->__As__.__Call__.__Arguments__[0],
                                                 sequence))
            return __LLVM_Invalid_Value__();
        result.type = &__LLVM_Integer_Type__;
        return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
    }
    if (!__LLVM_Builtin_Vector_Place__(emitter, expression->__As__.__Call__.__Arguments__[0], &place,
                                       &resolved) ||
        (element_type = __LLVM_Type__(emitter, resolved.__Inner__)) == NULL ||
        !__Layout_Type__(emitter->semantic, resolved.__Inner__, &element_size, &element_alignment) ||
        function == NULL)
    {
        __LLVM_Fail__("vector primitive lacks canonical element layout");
        return result;
    }
    data_field = LLVMBuildStructGEP2(emitter->builder, place.llvm_type, place.address, 0U, "vector.data.field");
    length_field = LLVMBuildStructGEP2(emitter->builder, place.llvm_type, place.address, 1U, "vector.length.field");
    capacity_field = LLVMBuildStructGEP2(emitter->builder, place.llvm_type, place.address, 2U, "vector.capacity.field");
    data = LLVMBuildLoad2(emitter->builder, LLVMPointerType(element_type, 0U), data_field, "vector.data");
    length = LLVMBuildLoad2(emitter->builder, i64, length_field, "vector.length");

    if (builtin == __Name_Builtin_Reserve_Exact__)
    {
        __LLVM_Value__ additional = __LLVM_Emit_Expression__(
            emitter, expression->__As__.__Call__.__Arguments__[1], &__Body_Builtin_Uint_Type__);
        if (additional.value == NULL)
            return result;
        additional = __LLVM_Coerce__(emitter, additional, &__Body_Builtin_Uint_Type__);
        if (additional.value == NULL)
            return result;
        if (!__LLVM_Reserve_Vector__(emitter, function, data_field, capacity_field, data, length,
                                     additional.value, element_type, element_size))
            return result;
        result.value = LLVMConstNull(i64);
        result.type = &__LLVM_Void_Type__;
        return result;
    }
    if (builtin == __Name_Builtin_Truncate__)
    {
        __LLVM_Value__ keep = __LLVM_Emit_Expression__(
            emitter, expression->__As__.__Call__.__Arguments__[1], &__Body_Builtin_Uint_Type__);
        LLVMValueRef shorter;
        LLVMBasicBlockRef drop_block, done_block;
        if (keep.value == NULL)
            return result;
        keep = __LLVM_Coerce__(emitter, keep, &__Body_Builtin_Uint_Type__);
        if (keep.value == NULL)
            return result;
        shorter = LLVMBuildICmp(emitter->builder, LLVMIntULT, keep.value, length, "truncate.shorter");
        drop_block = LLVMAppendBasicBlockInContext(emitter->context, function, "truncate.drop");
        done_block = LLVMAppendBasicBlockInContext(emitter->context, function, "truncate.done");
        LLVMBuildCondBr(emitter->builder, shorter, drop_block, done_block);
        LLVMPositionBuilderAtEnd(emitter->builder, drop_block);
        if (__Memory_Type_Is_Owned__(emitter->semantic, resolved.__Inner__))
        {
            /* Drops the removed elements, first to last. */
            LLVMValueRef counter = __LLVM_Allocate_Stack__(emitter, i64, "truncate.index");
            LLVMBasicBlockRef test_block = LLVMAppendBasicBlockInContext(emitter->context, function, "truncate.test");
            LLVMBasicBlockRef body_block = LLVMAppendBasicBlockInContext(emitter->context, function, "truncate.body");
            LLVMBasicBlockRef end_block = LLVMAppendBasicBlockInContext(emitter->context, function, "truncate.end");
            LLVMValueRef index, element;
            LLVMBuildStore(emitter->builder, keep.value, counter);
            LLVMBuildBr(emitter->builder, test_block);
            LLVMPositionBuilderAtEnd(emitter->builder, test_block);
            index = LLVMBuildLoad2(emitter->builder, i64, counter, "truncate.i");
            LLVMBuildCondBr(emitter->builder,
                            LLVMBuildICmp(emitter->builder, LLVMIntULT, index, length, "truncate.more"),
                            body_block, end_block);
            LLVMPositionBuilderAtEnd(emitter->builder, body_block);
            element = LLVMBuildGEP2(emitter->builder, element_type, data, &index, 1U, "truncate.element");
            if (!__LLVM_Drop_Owned_At__(emitter, element, resolved.__Inner__))
                return result;
            LLVMBuildStore(emitter->builder,
                           LLVMBuildAdd(emitter->builder, index, LLVMConstInt(i64, 1U, 0), "truncate.next"),
                           counter);
            LLVMBuildBr(emitter->builder, test_block);
            LLVMPositionBuilderAtEnd(emitter->builder, end_block);
        }
        LLVMBuildStore(emitter->builder, keep.value, length_field);
        LLVMBuildBr(emitter->builder, done_block);
        LLVMPositionBuilderAtEnd(emitter->builder, done_block);
        result.value = LLVMConstNull(i64);
        result.type = &__LLVM_Void_Type__;
        return result;
    }
    {
        /* pop: moves the last element out as some(x); none when empty. */
        __Ast_Type__ *option_type = expression->__Contextual_Type__;
        LLVMTypeRef option_llvm_type;
        LLVMValueRef storage, empty, last;
        LLVMBasicBlockRef some_block, none_block, join_block;
        __Type_Tagged_Constructor__ some_constructor, none_constructor;
        __LLVM_Value__ element;
        static const char some_name[] = "موجود";
        static const char none_name[] = "لا_شيء";
        __Text_Slice__ some_text = {some_name, sizeof(some_name) - 1U};
        __Text_Slice__ none_text = {none_name, sizeof(none_name) - 1U};
        if (option_type == NULL || (option_llvm_type = __LLVM_Type__(emitter, option_type)) == NULL ||
            !__Type_Tagged_Find_Constructor__(option_type, some_text, &some_constructor) ||
            !__Type_Tagged_Find_Constructor__(option_type, none_text, &none_constructor))
        {
            __LLVM_Fail__("pop has no verified option type");
            return result;
        }
        storage = __LLVM_Allocate_Stack__(emitter, option_llvm_type, "pop.result");
        LLVMBuildStore(emitter->builder, LLVMConstNull(option_llvm_type), storage);
        empty = LLVMBuildICmp(emitter->builder, LLVMIntEQ, length, LLVMConstInt(i64, 0U, 0), "pop.empty");
        some_block = LLVMAppendBasicBlockInContext(emitter->context, function, "pop.some");
        none_block = LLVMAppendBasicBlockInContext(emitter->context, function, "pop.none");
        join_block = LLVMAppendBasicBlockInContext(emitter->context, function, "pop.join");
        LLVMBuildCondBr(emitter->builder, empty, none_block, some_block);
        LLVMPositionBuilderAtEnd(emitter->builder, some_block);
        last = LLVMBuildSub(emitter->builder, length, LLVMConstInt(i64, 1U, 0), "pop.last");
        element.value = LLVMBuildLoad2(
            emitter->builder, element_type,
            LLVMBuildGEP2(emitter->builder, element_type, data, &last, 1U, "pop.element.place"),
            "pop.element");
        element.type = resolved.__Inner__;
        LLVMBuildStore(emitter->builder, last, length_field);
        if (!__LLVM_Write_Tagged_Arm__(emitter, option_type, option_llvm_type, storage,
                                       some_constructor.__Tag__, element))
            return result;
        LLVMBuildBr(emitter->builder, join_block);
        LLVMPositionBuilderAtEnd(emitter->builder, none_block);
        LLVMBuildStore(emitter->builder, LLVMConstInt(i64, (unsigned long long)none_constructor.__Tag__, 0),
                       LLVMBuildStructGEP2(emitter->builder, option_llvm_type, storage, 0U, "pop.none.tag"));
        LLVMBuildBr(emitter->builder, join_block);
        LLVMPositionBuilderAtEnd(emitter->builder, join_block);
        result.value = LLVMBuildLoad2(emitter->builder, option_llvm_type, storage, "pop.value");
        result.type = option_type;
        return result;
    }
}

/* Emits a wrapping arithmetic primitive: the plain instruction, modulo 2^N;
 * a shift takes its amount modulo N, and a right shift of a signed Type is
 * arithmetic. */
static __LLVM_Value__ __LLVM_Emit_Wrapping__(__LLVM_Emitter__ *emitter,
                                            __Ast_Expression__ *expression,
                                            __Ast_Type__ *expected,
                                            __Name_Builtin_Function__ builtin)
{
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    __Ast_Type__ *type = expression->__Contextual_Type__;
    __LLVM_Value__ left, right;
    unsigned bits;
    int is_signed;
    if (type == NULL || !__LLVM_Resolve_Integer__(emitter, type, &bits, &is_signed))
    {
        __LLVM_Fail__("wrapping primitive has no verified integer type");
        return result;
    }
    left = __LLVM_Emit_Expression__(emitter, expression->__As__.__Call__.__Arguments__[0], type);
    if (left.value == NULL || (left = __LLVM_Coerce__(emitter, left, type)).value == NULL)
        return result;
    right = __LLVM_Emit_Expression__(emitter, expression->__As__.__Call__.__Arguments__[1], type);
    if (right.value == NULL || (right = __LLVM_Coerce__(emitter, right, type)).value == NULL)
        return result;
    switch (builtin)
    {
        case __Name_Builtin_Wrapping_Add__:
            result.value = LLVMBuildAdd(emitter->builder, left.value, right.value, "wrapping.add");
            break;
        case __Name_Builtin_Wrapping_Subtract__:
            result.value = LLVMBuildSub(emitter->builder, left.value, right.value, "wrapping.sub");
            break;
        case __Name_Builtin_Wrapping_Multiply__:
            result.value = LLVMBuildMul(emitter->builder, left.value, right.value, "wrapping.mul");
            break;
        default:
        {
            LLVMValueRef amount = LLVMBuildAnd(
                emitter->builder, right.value,
                LLVMConstInt(LLVMTypeOf(right.value), (unsigned long long)(bits - 1U), 0), "wrapping.amount");
            if (builtin == __Name_Builtin_Wrapping_Shift_Left__)
                result.value = LLVMBuildShl(emitter->builder, left.value, amount, "wrapping.shl");
            else if (is_signed)
                result.value = LLVMBuildAShr(emitter->builder, left.value, amount, "wrapping.ashr");
            else
                result.value = LLVMBuildLShr(emitter->builder, left.value, amount, "wrapping.lshr");
            break;
        }
    }
    result.type = type;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits an atomic primitive or the fence, as the self-hosted LLVM provider
 * does (ابن_بدائي_ذري_إل_إل_في_إم, compiler/backend/provider/llvm/lowering.sn):
 * every ordering is lowered at sequentially consistent strength, which is
 * always sound (the ordering operands are still evaluated, then unused).
 * - load/store: a seq_cst `load atomic` / `store atomic` with the natural
 *   alignment of the integer (an under-aligned atomic access would become an
 *   __atomic_* library call std does not link);
 * - compare-and-swap: `cmpxchg` seq_cst/seq_cst, answering field 0 (the old
 *   value);
 * - swap and the fetch operations: `atomicrmw xchg/add/and/or/xor` seq_cst,
 *   answering the old value;
 * - the fence: `fence seq_cst`.
 * The first operand is a reference, already a pointer in LLVM. */
static __LLVM_Value__ __LLVM_Emit_Atomic__(__LLVM_Emitter__ *emitter,
                                          __Ast_Expression__ *expression,
                                          __Ast_Type__ *expected,
                                          __Name_Builtin_Function__ builtin)
{
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    __Ast_Expression__ **arguments = expression->__As__.__Call__.__Arguments__;
    size_t count = expression->__As__.__Call__.__Argument_Count__;
    size_t expected_count;
    size_t value_count;
    size_t index;
    __Ast_Type__ *reference_type;
    __Resolved_Type__ reference;
    LLVMTypeRef cell_type;
    __LLVM_Value__ place;
    __LLVM_Value__ values[2];
    unsigned alignment;
    if (builtin == __Name_Builtin_Memory_Fence__)
    {
        if (count != 1U)
        {
            __LLVM_Fail__("memory fence has no verified ordering operand");
            return result;
        }
        if (__LLVM_Emit_Expression__(emitter, arguments[0], &__Body_Builtin_U32_Type__).value == NULL)
            return result;
        result.value = LLVMBuildFence(emitter->builder, LLVMAtomicOrderingSequentiallyConsistent, 0, "");
        result.type = &__LLVM_Void_Type__;
        return result;
    }
    if (builtin == __Name_Builtin_Atomic_Load__)
    {
        expected_count = 2U;
        value_count = 0U;
    }
    else if (builtin == __Name_Builtin_Atomic_Compare_Swap__)
    {
        expected_count = 5U;
        value_count = 2U;
    }
    else
    {
        expected_count = 3U;
        value_count = 1U;
    }
    reference_type = count > 0U ? arguments[0]->__Contextual_Type__ : NULL;
    if (count != expected_count || reference_type == NULL ||
        !__Type_Resolve__(emitter->semantic, reference_type, &reference) ||
        reference.__Kind__ != __Resolved_Type_Reference__ || reference.__Inner__ == NULL ||
        (cell_type = __LLVM_Type__(emitter, reference.__Inner__)) == NULL ||
        LLVMGetTypeKind(cell_type) != LLVMIntegerTypeKind)
    {
        __LLVM_Fail__("atomic primitive has no verified integer reference operand");
        return result;
    }
    alignment = LLVMGetIntTypeWidth(cell_type) / 8U;
    place = __LLVM_Emit_Expression__(emitter, arguments[0], reference_type);
    if (place.value == NULL)
        return result;
    if (LLVMGetTypeKind(LLVMTypeOf(place.value)) != LLVMPointerTypeKind)
    {
        __LLVM_Fail__("atomic primitive reference operand is not a pointer");
        return result;
    }
    for (index = 0U; index < value_count; ++index)
    {
        values[index] = __LLVM_Emit_Expression__(emitter, arguments[index + 1U], reference.__Inner__);
        if (values[index].value == NULL ||
            (values[index] = __LLVM_Coerce__(emitter, values[index], reference.__Inner__)).value == NULL)
            return result;
    }
    for (index = value_count + 1U; index < count; ++index)
    {
        if (__LLVM_Emit_Expression__(emitter, arguments[index], &__Body_Builtin_U32_Type__).value == NULL)
            return result;
    }
    switch (builtin)
    {
        case __Name_Builtin_Atomic_Load__:
            result.value = LLVMBuildLoad2(emitter->builder, cell_type, place.value, "atomic.load");
            LLVMSetOrdering(result.value, LLVMAtomicOrderingSequentiallyConsistent);
            LLVMSetAlignment(result.value, alignment);
            break;
        case __Name_Builtin_Atomic_Store__:
            result.value = LLVMBuildStore(emitter->builder, values[0].value, place.value);
            LLVMSetOrdering(result.value, LLVMAtomicOrderingSequentiallyConsistent);
            LLVMSetAlignment(result.value, alignment);
            result.type = &__LLVM_Void_Type__;
            return result;
        case __Name_Builtin_Atomic_Compare_Swap__:
        {
            LLVMValueRef pair = LLVMBuildAtomicCmpXchg(
                emitter->builder, place.value, values[0].value, values[1].value,
                LLVMAtomicOrderingSequentiallyConsistent, LLVMAtomicOrderingSequentiallyConsistent, 0);
            result.value = LLVMBuildExtractValue(emitter->builder, pair, 0U, "atomic.previous");
            break;
        }
        default:
        {
            LLVMAtomicRMWBinOp operation;
            if (builtin == __Name_Builtin_Atomic_Swap__)
                operation = LLVMAtomicRMWBinOpXchg;
            else if (builtin == __Name_Builtin_Atomic_Fetch_Add__)
                operation = LLVMAtomicRMWBinOpAdd;
            else if (builtin == __Name_Builtin_Atomic_Fetch_And__)
                operation = LLVMAtomicRMWBinOpAnd;
            else if (builtin == __Name_Builtin_Atomic_Fetch_Or__)
                operation = LLVMAtomicRMWBinOpOr;
            else
                operation = LLVMAtomicRMWBinOpXor;
            result.value = LLVMBuildAtomicRMW(emitter->builder, operation, place.value, values[0].value,
                                              LLVMAtomicOrderingSequentiallyConsistent, 0);
            break;
        }
    }
    result.type = reference.__Inner__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits a flat machine-memory primitive, as the self-hosted LLVM provider
 * does (ابن_بدائي_ذاكرة_آلة_إل_إل_في_إم): the u64 address becomes a pointer
 * by inttoptr and one 8-byte-aligned i64 load or store accesses it. Only the
 * 64-bit flat address spaces have it; wasm targets reject the primitive. */
static __LLVM_Value__ __LLVM_Emit_Machine_Memory__(__LLVM_Emitter__ *emitter,
                                                  __Ast_Expression__ *expression,
                                                  __Ast_Type__ *expected,
                                                  __Name_Builtin_Function__ builtin)
{
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    size_t count = builtin == __Name_Builtin_Machine_Memory_Read__ ? 1U : 2U;
    LLVMTypeRef word = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef pointer = LLVMPointerTypeInContext(emitter->context, 0U);
    __LLVM_Value__ operands[2];
    LLVMValueRef place;
    size_t index;
    if (emitter->target == NULL ||
        (emitter->target->architecture != __Bootstrap_Target_Architecture_X86_64__ &&
         emitter->target->architecture != __Bootstrap_Target_Architecture_AArch64__ &&
         emitter->target->architecture != __Bootstrap_Target_Architecture_RISCV64__) ||
        expression->__As__.__Call__.__Argument_Count__ != count)
    {
        __LLVM_Fail__("machine memory is supported only for 64-bit native targets");
        return result;
    }
    for (index = 0U; index < count; ++index)
    {
        operands[index] = __LLVM_Emit_Expression__(emitter, expression->__As__.__Call__.__Arguments__[index],
                                                   &__Body_Builtin_U64_Type__);
        if (operands[index].value == NULL ||
            (operands[index] = __LLVM_Coerce__(emitter, operands[index], &__Body_Builtin_U64_Type__)).value ==
                NULL)
            return result;
        if (LLVMTypeOf(operands[index].value) != word)
        {
            __LLVM_Fail__("machine memory operand is not a 64-bit word");
            return result;
        }
    }
    place = LLVMBuildIntToPtr(emitter->builder, operands[0].value, pointer, "machine.place");
    if (builtin == __Name_Builtin_Machine_Memory_Write__)
    {
        result.value = LLVMBuildStore(emitter->builder, operands[1].value, place);
        LLVMSetAlignment(result.value, 8U);
        result.type = &__LLVM_Void_Type__;
        return result;
    }
    result.value = LLVMBuildLoad2(emitter->builder, word, place, "machine.word");
    LLVMSetAlignment(result.value, 8U);
    result.type = &__Body_Builtin_U64_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits a WebAssembly linear-memory primitive, as the self-hosted LLVM
 * provider does (ابن_بدائي_ذاكرة_خطية_إل_إل_في_إم): only wasm32, whose one
 * linear memory is addressed by u32 offsets, has them; elsewhere they are
 * rejected, never emulated.
 * - grow: llvm.wasm.memory.grow.i32(memory 0, pages);
 * - read/write: the offset becomes a pointer by inttoptr and one 4-byte-
 *   aligned i32 load/store accesses it;
 * - copy: llvm.memcpy (memory.copy under bulk memory, the default wasm CPU);
 * - heap base: ptrtoint of the external `__heap_base` the wasm linker defines. */
static __LLVM_Value__ __LLVM_Emit_Linear_Memory__(__LLVM_Emitter__ *emitter,
                                                 __Ast_Expression__ *expression,
                                                 __Ast_Type__ *expected,
                                                 __Name_Builtin_Function__ builtin)
{
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    LLVMTypeRef word = LLVMIntTypeInContext(emitter->context, 32U);
    LLVMTypeRef pointer = LLVMPointerTypeInContext(emitter->context, 0U);
    size_t count = expression->__As__.__Call__.__Argument_Count__;
    LLVMValueRef operands[3];
    size_t index;
    if (emitter->target == NULL || emitter->target->architecture != __Bootstrap_Target_Architecture_Wasm32__ ||
        count > 3U)
    {
        __LLVM_Fail__("linear memory is supported only for wasm32 targets");
        return result;
    }
    for (index = 0U; index < count; ++index)
    {
        __LLVM_Value__ value = __LLVM_Emit_Expression__(
            emitter, expression->__As__.__Call__.__Arguments__[index], &__Body_Builtin_U32_Type__);
        if (value.value == NULL || (value = __LLVM_Coerce__(emitter, value, &__Body_Builtin_U32_Type__)).value == NULL)
            return result;
        operands[index] = value.value;
    }
    switch (builtin)
    {
        case __Name_Builtin_Linear_Memory_Grow__:
        {
            static const char name[] = "llvm.wasm.memory.grow";
            unsigned id = LLVMLookupIntrinsicID(name, sizeof(name) - 1U);
            LLVMValueRef arguments[2];
            if (id == 0U || count != 1U)
            {
                __LLVM_Fail__("linear memory grow has no LLVM intrinsic");
                return result;
            }
            arguments[0] = LLVMConstInt(word, 0U, 0);
            arguments[1] = operands[0];
            result.value = LLVMBuildCall2(emitter->builder, LLVMIntrinsicGetType(emitter->context, id, &word, 1U),
                                          LLVMGetIntrinsicDeclaration(emitter->module, id, &word, 1U),
                                          arguments, 2U, "linear.grow");
            break;
        }
        case __Name_Builtin_Linear_Memory_Read__:
            result.value = LLVMBuildLoad2(emitter->builder, word,
                                          LLVMBuildIntToPtr(emitter->builder, operands[0], pointer, "linear.place"),
                                          "linear.word");
            LLVMSetAlignment(result.value, 4U);
            break;
        case __Name_Builtin_Linear_Memory_Write__:
            result.value = LLVMBuildStore(
                emitter->builder, operands[1],
                LLVMBuildIntToPtr(emitter->builder, operands[0], pointer, "linear.place"));
            LLVMSetAlignment(result.value, 4U);
            result.type = &__LLVM_Void_Type__;
            return result;
        case __Name_Builtin_Linear_Memory_Copy__:
            result.value = LLVMBuildMemCpy(
                emitter->builder, LLVMBuildIntToPtr(emitter->builder, operands[0], pointer, "linear.destination"),
                1U, LLVMBuildIntToPtr(emitter->builder, operands[1], pointer, "linear.source"), 1U, operands[2]);
            result.type = &__LLVM_Void_Type__;
            return result;
        default:
        {
            LLVMValueRef base = LLVMGetNamedGlobal(emitter->module, "__heap_base");
            if (base == NULL)
                base = LLVMAddGlobal(emitter->module, LLVMIntTypeInContext(emitter->context, 8U), "__heap_base");
            result.value = LLVMBuildPtrToInt(emitter->builder, base, word, "linear.heap.base");
            break;
        }
    }
    result.type = &__Body_Builtin_U32_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Calls the LLVM float intrinsic NAME overloaded on the operands' float type. */
static LLVMValueRef __LLVM_Emit_Float_Intrinsic__(__LLVM_Emitter__ *emitter,
                                                  const char *name,
                                                  LLVMValueRef *operands,
                                                  unsigned count)
{
    unsigned id = LLVMLookupIntrinsicID(name, strlen(name));
    LLVMTypeRef type = LLVMTypeOf(operands[0]);
    if (id == 0U)
    {
        __LLVM_Fail__("float primitive has no LLVM intrinsic");
        return NULL;
    }
    return LLVMBuildCall2(emitter->builder, LLVMIntrinsicGetType(emitter->context, id, &type, 1U),
                          LLVMGetIntrinsicDeclaration(emitter->module, id, &type, 1U), operands, count,
                          "float.operation");
}

/* IEEE 754-2019 minimum/maximum spelled with compares and selects, for
 * targets whose LLVM back end may lower llvm.minimum/llvm.maximum to a libm
 * call (ابن_أدنى_أقصى_صريح_إل_إل_في_إم): unordered operands give a + b (a
 * quiet NaN); equal operands (only the sign of a zero can differ) combine
 * their bits with OR (minimum: -0 wins) or AND (maximum: +0 wins); otherwise
 * an ordered compare selects. */
static LLVMValueRef __LLVM_Emit_Explicit_Minimum_Maximum__(__LLVM_Emitter__ *emitter,
                                                           LLVMValueRef left,
                                                           LLVMValueRef right,
                                                           unsigned bits,
                                                           int maximum)
{
    LLVMBuilderRef builder = emitter->builder;
    LLVMTypeRef type = LLVMTypeOf(left);
    LLVMTypeRef integer = LLVMIntTypeInContext(emitter->context, bits);
    LLVMValueRef unordered = LLVMBuildFCmp(builder, LLVMRealUNO, left, right, "float.unordered");
    LLVMValueRef ordered = LLVMBuildFCmp(builder, maximum ? LLVMRealOGT : LLVMRealOLT, left, right,
                                         "float.ordered");
    LLVMValueRef chosen = LLVMBuildSelect(builder, ordered, left, right, "float.chosen");
    LLVMValueRef equal = LLVMBuildFCmp(builder, LLVMRealOEQ, left, right, "float.equal");
    LLVMValueRef left_bits = LLVMBuildBitCast(builder, left, integer, "float.left.bits");
    LLVMValueRef right_bits = LLVMBuildBitCast(builder, right, integer, "float.right.bits");
    LLVMValueRef zero_bits = maximum ? LLVMBuildAnd(builder, left_bits, right_bits, "float.zero.bits")
                                     : LLVMBuildOr(builder, left_bits, right_bits, "float.zero.bits");
    LLVMValueRef zero = LLVMBuildBitCast(builder, zero_bits, type, "float.zero");
    LLVMValueRef ordered_result = LLVMBuildSelect(builder, equal, zero, chosen, "float.ordered.result");
    LLVMValueRef nan = LLVMBuildFAdd(builder, left, right, "float.nan");
    return LLVMBuildSelect(builder, unordered, nan, ordered_result, "float.minmax");
}

/* Emits a float operation as the LLVM intrinsic rustc emits for f64::sqrt,
 * abs, floor, ceil, trunc, round_ties_even, minimum, maximum, copysign and
 * mul_add (ابن_عملية_عائمة_إل_إل_في_إم). The selected target must have the
 * instruction (يدعم_عملية_عائمة_للهدف): the rounding operations exist only on
 * AArch64 and wasm32, fused multiply-add only on AArch64 and RISC-V; std
 * selects its exact software forms elsewhere, so no libm call is ever
 * emitted. llvm.minimum/llvm.maximum are native on AArch64, WebAssembly and
 * x86-64; other targets use the explicit compare-and-select form. */
static __LLVM_Value__ __LLVM_Emit_Float_Operation__(__LLVM_Emitter__ *emitter,
                                                   __Ast_Expression__ *expression,
                                                   __Ast_Type__ *expected,
                                                   __Name_Builtin_Function__ builtin)
{
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    __Ast_Type__ *type = expression->__Contextual_Type__;
    __Bootstrap_Target_Architecture__ architecture;
    __Resolved_Type__ resolved;
    LLVMValueRef operands[3];
    size_t count = expression->__As__.__Call__.__Argument_Count__;
    size_t index;
    int supported;
    int native_minimum;
    if (emitter->target == NULL || type == NULL || !__Type_Resolve__(emitter->semantic, type, &resolved) ||
        resolved.__Kind__ != __Resolved_Type_Float__ || count == 0U || count > 3U)
    {
        __LLVM_Fail__("float primitive has no verified float type");
        return result;
    }
    architecture = emitter->target->architecture;
    switch (builtin)
    {
        case __Name_Builtin_Float_Floor__:
        case __Name_Builtin_Float_Ceil__:
        case __Name_Builtin_Float_Trunc__:
        case __Name_Builtin_Float_Round_Even__:
            supported = architecture == __Bootstrap_Target_Architecture_AArch64__ ||
                        architecture == __Bootstrap_Target_Architecture_Wasm32__;
            break;
        case __Name_Builtin_Float_Fused_Multiply_Add__:
            supported = architecture == __Bootstrap_Target_Architecture_AArch64__ ||
                        architecture == __Bootstrap_Target_Architecture_RISCV64__;
            break;
        default:
            supported = 1;
            break;
    }
    if (!supported)
    {
        __LLVM_Fail__("float primitive has no instruction on the selected target");
        return result;
    }
    for (index = 0U; index < count; ++index)
    {
        __LLVM_Value__ value = __LLVM_Emit_Expression__(emitter, expression->__As__.__Call__.__Arguments__[index], type);
        if (value.value == NULL || (value = __LLVM_Coerce__(emitter, value, type)).value == NULL)
            return result;
        operands[index] = value.value;
    }
    native_minimum = architecture == __Bootstrap_Target_Architecture_AArch64__ ||
                     architecture == __Bootstrap_Target_Architecture_Wasm32__ ||
                     architecture == __Bootstrap_Target_Architecture_X86_64__;
    switch (builtin)
    {
        case __Name_Builtin_Float_Sqrt__:
            result.value = __LLVM_Emit_Float_Intrinsic__(emitter, "llvm.sqrt", operands, 1U);
            break;
        case __Name_Builtin_Float_Abs__:
            result.value = __LLVM_Emit_Float_Intrinsic__(emitter, "llvm.fabs", operands, 1U);
            break;
        case __Name_Builtin_Float_Floor__:
            result.value = __LLVM_Emit_Float_Intrinsic__(emitter, "llvm.floor", operands, 1U);
            break;
        case __Name_Builtin_Float_Ceil__:
            result.value = __LLVM_Emit_Float_Intrinsic__(emitter, "llvm.ceil", operands, 1U);
            break;
        case __Name_Builtin_Float_Trunc__:
            result.value = __LLVM_Emit_Float_Intrinsic__(emitter, "llvm.trunc", operands, 1U);
            break;
        case __Name_Builtin_Float_Round_Even__:
            result.value = __LLVM_Emit_Float_Intrinsic__(emitter, "llvm.roundeven", operands, 1U);
            break;
        case __Name_Builtin_Float_Minimum__:
        case __Name_Builtin_Float_Maximum__:
            if (count != 2U)
                break;
            result.value = native_minimum
                               ? __LLVM_Emit_Float_Intrinsic__(
                                     emitter,
                                     builtin == __Name_Builtin_Float_Minimum__ ? "llvm.minimum" : "llvm.maximum",
                                     operands, 2U)
                               : __LLVM_Emit_Explicit_Minimum_Maximum__(
                                     emitter, operands[0], operands[1], resolved.__Bits__,
                                     builtin == __Name_Builtin_Float_Maximum__);
            break;
        case __Name_Builtin_Float_Copy_Sign__:
            if (count == 2U)
                result.value = __LLVM_Emit_Float_Intrinsic__(emitter, "llvm.copysign", operands, 2U);
            break;
        case __Name_Builtin_Float_Fused_Multiply_Add__:
            if (count == 3U)
                result.value = __LLVM_Emit_Float_Intrinsic__(emitter, "llvm.fma", operands, 3U);
            break;
        default:
            break;
    }
    if (result.value == NULL)
    {
        __LLVM_Fail__("float primitive has no verified operands");
        return result;
    }
    result.type = type;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM builtin call. */
__LLVM_Value__ __LLVM_Emit_Builtin_Call__(__LLVM_Emitter__ *emitter,
                                          __Ast_Expression__ *expression,
                                          __Ast_Type__ *expected,
                                          __Name_Builtin_Function__ builtin)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    if (builtin == __Name_Builtin_Host_Architecture__ ||
        builtin == __Name_Builtin_Host_Platform__ ||
        builtin == __Name_Builtin_Host_Environment__)
        return __LLVM_Emit_Host_Identity__(emitter, builtin, expected);
    if (builtin == __Name_Builtin_F64_Bits__ ||
        builtin == __Name_Builtin_F64_From_Bits__ ||
        builtin == __Name_Builtin_F32_Bits__ ||
        builtin == __Name_Builtin_F32_From_Bits__)
    {
        __Ast_Type__ *argument_type = NULL;
        __Ast_Type__ *result_type;
        __LLVM_Value__ argument;
        LLVMTypeRef llvm_result_type;
        if (expression->__As__.__Call__.__Argument_Count__ != 1U ||
            (argument_type = __LLVM_Expression_Type__(
                emitter, expression->__As__.__Call__.__Arguments__[0])) == NULL)
        {
            __LLVM_Fail__("scalar bit builtin has no verified argument type");
            return result;
        }
        /* Stage0 uses its canonical machine Type descriptors, not host C floats. */
        result_type = (builtin == __Name_Builtin_F64_Bits__)
                          ? &__Body_Builtin_U64_Type__
                          : (builtin == __Name_Builtin_F64_From_Bits__)
                                ? &__Body_Builtin_F64_Type__
                                : (builtin == __Name_Builtin_F32_Bits__)
                                      ? &__Body_Builtin_U32_Type__
                                      : &__Body_Builtin_F32_Type__;
        llvm_result_type = __LLVM_Type__(emitter, result_type);
        if (llvm_result_type == NULL)
            return result;
        argument = __LLVM_Emit_Expression__(
            emitter, expression->__As__.__Call__.__Arguments__[0], argument_type);
        if (argument.value == NULL)
            return result;
        {
            LLVMTypeKind operand_kind = LLVMGetTypeKind(LLVMTypeOf(argument.value));
            LLVMTypeKind output_kind = LLVMGetTypeKind(llvm_result_type);
            int canonical = 0;
            if (builtin == __Name_Builtin_F64_Bits__)
                canonical = operand_kind == LLVMDoubleTypeKind &&
                            output_kind == LLVMIntegerTypeKind &&
                            LLVMGetIntTypeWidth(llvm_result_type) == 64U;
            else if (builtin == __Name_Builtin_F64_From_Bits__)
                canonical = operand_kind == LLVMIntegerTypeKind &&
                            LLVMGetIntTypeWidth(LLVMTypeOf(argument.value)) == 64U &&
                            output_kind == LLVMDoubleTypeKind;
            else if (builtin == __Name_Builtin_F32_Bits__)
                canonical = operand_kind == LLVMFloatTypeKind &&
                            output_kind == LLVMIntegerTypeKind &&
                            LLVMGetIntTypeWidth(llvm_result_type) == 32U;
            else if (builtin == __Name_Builtin_F32_From_Bits__)
                canonical = operand_kind == LLVMIntegerTypeKind &&
                            LLVMGetIntTypeWidth(LLVMTypeOf(argument.value)) == 32U &&
                            output_kind == LLVMFloatTypeKind;
            if (!canonical)
            {
                __LLVM_Fail__("scalar bit builtin received a noncanonical type or width");
                return result;
            }
        }
        result.value = LLVMBuildBitCast(emitter->builder,
                                        argument.value, llvm_result_type, "scalar.bits");
        result.type = result_type;
        return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
    }
    if (builtin == __Name_Builtin_Raw_Kernel_Call__)
    {
        /* Mirrors canonical ابن_نداء_نواة_خام_إل_إل_في_إم
         * (compiler/backend/provider/llvm/lowering.sn): seven i64 operands,
         * pointers converted with ptrtoint, side-effecting inline asm. The
         * selected target, never the host, decides the ABI. */
        static const char x86_asm[] = "syscall";
        static const char x86_constraints[] =
            "={rax},0,{rdi},{rsi},{rdx},{r10},{r8},{r9},~{rcx},~{r11},~{flags},~{memory}";
        static const char arm_asm[] = "svc #0";
        static const char arm_constraints[] =
            "={x0},{x8},0,{x1},{x2},{x3},{x4},{x5},~{memory},~{cc}";
        const char *asm_text;
        const char *constraints;
        LLVMTypeRef word = LLVMIntTypeInContext(emitter->context, 64U);
        LLVMTypeRef operand_types[7];
        LLVMValueRef operands[7];
        LLVMTypeRef signature;
        LLVMValueRef inline_asm;
        size_t index;
        if (emitter->target == NULL ||
            emitter->target->platform != __Bootstrap_Target_Platform_Linux__ ||
            expression->__As__.__Call__.__Argument_Count__ != 7U)
        {
            __LLVM_Fail__("raw kernel call is supported only for registered 64-bit Linux targets");
            return result;
        }
        if (emitter->target->architecture == __Bootstrap_Target_Architecture_X86_64__)
        {
            asm_text = x86_asm;
            constraints = x86_constraints;
        }
        else if (emitter->target->architecture == __Bootstrap_Target_Architecture_AArch64__)
        {
            asm_text = arm_asm;
            constraints = arm_constraints;
        }
        else
        {
            __LLVM_Fail__("raw kernel call is supported only for registered 64-bit Linux targets");
            return result;
        }
        for (index = 0U; index < 7U; ++index)
        {
            __Ast_Expression__ *argument = expression->__As__.__Call__.__Arguments__[index];
            __Ast_Type__ *argument_type = __LLVM_Expression_Type__(emitter, argument);
            __LLVM_Value__ value;
            LLVMTypeRef value_type;
            if (argument_type == NULL)
                argument_type = &__Body_Builtin_U64_Type__;
            value = __LLVM_Emit_Expression__(emitter, argument, argument_type);
            if (value.value == NULL)
                return result;
            value_type = LLVMTypeOf(value.value);
            if (LLVMGetTypeKind(value_type) == LLVMPointerTypeKind)
            {
                value.value = LLVMBuildPtrToInt(emitter->builder, value.value, word, "kernel.ptr");
            }
            else if (LLVMGetTypeKind(value_type) != LLVMIntegerTypeKind ||
                     LLVMGetIntTypeWidth(value_type) != 64U)
            {
                __LLVM_Fail__("raw kernel call operand is not a 64-bit word or pointer");
                return result;
            }
            operand_types[index] = word;
            operands[index] = value.value;
        }
        signature = LLVMFunctionType(word, operand_types, 7U, 0);
        inline_asm = LLVMGetInlineAsm(signature, (char *)asm_text, strlen(asm_text),
                                      (char *)constraints, strlen(constraints),
                                      1, 0, LLVMInlineAsmDialectATT, 0);
        result.value = LLVMBuildCall2(emitter->builder, signature, inline_asm, operands, 7U,
                                      "kernel.result");
        result.type = &__Body_Builtin_U64_Type__;
        return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
    }
    if (builtin == __Name_Builtin_C_Address_Call__)
    {
        /* Mirrors canonical ابن_نداء_عنوان_سي_إل_إل_في_إم
         * (compiler/backend/provider/llvm/lowering.sn): an indirect C call.
         * System V x86-64 passes every word as a variadic argument (the same
         * registers as a prototyped call, plus AL for a variadic callee);
         * Windows x64 passes the eight integer words positionally to a
         * variadic `ret (...)` type: the first four as double, which the
         * Win64 variadic convention passes in xmm k and copies to the k-th
         * integer register (so an integer or a float parameter finds the
         * same bits), the next four as i64 on the stack; AAPCS64 uses the
         * prototyped (i64 x8, double x8) type. */
        LLVMTypeRef word = LLVMIntTypeInContext(emitter->context, 64U);
        LLVMTypeRef real = LLVMDoubleTypeInContext(emitter->context);
        LLVMTypeRef pointer = LLVMPointerTypeInContext(emitter->context, 0U);
        LLVMTypeRef parameter_types[16];
        LLVMValueRef operands[16];
        LLVMTypeRef result_type;
        LLVMTypeRef signature;
        LLVMValueRef callee;
        __Ast_Type__ *result_ast = expression->__Contextual_Type__;
        __LLVM_Value__ address;
        size_t index;
        int variadic;
        int windows_x64 = 0;
        unsigned operand_count = 16U;
        if (emitter->target == NULL || expression->__As__.__Call__.__Argument_Count__ != 18U ||
            result_ast == NULL)
        {
            __LLVM_Fail__("C address call has no verified signature");
            return result;
        }
        if (emitter->target->architecture == __Bootstrap_Target_Architecture_X86_64__ &&
            emitter->target->platform == __Bootstrap_Target_Platform_Windows__)
        {
            variadic = 1;
            windows_x64 = 1;
        }
        else if (emitter->target->architecture == __Bootstrap_Target_Architecture_X86_64__)
            variadic = 1;
        else if (emitter->target->architecture == __Bootstrap_Target_Architecture_AArch64__)
            variadic = 0;
        else
        {
            __LLVM_Fail__("C address call is supported only for 64-bit System V, Windows x64 and AAPCS64 targets");
            return result;
        }
        result_type = (result_ast == &__Body_Builtin_F64_Type__) ? real : word;
        address = __LLVM_Emit_Expression__(emitter, expression->__As__.__Call__.__Arguments__[1],
                                           &__Body_Builtin_U64_Type__);
        if (address.value == NULL)
            return result;
        callee = LLVMBuildIntToPtr(emitter->builder, address.value, pointer, "c.callee");
        for (index = 0U; index < 16U; ++index)
        {
            __Ast_Type__ *slot_type = index < 8U ? &__Body_Builtin_U64_Type__ : &__Body_Builtin_F64_Type__;
            __LLVM_Value__ value = __LLVM_Emit_Expression__(
                emitter, expression->__As__.__Call__.__Arguments__[index + 2U], slot_type);
            if (value.value == NULL || (value = __LLVM_Coerce__(emitter, value, slot_type)).value == NULL)
                return result;
            parameter_types[index] = index < 8U ? word : real;
            operands[index] = value.value;
        }
        if (windows_x64)
        {
            /* Only the eight integer words, positionally; words 0..3 as double. */
            for (index = 0U; index < 4U; ++index)
                operands[index] = LLVMBuildBitCast(emitter->builder, operands[index], real, "c.word.xmm");
            operand_count = 8U;
        }
        signature = variadic ? LLVMFunctionType(result_type, parameter_types, 0U, 1)
                             : LLVMFunctionType(result_type, parameter_types, 16U, 0);
        result.value =
            LLVMBuildCall2(emitter->builder, signature, callee, operands, operand_count, "c.result");
        result.type = result_ast;
        return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
    }
    if (builtin == __Name_Builtin_Release__)
    {
        /* حرر: the owned local is moved out (semantic analysis cleared its
         * drop flag at this read) and destroyed now through its drop glue,
         * the same release every scope exit performs. */
        __Ast_Expression__ *argument = expression->__As__.__Call__.__Arguments__[0];
        __LLVM_Value__ value;
        LLVMTypeRef llvm_type;
        LLVMValueRef storage;
        if (expression->__As__.__Call__.__Argument_Count__ != 1U || argument == NULL ||
            argument->__Kind__ != __Ast_Expression_Atom__ ||
            argument->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__)
        {
            __LLVM_Fail__("release requires one owned local");
            return result;
        }
        value = __LLVM_Emit_Lvalue__(emitter, argument->__As__.__Atom__.__As__.__Lvalue__);
        if (value.value == NULL || value.type == NULL ||
            (llvm_type = __LLVM_Type__(emitter, value.type)) == NULL)
            return result;
        storage = __LLVM_Allocate_Stack__(emitter, llvm_type, "release.value");
        LLVMBuildStore(emitter->builder, value.value, storage);
        if (!__LLVM_Drop_Owned_At__(emitter, storage, value.type))
            return result;
        result.value = LLVMConstNull(LLVMIntTypeInContext(emitter->context, 64U));
        result.type = &__LLVM_Void_Type__;
        return result;
    }
    if (builtin == __Name_Builtin_Length__)
    {
        /* References the argument type. */
        __Ast_Type__ *argument_type;
        /* Stores the resolved. */
        __Resolved_Type__ resolved;
        /* Stores the sequence. */
        __LLVM_Value__ sequence;
        if (expression->__As__.__Call__.__Argument_Count__ != 1U ||
            (argument_type = __LLVM_Expression_Type__(
                 emitter, expression->__As__.__Call__.__Arguments__[0])) == NULL ||
            !__Type_Resolve__(emitter->semantic, argument_type, &resolved) ||
            (resolved.__Kind__ != __Resolved_Type_Vector__ &&
             resolved.__Kind__ != __Resolved_Type_String__))
        {
            __LLVM_Fail__("L2.7 length call disagrees with canonical builtin semantics");
            return result;
        }
        sequence = __LLVM_Emit_Expression__(
            emitter, expression->__As__.__Call__.__Arguments__[0], argument_type);
        if (sequence.value == NULL)
            return result;
        result.value =
            LLVMBuildExtractValue(emitter->builder, sequence.value, 1U, "sequence.length");
        if (!__LLVM_Drop_Read_Temporary_Member__(emitter, expression->__As__.__Call__.__Arguments__[0],
                                                 sequence))
            return __LLVM_Invalid_Value__();
        result.type = &__LLVM_Integer_Type__;
        return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
    }
    if (builtin == __Name_Builtin_Swap__)
    {
        __Ast_Expression__ *left_expression;
        __Ast_Expression__ *right_expression;
        __Ast_Lvalue__ *left_lvalue;
        __Ast_Lvalue__ *right_lvalue;
        __LLVM_Place__ left_place;
        __LLVM_Place__ right_place;
        LLVMValueRef left_value;
        LLVMValueRef right_value;
        LLVMValueRef last_store;

        if (expression->__As__.__Call__.__Argument_Count__ != 2U ||
            (left_expression = expression->__As__.__Call__.__Arguments__[0]) == NULL ||
            (right_expression = expression->__As__.__Call__.__Arguments__[1]) == NULL ||
            left_expression->__Kind__ != __Ast_Expression_Atom__ ||
            right_expression->__Kind__ != __Ast_Expression_Atom__ ||
            left_expression->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
            right_expression->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
            (left_lvalue = left_expression->__As__.__Atom__.__As__.__Lvalue__) == NULL ||
            (right_lvalue = right_expression->__As__.__Atom__.__As__.__Lvalue__) == NULL)
        {
            __LLVM_Fail__("L2.7 swap disagrees with canonical builtin semantics");
            return result;
        }
        left_place = __LLVM_Emit_Place__(emitter, left_lvalue);
        right_place = __LLVM_Emit_Place__(emitter, right_lvalue);
        if (left_place.address == NULL || right_place.address == NULL ||
            left_place.type == NULL || right_place.type == NULL ||
            left_place.llvm_type == NULL || right_place.llvm_type == NULL ||
            !__Type_Compatible__(emitter->semantic, left_place.type, right_place.type) ||
            !__Type_Compatible__(emitter->semantic, right_place.type, left_place.type))
        {
            __LLVM_Fail__("L2.7 swap requires two compatible mutable places");
            return result;
        }
        left_value = LLVMBuildLoad2(
            emitter->builder, left_place.llvm_type, left_place.address, "swap.left");
        right_value = LLVMBuildLoad2(
            emitter->builder, right_place.llvm_type, right_place.address, "swap.right");
        LLVMBuildStore(emitter->builder, right_value, left_place.address);
        last_store = LLVMBuildStore(emitter->builder, left_value, right_place.address);
        result.value = last_store;
        result.type = &__LLVM_Void_Type__;
        return result;
    }
    if (builtin == __Name_Builtin_Append__)
    {
        /* References the vector expression. */
        __Ast_Expression__ *vector_expression;
        /* References the vector lvalue. */
        __Ast_Lvalue__ *vector_lvalue;
        /* Stores the vector place. */
        __LLVM_Place__ vector_place;
        /* Stores the vector resolved. */
        __Resolved_Type__ vector_resolved;
        /* Stores the element type. */
        LLVMTypeRef element_type;
        /* Stores the element pointer type. */
        LLVMTypeRef element_pointer_type;
        /* Stores the LLVM i64 type. */
        LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
        /* Stores the data field. */
        LLVMValueRef data_field;
        /* Stores the length field. */
        LLVMValueRef length_field;
        /* Stores the capacity field. */
        LLVMValueRef capacity_field;
        /* Stores the data. */
        LLVMValueRef data;
        /* Stores the length. */
        LLVMValueRef length;
        /* Stores the capacity. */
        LLVMValueRef capacity;
        /* Stores the one. */
        LLVMValueRef one = LLVMConstInt(i64, 1U, 0);
        /* Stores the required. */
        LLVMValueRef required;
        /* Stores the length overflow. */
        LLVMValueRef length_overflow;
        /* Stores the need grow. */
        LLVMValueRef need_grow;
        /* Stores the function. */
        LLVMValueRef function;
        /* Stores the current. */
        LLVMBasicBlockRef current;
        /* Stores the grow block. */
        LLVMBasicBlockRef grow_block;
        /* Stores the append block. */
        LLVMBasicBlockRef append_block;
        /* References the policy. */
        const __Memory_Vector_Growth_Policy__ *policy;
        /* Stores the element size. */
        size_t element_size = 0U;
        /* Stores the element alignment. */
        size_t element_alignment = 1U;
        /* Stores the element. */
        __LLVM_Value__ element;

        if (expression->__As__.__Call__.__Argument_Count__ != 2U ||
            (vector_expression = expression->__As__.__Call__.__Arguments__[0]) == NULL)
        {
            __LLVM_Fail__("L2.7 append disagrees with canonical builtin semantics");
            return result;
        }
        if (vector_expression->__Kind__ == __Ast_Expression_Atom__ &&
            vector_expression->__As__.__Atom__.__Kind__ == __Ast_Atom_Lvalue__ &&
            (vector_lvalue = vector_expression->__As__.__Atom__.__As__.__Lvalue__) != NULL)
        {
            vector_place = __LLVM_Emit_Place__(emitter, vector_lvalue);
        }
        else if (vector_expression->__Kind__ == __Ast_Expression_Unary__ &&
                 vector_expression->__As__.__Unary__.__Operation__ == __Unary_Dereference__)
        {
            __LLVM_Value__ reference = __LLVM_Emit_Expression__(
                emitter, vector_expression->__As__.__Unary__.__Operand__, NULL);
            __Resolved_Type__ reference_resolved;
            if (reference.value == NULL || reference.type == NULL ||
                !__Type_Resolve__(emitter->semantic, reference.type, &reference_resolved) ||
                reference_resolved.__Kind__ != __Resolved_Type_Reference__ ||
                !reference_resolved.__Reference_Mutable__ ||
                reference_resolved.__Inner__ == NULL)
            {
                __LLVM_Fail__("L2.7 append dereference requires mutable reference storage");
                return result;
            }
            vector_place.address = reference.value;
            vector_place.type = reference_resolved.__Inner__;
            vector_place.llvm_type = __LLVM_Type__(emitter, vector_place.type);
        }
        else
        {
            __LLVM_Fail__("L2.7 append disagrees with canonical builtin semantics");
            return result;
        }
        if (vector_place.address == NULL || vector_place.type == NULL ||
            !__Type_Resolve__(emitter->semantic, vector_place.type, &vector_resolved) ||
            vector_resolved.__Kind__ != __Resolved_Type_Vector__ ||
            vector_resolved.__Inner__ == NULL ||
            !__Layout_Type__(emitter->semantic,
                             vector_resolved.__Inner__,
                             &element_size,
                             &element_alignment) ||
            element_size == 0U)
        {
            __LLVM_Fail__("L2.7 append requires canonical mutable vector storage");
            return result;
        }
        (void)element_alignment;
        policy = __Memory_Vector_Growth_Policy_View__();
        if (policy == NULL || policy->__Minimum_Capacity__ == 0U ||
            policy->__Growth_Factor__ < 2U)
        {
            __LLVM_Fail__("L2.7 append could not consume canonical vector growth policy");
            return result;
        }
        element_type = __LLVM_Type__(emitter, vector_resolved.__Inner__);
        if (element_type == NULL)
            return result;
        element_pointer_type = LLVMPointerType(element_type, 0U);
        element = __LLVM_Emit_Expression__(emitter,
                                           expression->__As__.__Call__.__Arguments__[1],
                                           vector_resolved.__Inner__);
        if (element.value == NULL)
            return result;
        element = __LLVM_Coerce__(emitter, element, vector_resolved.__Inner__);
        if (element.value == NULL)
            return result;

        data_field = LLVMBuildStructGEP2(emitter->builder,
                                         vector_place.llvm_type,
                                         vector_place.address,
                                         0U,
                                         "append.data.field");
        length_field = LLVMBuildStructGEP2(emitter->builder,
                                           vector_place.llvm_type,
                                           vector_place.address,
                                           1U,
                                           "append.length.field");
        capacity_field = LLVMBuildStructGEP2(emitter->builder,
                                             vector_place.llvm_type,
                                             vector_place.address,
                                             2U,
                                             "append.capacity.field");
        data = LLVMBuildLoad2(
            emitter->builder, element_pointer_type, data_field, "append.data");
        length = LLVMBuildLoad2(emitter->builder, i64, length_field, "append.length");
        capacity = LLVMBuildLoad2(emitter->builder, i64, capacity_field, "append.capacity");
        required = LLVMBuildAdd(emitter->builder, length, one, "append.required");
        length_overflow = LLVMBuildICmp(
            emitter->builder, LLVMIntULT, required, length, "append.length.overflow");
        if (!__LLVM_Emit_Trap_If__(emitter, length_overflow, "vector.length.overflow"))
            return result;
        need_grow = LLVMBuildICmp(
            emitter->builder, LLVMIntUGT, required, capacity, "append.needs.grow");
        current = LLVMGetInsertBlock(emitter->builder);
        function = current != NULL ? LLVMGetBasicBlockParent(current) : NULL;
        if (function == NULL)
        {
            __LLVM_Fail__("L2.7 append lost current LLVM function");
            return result;
        }
        grow_block =
            LLVMAppendBasicBlockInContext(emitter->context, function, "append.grow");
        append_block =
            LLVMAppendBasicBlockInContext(emitter->context, function, "append.store");
        LLVMBuildCondBr(emitter->builder, need_grow, grow_block, append_block);

        LLVMPositionBuilderAtEnd(emitter->builder, grow_block);
        {
            /* Stores the zero. */
            LLVMValueRef zero = LLVMConstInt(i64, 0U, 0);
            /* Tracks whether the capacity is is zero. */
            LLVMValueRef capacity_is_zero = LLVMBuildICmp(
                emitter->builder, LLVMIntEQ, capacity, zero, "append.capacity.zero");
            /* Stores the factor. */
            LLVMValueRef factor =
                LLVMConstInt(i64, (unsigned long long)policy->__Growth_Factor__, 0);
            /* Stores the min capacity. */
            LLVMValueRef min_capacity =
                LLVMConstInt(i64, (unsigned long long)policy->__Minimum_Capacity__, 0);
            /* Stores the max before growth. */
            LLVMValueRef max_before_growth = LLVMConstInt(
                i64, UINT64_MAX / (unsigned long long)policy->__Growth_Factor__, 0);
            /* Stores the growth overflow. */
            LLVMValueRef growth_overflow = LLVMBuildICmp(emitter->builder,
                                                         LLVMIntUGT,
                                                         capacity,
                                                         max_before_growth,
                                                         "append.capacity.overflow");
            /* Stores the grown. */
            LLVMValueRef grown;
            /* Stores the new capacity. */
            LLVMValueRef new_capacity;
            /* Stores the max before bytes. */
            LLVMValueRef max_before_bytes =
                LLVMConstInt(i64, UINT64_MAX / (unsigned long long)element_size, 0);
            /* Stores the bytes overflow. */
            LLVMValueRef bytes_overflow;
            /* Stores the byte count. */
            LLVMValueRef byte_count;
            /* Stores the new data. */
            LLVMValueRef new_data;
            if (!__LLVM_Emit_Trap_If__(
                    emitter, growth_overflow, "vector.capacity.overflow"))
                return result;
            grown =
                LLVMBuildMul(emitter->builder, capacity, factor, "append.grown.capacity");
            new_capacity = LLVMBuildSelect(emitter->builder,
                                           capacity_is_zero,
                                           min_capacity,
                                           grown,
                                           "append.new.capacity");
            /* Growth factor >= 2 covers one append unless arithmetic overflowed. */
            bytes_overflow = LLVMBuildICmp(emitter->builder,
                                           LLVMIntUGT,
                                           new_capacity,
                                           max_before_bytes,
                                           "append.bytes.overflow");
            if (!__LLVM_Emit_Trap_If__(
                    emitter, bytes_overflow, "vector.backing.bytes.overflow"))
                return result;
            byte_count =
                LLVMBuildMul(emitter->builder,
                             new_capacity,
                             LLVMConstInt(i64, (unsigned long long)element_size, 0),
                             "append.backing.bytes");
            new_data = __LLVM_Grow_Sequence_Bytes__(
                emitter, data, capacity,
                LLVMBuildMul(emitter->builder, length,
                             LLVMConstInt(i64, (unsigned long long)element_size, 0),
                             "append.live.bytes"),
                byte_count, element_pointer_type, "append.new.data");
            if (new_data == NULL)
                return result;
            LLVMBuildStore(emitter->builder, new_data, data_field);
            LLVMBuildStore(emitter->builder, new_capacity, capacity_field);
            LLVMBuildBr(emitter->builder, append_block);
        }

        LLVMPositionBuilderAtEnd(emitter->builder, append_block);
        data = LLVMBuildLoad2(
            emitter->builder, element_pointer_type, data_field, "append.current.data");
        {
            /* Stores the element place. */
            LLVMValueRef element_place = LLVMBuildGEP2(
                emitter->builder, element_type, data, &length, 1U, "append.element.place");
            /* Stores the store. */
            LLVMValueRef store =
                LLVMBuildStore(emitter->builder, element.value, element_place);
            LLVMBuildStore(emitter->builder, required, length_field);
            result.value = store;
            result.type = &__LLVM_Void_Type__;
            return result;
        }
    }
    if (builtin == __Name_Builtin_Capacity__ || builtin == __Name_Builtin_Reserve_Exact__ ||
        builtin == __Name_Builtin_Pop__ || builtin == __Name_Builtin_Truncate__)
        return __LLVM_Emit_Vector_Primitive__(emitter, expression, expected, builtin);
    if (builtin == __Name_Builtin_Extend_From__)
        return __LLVM_Emit_Extend_From__(emitter, expression);
    if (builtin == __Name_Builtin_Wrapping_Add__ || builtin == __Name_Builtin_Wrapping_Subtract__ ||
        builtin == __Name_Builtin_Wrapping_Multiply__ || builtin == __Name_Builtin_Wrapping_Shift_Left__ ||
        builtin == __Name_Builtin_Wrapping_Shift_Right__)
        return __LLVM_Emit_Wrapping__(emitter, expression, expected, builtin);
    if (builtin == __Name_Builtin_Atomic_Load__ || builtin == __Name_Builtin_Atomic_Store__ ||
        builtin == __Name_Builtin_Atomic_Swap__ || builtin == __Name_Builtin_Atomic_Compare_Swap__ ||
        builtin == __Name_Builtin_Atomic_Fetch_Add__ || builtin == __Name_Builtin_Atomic_Fetch_And__ ||
        builtin == __Name_Builtin_Atomic_Fetch_Or__ || builtin == __Name_Builtin_Atomic_Fetch_Xor__ ||
        builtin == __Name_Builtin_Memory_Fence__)
        return __LLVM_Emit_Atomic__(emitter, expression, expected, builtin);
    if (builtin == __Name_Builtin_Machine_Memory_Read__ || builtin == __Name_Builtin_Machine_Memory_Write__)
        return __LLVM_Emit_Machine_Memory__(emitter, expression, expected, builtin);
    if (builtin == __Name_Builtin_Linear_Memory_Grow__ || builtin == __Name_Builtin_Linear_Memory_Read__ ||
        builtin == __Name_Builtin_Linear_Memory_Write__ || builtin == __Name_Builtin_Linear_Memory_Copy__ ||
        builtin == __Name_Builtin_Linear_Heap_Base__)
        return __LLVM_Emit_Linear_Memory__(emitter, expression, expected, builtin);
    if (builtin == __Name_Builtin_Float_Sqrt__ || builtin == __Name_Builtin_Float_Abs__ ||
        builtin == __Name_Builtin_Float_Floor__ || builtin == __Name_Builtin_Float_Ceil__ ||
        builtin == __Name_Builtin_Float_Trunc__ || builtin == __Name_Builtin_Float_Round_Even__ ||
        builtin == __Name_Builtin_Float_Minimum__ || builtin == __Name_Builtin_Float_Maximum__ ||
        builtin == __Name_Builtin_Float_Copy_Sign__ || builtin == __Name_Builtin_Float_Fused_Multiply_Add__)
        return __LLVM_Emit_Float_Operation__(emitter, expression, expected, builtin);
    if (builtin != __Name_Builtin_None__)
    {
        __LLVM_Fail__("canonical builtin is outside the native runtime lowering boundary");
        return result;
    }
    return result;
}

/* Resolves the LLVM builtin tagged construct. */
int __LLVM_Resolve_Builtin_Tagged_Construct__(__Ast_Type__ *target_type,
                                                     __Ast_Expression__ *expression,
                                                     __Type_Tagged_Constructor__ *out_constructor)
{
    /* References the function. */
    __Ast_Lvalue__ *function;
    /* References the parent. */
    __Ast_Lvalue__ *parent;
    if (target_type == NULL || expression == NULL ||
        expression->__Kind__ != __Ast_Expression_Call__ || !__Type_Is_Builtin_Tagged__(target_type))
    {
        return 0;
    }
    function = expression->__As__.__Call__.__Function__;
    if (function == NULL || function->__Kind__ != __Ast_Lvalue_Field__)
    {
        return 0;
    }
    parent = function->__As__.__Field__.__Parent__;
    if (parent == NULL || parent->__Kind__ != __Ast_Lvalue_Base__ ||
        parent->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__ ||
        !__Type_Tagged_Name_Matches__(target_type, parent->__As__.__Base__.__As__.__Identifier__))
    {
        return 0;
    }
    return __Type_Tagged_Find_Constructor__(
        target_type, function->__As__.__Field__.__Field__, out_constructor);
}
