/* Owns Stage0 LLVM builtin lowering helpers. */

#include "llvm/llvm_builtins.h"
#include "llvm/llvm_aggregates.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "llvm/runtime/llvm_runtime_directory.h"
#include "llvm/runtime/llvm_runtime_file.h"
#include "llvm/runtime/llvm_runtime_memory.h"
#include "llvm/runtime/llvm_runtime_process.h"
#include "llvm/runtime/llvm_runtime_stream.h"
#include "frontend/identifier_identity.h"
#include "kernel/layout/layout.h"
#include "kernel/memory/memory.h"
#include "kernel/name/name.h"
#include "kernel/type/tagged.h"
#include "kernel/type/type.h"
#include <stdint.h>

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
    if (unit == NULL || !__Name_Resolve_Module_Alias_Name__(unit, name, &resolved_name) ||
        __Identifier_Identity_Equals__(resolved_name, name))
        return __Name_Builtin_None__;
    return __Name_Find_Builtin_Function__(resolved_name);
}


/* Emits the LLVM argument. */
static __LLVM_Value__ __LLVM_Emit_Argument__(__LLVM_Emitter__ *emitter,
                                             __Ast_Expression__ *expression,
                                             __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Tracks the index. */
    __LLVM_Value__ index;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the argv type. */
    LLVMTypeRef argv_type = LLVMPointerType(i8_pointer, 0U);
    /* Stores the string type. */
    LLVMTypeRef string_type = __LLVM_Type__(emitter, &__LLVM_String_Type__);
    /* Stores the argument count. */
    LLVMValueRef argc;
    /* References the argument vector. */
    LLVMValueRef argv;
    /* Stores the negative. */
    LLVMValueRef negative;
    /* Stores the too large. */
    LLVMValueRef too_large;
    /* Stores the invalid. */
    LLVMValueRef invalid;
    /* Tracks the host index. */
    LLVMValueRef host_index;
    /* Stores the slot. */
    LLVMValueRef slot;
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the value. */
    LLVMValueRef value;
    if (expression->__As__.__Call__.__Argument_Count__ != 1U ||
        !__LLVM_Ensure_Process_Globals__(emitter) || string_type == NULL)
    {
        __LLVM_Fail__("argument builtin disagrees with canonical semantics");
        return result;
    }
    index = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
    if (index.value == NULL)
        return result;
    index = __LLVM_Coerce__(emitter, index, &__LLVM_Integer_Type__);
    if (index.value == NULL)
        return result;
    argc = LLVMBuildLoad2(emitter->builder, i64, emitter->process_argc_global, "argument.count");
    negative = LLVMBuildICmp(emitter->builder,
                             LLVMIntSLT,
                             index.value,
                             LLVMConstInt(i64, 0U, 0),
                             "argument.index.negative");
    too_large =
        LLVMBuildICmp(emitter->builder, LLVMIntUGE, index.value, argc, "argument.index.large");
    invalid = LLVMBuildOr(emitter->builder, negative, too_large, "argument.index.invalid");
    if (!__LLVM_Emit_Trap_If__(emitter, invalid, "argument.index.out.of.range"))
        return result;
    argv = LLVMBuildLoad2(emitter->builder, argv_type, emitter->process_argv_global, "argv");
    host_index = LLVMBuildAdd(
        emitter->builder, index.value, LLVMConstInt(i64, 1U, 0), "argument.host.index");
    slot = LLVMBuildGEP2(emitter->builder, i8_pointer, argv, &host_index, 1U, "argument.slot");
    data = LLVMBuildLoad2(emitter->builder, i8_pointer, slot, "argument.data");
    length = __LLVM_Strlen__(emitter, data);
    if (length == NULL)
        return result;
    value = LLVMConstNull(string_type);
    value = LLVMBuildInsertValue(emitter->builder, value, data, 0U, "argument.with.data");
    value = LLVMBuildInsertValue(emitter->builder, value, length, 1U, "argument.with.length");
    value = LLVMBuildInsertValue(
        emitter->builder, value, LLVMConstInt(i64, 0U, 0), 2U, "argument.with.capacity");
    result.value = value;
    result.type = &__LLVM_String_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Maps the bytes to the LLVM emit text. */
static __LLVM_Value__ __LLVM_Emit_Text_From_Bytes__(__LLVM_Emitter__ *emitter,
                                                    __Ast_Expression__ *expression,
                                                    __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* References the source type. */
    __Ast_Type__ *source_type;
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    /* Stores the source. */
    __LLVM_Value__ source;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the capacity. */
    LLVMValueRef capacity;
    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("text_from_bytes disagrees with canonical builtin arity");
        return result;
    }
    source_type = __LLVM_Expression_Type__(emitter, expression->__As__.__Call__.__Arguments__[0]);
    if (source_type == NULL || !__Type_Resolve__(emitter->semantic, source_type, &resolved) ||
        resolved.__Kind__ != __Resolved_Type_Vector__)
    {
        __LLVM_Fail__("text_from_bytes requires canonical byte-vector Type");
        return result;
    }
    source = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], source_type);
    if (source.value == NULL)
        return result;
    data = LLVMBuildPointerCast(
        emitter->builder,
        LLVMBuildExtractValue(emitter->builder, source.value, 0U, "text.from.bytes.data"),
        i8_pointer,
        "text.from.bytes.pointer");
    length = LLVMBuildExtractValue(emitter->builder, source.value, 1U, "text.from.bytes.length");
    capacity =
        LLVMBuildExtractValue(emitter->builder, source.value, 2U, "text.from.bytes.capacity");
    result = __LLVM_Make_Text_Value__(
        emitter, data, length, capacity, expected != NULL ? expected : &__LLVM_String_Type__);
    return result;
}

/* Emits the LLVM builtin call. */
__LLVM_Value__ __LLVM_Emit_Builtin_Call__(__LLVM_Emitter__ *emitter,
                                          __Ast_Expression__ *expression,
                                          __Ast_Type__ *expected,
                                          __Ast_Type__ *tagged_target,
                                          __Name_Builtin_Function__ builtin)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    if (builtin == __Name_Builtin_Host_Architecture__ ||
        builtin == __Name_Builtin_Host_Platform__ ||
        builtin == __Name_Builtin_Host_Environment__)
        return __LLVM_Emit_Host_Identity__(emitter, builtin, expected);
    if (builtin == __Name_Builtin_Argument_Count__)
        return __LLVM_Emit_Argument_Count__(emitter, expected);
    if (builtin == __Name_Builtin_Argument__)
        return __LLVM_Emit_Argument__(emitter, expression, expected);
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
        result.type = &__LLVM_Integer_Type__;
        return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
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
            new_data = __LLVM_Reallocate_Bytes__(
                emitter, data, byte_count, element_pointer_type, "append.new.data");
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
    if (builtin == __Name_Builtin_Open_File_Read__)
        return __LLVM_Emit_Runtime_Open_File_Service__(emitter, expression, expected, 0);
    if (builtin == __Name_Builtin_Read_File_Byte__)
        return __LLVM_Emit_Runtime_Read_Byte_Service__(emitter, expression, expected, 0);
    if (builtin == __Name_Builtin_Read_File_Segment__)
        return __LLVM_Emit_Runtime_Read_Segment_Service__(emitter, expression, expected, 0);
    if (builtin == __Name_Builtin_Create_File_Write__)
        return __LLVM_Emit_Runtime_Open_File_Service__(emitter, expression, expected, 1);
    if (builtin == __Name_Builtin_Write_File_Segment__)
        return __LLVM_Emit_Runtime_Write_Segment_Service__(emitter, expression, expected);
    if (builtin == __Name_Builtin_Close_File__)
        return __LLVM_Emit_Runtime_Close_File_Service__(emitter, expression, expected);
    if (builtin == __Name_Builtin_Open_Directory__)
        return __LLVM_Emit_Runtime_Open_Directory_Service__(emitter, expression, expected);
    if (builtin == __Name_Builtin_Read_Directory_Entry__)
        return __LLVM_Emit_Runtime_Read_Directory_Entry_Service__(emitter, expression, expected);
    if (builtin == __Name_Builtin_Close_Directory__)
        return __LLVM_Emit_Runtime_Close_Directory_Service__(emitter, expression, expected);
    if (builtin == __Name_Builtin_Read_Stdin_Byte__)
        return __LLVM_Emit_Runtime_Read_Byte_Service__(emitter, expression, expected, 1);
    if (builtin == __Name_Builtin_Read_Stdin_Segment__)
        return __LLVM_Emit_Runtime_Read_Segment_Service__(emitter, expression, expected, 1);
    if (builtin == __Name_Builtin_Write_Executable_Bytes__)
        return __LLVM_Emit_Runtime_Write_Executable_Bytes__(
            emitter, expression, tagged_target);
    if (builtin == __Name_Builtin_Stdout_Write__)
        return __LLVM_Emit_Runtime_Stream__(emitter, expression, expected, 1);
    if (builtin == __Name_Builtin_Stderr_Write__)
        return __LLVM_Emit_Runtime_Stream__(emitter, expression, expected, 2);
    if (builtin == __Name_Builtin_Process_Exit__)
        return __LLVM_Emit_Runtime_Exit__(emitter, expression);
    if (builtin == __Name_Builtin_Text_From_Bytes__)
        return __LLVM_Emit_Text_From_Bytes__(emitter, expression, expected);
    if (builtin == __Name_Builtin_Path_Type__ ||
        builtin == __Name_Builtin_Path_Size__ ||
        builtin == __Name_Builtin_Path_Modified_Time__)
        return __LLVM_Emit_Runtime_Path_Metadata_Service__(
            emitter, expression, expected, builtin);
    if (builtin != __Name_Builtin_None__)
    {
        __LLVM_Fail__("canonical builtin is outside the native runtime lowering boundary");
        return result;
    }
    return result;
}

/* Builds the LLVM runtime result. */
__LLVM_Value__ __LLVM_Build_Runtime_Result__(__LLVM_Emitter__ *emitter,
                                                    __Ast_Type__ *result_type,
                                                    LLVMValueRef status,
                                                    __LLVM_Value__ success_payload)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    /* Stores the LLVM type. */
    LLVMTypeRef llvm_type;
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the storage. */
    LLVMValueRef storage;
    /* Tracks whether the value is error. */
    LLVMValueRef is_error;
    /* Stores the function. */
    LLVMValueRef function;
    /* Tracks the success block state. */
    LLVMBasicBlockRef success_block;
    /* Stores the error block. */
    LLVMBasicBlockRef error_block;
    /* Stores the merge block. */
    LLVMBasicBlockRef merge_block;
    /* Stores the error payload. */
    __LLVM_Value__ error_payload;

    if (result_type == NULL || status == NULL ||
        !__Type_Resolve__(emitter->semantic, result_type, &resolved) ||
        resolved.__Kind__ != __Resolved_Type_Result__)
    {
        __LLVM_Fail__("L2.8 native runtime primitive requires canonical Result contextual Type");
        return result;
    }
    llvm_type = __LLVM_Type__(emitter, result_type);
    if (llvm_type == NULL)
        return result;
    storage = __LLVM_Allocate_Stack__(emitter, llvm_type, "runtime.result.storage");
    LLVMBuildStore(emitter->builder, LLVMConstNull(llvm_type), storage);
    is_error = LLVMBuildICmp(
        emitter->builder, LLVMIntSLT, status, LLVMConstInt(i64, 0U, 0), "runtime.result.is.error");
    function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    if (function == NULL)
    {
        __LLVM_Fail__("L2.8 runtime Result lost function CFG");
        return result;
    }
    success_block = LLVMAppendBasicBlockInContext(emitter->context, function, "runtime.result.ok");
    error_block = LLVMAppendBasicBlockInContext(emitter->context, function, "runtime.result.err");
    merge_block = LLVMAppendBasicBlockInContext(emitter->context, function, "runtime.result.merge");
    LLVMBuildCondBr(emitter->builder, is_error, error_block, success_block);

    LLVMPositionBuilderAtEnd(emitter->builder, success_block);
    if (!__LLVM_Write_Tagged_Arm__(emitter, result_type, llvm_type, storage, 0U, success_payload))
        return __LLVM_Invalid_Value__();
    LLVMBuildBr(emitter->builder, merge_block);

    LLVMPositionBuilderAtEnd(emitter->builder, error_block);
    error_payload.value = status;
    error_payload.type = &__LLVM_Integer_Type__;
    if (!__LLVM_Write_Tagged_Arm__(emitter, result_type, llvm_type, storage, 1U, error_payload))
        return __LLVM_Invalid_Value__();
    LLVMBuildBr(emitter->builder, merge_block);

    LLVMPositionBuilderAtEnd(emitter->builder, merge_block);
    result.value = LLVMBuildLoad2(emitter->builder, llvm_type, storage, "runtime.result.value");
    result.type = result_type;
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
