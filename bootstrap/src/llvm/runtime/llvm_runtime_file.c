/* Owns existing Stage0 LLVM runtime file bridges. */

#include "llvm/runtime/llvm_runtime_file.h"
#include "llvm/llvm_builtins.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "llvm/runtime/llvm_runtime_memory.h"
#include "kernel/type/type.h"

/* Emits the LLVM runtime open file service. */
__LLVM_Value__ __LLVM_Emit_Runtime_Open_File_Service__(__LLVM_Emitter__ *emitter,
                                                              __Ast_Expression__ *expression,
                                                              __Ast_Type__ *expected,
                                                              int create_for_write)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the path. */
    __LLVM_Value__ path;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[3] = {i8_pointer, i32, i32};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the C path string. */
    LLVMValueRef c_path;
    /* Stores the call arguments. */
    LLVMValueRef arguments[3];
    /* Stores the descriptor 32. */
    LLVMValueRef descriptor32;
    /* Stores the descriptor 64. */
    LLVMValueRef descriptor64;
    /* Tracks the failed state. */
    LLVMValueRef failed;
#if defined(__APPLE__)
    const unsigned write_flags = 1U | 512U | 1024U;
#else
    /* Stores the write flags. */
    const unsigned write_flags = 1U | 64U | 512U;
#endif

    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("low-level open service disagrees with canonical builtin arity");
        return result;
    }
    path = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_String_Type__);
    if (path.value == NULL)
        return result;
    c_path = __LLVM_Runtime_C_String__(emitter, path.value, "runtime.service.path");
    if (c_path == NULL)
        return result;
    function =
        __LLVM_Declare_Runtime_Function__(emitter, "open", i32, parameters, 3U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] = c_path;
    arguments[1] = LLVMConstInt(i32, create_for_write ? write_flags : 0U, 0);
    arguments[2] = LLVMConstInt(i32, 0666U, 0);
    descriptor32 = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 3U, "runtime.service.open");
    if (!__LLVM_Runtime_Free__(emitter, c_path))
        return result;
    descriptor64 =
        LLVMBuildSExt(emitter->builder, descriptor32, i64, "runtime.service.open.descriptor");
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           descriptor32,
                           LLVMConstInt(i32, 0U, 0),
                           "runtime.service.open.failed");
    result.value = LLVMBuildSelect(emitter->builder,
                                   failed,
                                   LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                   descriptor64,
                                   "runtime.service.open.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime read byte service. */
__LLVM_Value__ __LLVM_Emit_Runtime_Read_Byte_Service__(__LLVM_Emitter__ *emitter,
                                                              __Ast_Expression__ *expression,
                                                              __Ast_Type__ *expected,
                                                              int stdin_mode)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the descriptor. */
    __LLVM_Value__ descriptor;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[3] = {i32, i8_pointer, i64};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the byte slot. */
    LLVMValueRef byte_slot;
    /* Stores the descriptor 32. */
    LLVMValueRef descriptor32;
    /* Stores the call arguments. */
    LLVMValueRef arguments[3];
    /* Stores the count. */
    LLVMValueRef count;
    /* Stores the byte. */
    LLVMValueRef byte;
    /* Stores the byte 64. */
    LLVMValueRef byte64;
    /* Tracks whether the value is one. */
    LLVMValueRef is_one;
    /* Tracks whether the value is EOF. */
    LLVMValueRef is_eof;
    /* Tracks the non success state. */
    LLVMValueRef non_success;

    if (expression->__As__.__Call__.__Argument_Count__ != (stdin_mode ? 0U : 1U))
    {
        __LLVM_Fail__("low-level read-byte service disagrees with canonical builtin arity");
        return result;
    }
    if (stdin_mode)
    {
        descriptor32 = LLVMConstInt(i32, 0U, 0);
    }
    else
    {
        descriptor = __LLVM_Emit_Expression__(
            emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
        if (descriptor.value == NULL)
            return result;
        descriptor = __LLVM_Coerce__(emitter, descriptor, &__LLVM_Integer_Type__);
        if (descriptor.value == NULL)
            return result;
        descriptor32 =
            LLVMBuildTrunc(emitter->builder, descriptor.value, i32, "runtime.service.read.fd");
    }
    byte_slot = __LLVM_Allocate_Stack__(emitter, i8, "runtime.service.read.byte");
    if (byte_slot == NULL)
        return result;
    function =
        __LLVM_Declare_Runtime_Function__(emitter, "read", i64, parameters, 3U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] = descriptor32;
    arguments[1] = LLVMBuildPointerCast(
        emitter->builder, byte_slot, i8_pointer, "runtime.service.read.buffer");
    arguments[2] = LLVMConstInt(i64, 1U, 0);
    count = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 3U, "runtime.service.read.count");
    byte = LLVMBuildLoad2(emitter->builder, i8, byte_slot, "runtime.service.read.value");
    byte64 = LLVMBuildZExt(emitter->builder, byte, i64, "runtime.service.read.byte64");
    is_one = LLVMBuildICmp(
        emitter->builder, LLVMIntEQ, count, LLVMConstInt(i64, 1U, 0), "runtime.service.read.one");
    is_eof = LLVMBuildICmp(
        emitter->builder, LLVMIntEQ, count, LLVMConstInt(i64, 0U, 0), "runtime.service.read.eof");
    non_success = LLVMBuildSelect(emitter->builder,
                                  is_eof,
                                  LLVMConstInt(i64, (unsigned long long)-1LL, 1),
                                  LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                  "runtime.service.read.non.success");
    result.value = LLVMBuildSelect(
        emitter->builder, is_one, byte64, non_success, "runtime.service.read.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime read segment service. */
__LLVM_Value__ __LLVM_Emit_Runtime_Read_Segment_Service__(__LLVM_Emitter__ *emitter,
                                                                 __Ast_Expression__ *expression,
                                                                 __Ast_Type__ *expected,
                                                                 int stdin_mode)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the descriptor. */
    __LLVM_Value__ descriptor = __LLVM_Invalid_Value__();
    /* Stores the bytes. */
    __LLVM_Value__ bytes;
    /* Stores the offset. */
    __LLVM_Value__ offset;
    /* Stores the requested. */
    __LLVM_Value__ requested;
    /* References the bytes type. */
    __Ast_Type__ *bytes_type;
    /* Tracks the bytes index. */
    size_t bytes_index = stdin_mode ? 0U : 1U;
    /* Tracks the offset index. */
    size_t offset_index = stdin_mode ? 1U : 2U;
    /* Tracks the count index. */
    size_t count_index = stdin_mode ? 2U : 3U;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[3] = {i32, i8_pointer, i64};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the descriptor 32. */
    LLVMValueRef descriptor32;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the negative offset. */
    LLVMValueRef negative_offset;
    /* Stores the negative count. */
    LLVMValueRef negative_count;
    /* Stores the offset too large. */
    LLVMValueRef offset_too_large;
    /* Stores the remaining. */
    LLVMValueRef remaining;
    /* Stores the count too large. */
    LLVMValueRef count_too_large;
    /* Stores the invalid. */
    LLVMValueRef invalid;
    /* Stores the status slot. */
    LLVMValueRef status_slot;
    /* Stores the current function. */
    LLVMValueRef current_function;
    /* Stores the invalid block. */
    LLVMBasicBlockRef invalid_block;
    /* Stores the read block. */
    LLVMBasicBlockRef read_block;
    /* Tracks the done block state. */
    LLVMBasicBlockRef done_block;
    /* Stores the pointer. */
    LLVMValueRef pointer;
    /* Stores the call arguments. */
    LLVMValueRef args[3];
    /* Stores the read count. */
    LLVMValueRef read_count;
    /* Tracks the failed state. */
    LLVMValueRef failed;

    if (expression->__As__.__Call__.__Argument_Count__ != (stdin_mode ? 3U : 4U))
    {
        __LLVM_Fail__("low-level read-segment service disagrees with canonical builtin arity");
        return result;
    }

    if (stdin_mode)
    {
        descriptor32 = LLVMConstInt(i32, 0U, 0);
    }
    else
    {
        descriptor = __LLVM_Emit_Expression__(
            emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
        if (descriptor.value == NULL)
            return result;
        descriptor = __LLVM_Coerce__(emitter, descriptor, &__LLVM_Integer_Type__);
        if (descriptor.value == NULL)
            return result;
        descriptor32 = LLVMBuildTrunc(
            emitter->builder, descriptor.value, i32, "runtime.service.read.segment.fd");
    }

    bytes_type =
        __LLVM_Expression_Type__(emitter, expression->__As__.__Call__.__Arguments__[bytes_index]);
    if (bytes_type == NULL)
        return result;
    bytes = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[bytes_index], bytes_type);
    offset = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[offset_index], &__LLVM_Integer_Type__);
    requested = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[count_index], &__LLVM_Integer_Type__);
    if (bytes.value == NULL || offset.value == NULL || requested.value == NULL)
        return result;
    offset = __LLVM_Coerce__(emitter, offset, &__LLVM_Integer_Type__);
    requested = __LLVM_Coerce__(emitter, requested, &__LLVM_Integer_Type__);
    if (offset.value == NULL || requested.value == NULL)
        return result;

    data = LLVMBuildPointerCast(
        emitter->builder,
        LLVMBuildExtractValue(
            emitter->builder, bytes.value, 0U, "runtime.service.read.segment.data"),
        i8_pointer,
        "runtime.service.read.segment.bytes");
    length = LLVMBuildExtractValue(
        emitter->builder, bytes.value, 1U, "runtime.service.read.segment.length");
    negative_offset = LLVMBuildICmp(emitter->builder,
                                    LLVMIntSLT,
                                    offset.value,
                                    LLVMConstInt(i64, 0U, 0),
                                    "runtime.service.read.segment.offset.negative");
    negative_count = LLVMBuildICmp(emitter->builder,
                                   LLVMIntSLT,
                                   requested.value,
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.service.read.segment.count.negative");
    offset_too_large = LLVMBuildICmp(emitter->builder,
                                     LLVMIntUGT,
                                     offset.value,
                                     length,
                                     "runtime.service.read.segment.offset.large");
    remaining = LLVMBuildSub(
        emitter->builder, length, offset.value, "runtime.service.read.segment.remaining");
    count_too_large = LLVMBuildICmp(emitter->builder,
                                    LLVMIntUGT,
                                    requested.value,
                                    remaining,
                                    "runtime.service.read.segment.count.large");
    invalid = LLVMBuildOr(emitter->builder,
                          negative_offset,
                          negative_count,
                          "runtime.service.read.segment.invalid.sign");
    invalid = LLVMBuildOr(
        emitter->builder, invalid, offset_too_large, "runtime.service.read.segment.invalid.offset");
    invalid = LLVMBuildOr(
        emitter->builder, invalid, count_too_large, "runtime.service.read.segment.invalid.count");

    status_slot = __LLVM_Allocate_Stack__(emitter, i64, "runtime.service.read.segment.status");
    if (status_slot == NULL)
        return result;
    current_function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    if (current_function == NULL)
        return result;
    invalid_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.read.segment.invalid");
    read_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.read.segment.valid");
    done_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.read.segment.done");
    LLVMBuildCondBr(emitter->builder, invalid, invalid_block, read_block);

    LLVMPositionBuilderAtEnd(emitter->builder, invalid_block);
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, (unsigned long long)-2LL, 1), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, read_block);
    pointer = LLVMBuildGEP2(
        emitter->builder, i8, data, &offset.value, 1U, "runtime.service.read.segment.pointer");
    function =
        __LLVM_Declare_Runtime_Function__(emitter, "read", i64, parameters, 3U, &function_type);
    if (function == NULL)
        return result;
    args[0] = descriptor32;
    args[1] = pointer;
    args[2] = requested.value;
    read_count = LLVMBuildCall2(
        emitter->builder, function_type, function, args, 3U, "runtime.service.read.segment.count");
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           read_count,
                           LLVMConstInt(i64, 0U, 0),
                           "runtime.service.read.segment.failed");
    read_count = LLVMBuildSelect(emitter->builder,
                                 failed,
                                 LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                 read_count,
                                 "runtime.service.read.segment.result");
    LLVMBuildStore(emitter->builder, read_count, status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    result.value = LLVMBuildLoad2(
        emitter->builder, i64, status_slot, "runtime.service.read.segment.status.value");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime close file service. */
__LLVM_Value__ __LLVM_Emit_Runtime_Close_File_Service__(__LLVM_Emitter__ *emitter,
                                                               __Ast_Expression__ *expression,
                                                               __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the descriptor. */
    __LLVM_Value__ descriptor;
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[1] = {i32};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    /* Stores the status. */
    LLVMValueRef status;
    /* Tracks the failed state. */
    LLVMValueRef failed;
    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("low-level close service disagrees with canonical builtin arity");
        return result;
    }
    descriptor = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
    if (descriptor.value == NULL)
        return result;
    descriptor = __LLVM_Coerce__(emitter, descriptor, &__LLVM_Integer_Type__);
    if (descriptor.value == NULL)
        return result;
    function =
        __LLVM_Declare_Runtime_Function__(emitter, "close", i32, parameters, 1U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] =
        LLVMBuildTrunc(emitter->builder, descriptor.value, i32, "runtime.service.close.fd");
    status = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 1U, "runtime.service.close.status");
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           status,
                           LLVMConstInt(i32, 0U, 0),
                           "runtime.service.close.failed");
    result.value = LLVMBuildSelect(emitter->builder,
                                   failed,
                                   LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.service.close.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime write segment service. */
__LLVM_Value__ __LLVM_Emit_Runtime_Write_Segment_Service__(__LLVM_Emitter__ *emitter,
                                                                  __Ast_Expression__ *expression,
                                                                  __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the descriptor. */
    __LLVM_Value__ descriptor;
    /* Stores the bytes. */
    __LLVM_Value__ bytes;
    /* Stores the offset. */
    __LLVM_Value__ offset;
    /* Stores the requested. */
    __LLVM_Value__ requested;
    /* References the bytes type. */
    __Ast_Type__ *bytes_type;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[3] = {i32, i8_pointer, i64};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the negative offset. */
    LLVMValueRef negative_offset;
    /* Stores the negative count. */
    LLVMValueRef negative_count;
    /* Stores the offset too large. */
    LLVMValueRef offset_too_large;
    /* Stores the remaining. */
    LLVMValueRef remaining;
    /* Stores the count too large. */
    LLVMValueRef count_too_large;
    /* Stores the invalid. */
    LLVMValueRef invalid;
    /* Stores the status slot. */
    LLVMValueRef status_slot;
    /* Stores the current function. */
    LLVMValueRef current_function;
    /* Stores the invalid block. */
    LLVMBasicBlockRef invalid_block;
    /* Stores the write block. */
    LLVMBasicBlockRef write_block;
    /* Tracks the done block state. */
    LLVMBasicBlockRef done_block;
    /* Stores the pointer. */
    LLVMValueRef pointer;
    /* Stores the call arguments. */
    LLVMValueRef args[3];
    /* Stores the written. */
    LLVMValueRef written;
    /* Tracks the failed state. */
    LLVMValueRef failed;

    if (expression->__As__.__Call__.__Argument_Count__ != 4U)
    {
        __LLVM_Fail__("low-level write-segment service disagrees with canonical builtin arity");
        return result;
    }
    descriptor = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
    bytes_type = __LLVM_Expression_Type__(emitter, expression->__As__.__Call__.__Arguments__[1]);
    if (descriptor.value == NULL || bytes_type == NULL)
        return result;
    bytes =
        __LLVM_Emit_Expression__(emitter, expression->__As__.__Call__.__Arguments__[1], bytes_type);
    offset = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[2], &__LLVM_Integer_Type__);
    requested = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[3], &__LLVM_Integer_Type__);
    if (bytes.value == NULL || offset.value == NULL || requested.value == NULL)
        return result;
    descriptor = __LLVM_Coerce__(emitter, descriptor, &__LLVM_Integer_Type__);
    offset = __LLVM_Coerce__(emitter, offset, &__LLVM_Integer_Type__);
    requested = __LLVM_Coerce__(emitter, requested, &__LLVM_Integer_Type__);
    if (descriptor.value == NULL || offset.value == NULL || requested.value == NULL)
        return result;
    data = LLVMBuildPointerCast(
        emitter->builder,
        LLVMBuildExtractValue(emitter->builder, bytes.value, 0U, "runtime.service.write.data"),
        i8_pointer,
        "runtime.service.write.bytes");
    length =
        LLVMBuildExtractValue(emitter->builder, bytes.value, 1U, "runtime.service.write.length");
    negative_offset = LLVMBuildICmp(emitter->builder,
                                    LLVMIntSLT,
                                    offset.value,
                                    LLVMConstInt(i64, 0U, 0),
                                    "runtime.service.write.offset.negative");
    negative_count = LLVMBuildICmp(emitter->builder,
                                   LLVMIntSLT,
                                   requested.value,
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.service.write.count.negative");
    offset_too_large = LLVMBuildICmp(
        emitter->builder, LLVMIntUGT, offset.value, length, "runtime.service.write.offset.large");
    remaining =
        LLVMBuildSub(emitter->builder, length, offset.value, "runtime.service.write.remaining");
    count_too_large = LLVMBuildICmp(emitter->builder,
                                    LLVMIntUGT,
                                    requested.value,
                                    remaining,
                                    "runtime.service.write.count.large");
    invalid = LLVMBuildOr(
        emitter->builder, negative_offset, negative_count, "runtime.service.write.invalid.sign");
    invalid = LLVMBuildOr(
        emitter->builder, invalid, offset_too_large, "runtime.service.write.invalid.offset");
    invalid = LLVMBuildOr(
        emitter->builder, invalid, count_too_large, "runtime.service.write.invalid.count");
    status_slot = __LLVM_Allocate_Stack__(emitter, i64, "runtime.service.write.status");
    if (status_slot == NULL)
        return result;
    current_function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    if (current_function == NULL)
        return result;
    invalid_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.write.invalid");
    write_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.write.valid");
    done_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.write.done");
    LLVMBuildCondBr(emitter->builder, invalid, invalid_block, write_block);

    LLVMPositionBuilderAtEnd(emitter->builder, invalid_block);
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, (unsigned long long)-2LL, 1), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, write_block);
    pointer = LLVMBuildGEP2(
        emitter->builder, i8, data, &offset.value, 1U, "runtime.service.write.pointer");
    function =
        __LLVM_Declare_Runtime_Function__(emitter, "write", i64, parameters, 3U, &function_type);
    if (function == NULL)
        return result;
    args[0] = LLVMBuildTrunc(emitter->builder, descriptor.value, i32, "runtime.service.write.fd");
    args[1] = pointer;
    args[2] = requested.value;
    written = LLVMBuildCall2(
        emitter->builder, function_type, function, args, 3U, "runtime.service.write.count");
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           written,
                           LLVMConstInt(i64, 0U, 0),
                           "runtime.service.write.failed");
    written = LLVMBuildSelect(emitter->builder,
                              failed,
                              LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                              written,
                              "runtime.service.write.result");
    LLVMBuildStore(emitter->builder, written, status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    result.value =
        LLVMBuildLoad2(emitter->builder, i64, status_slot, "runtime.service.write.status.value");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime write executable bytes. */
__LLVM_Value__ __LLVM_Emit_Runtime_Write_Executable_Bytes__(__LLVM_Emitter__ *emitter,
                                                                   __Ast_Expression__ *expression,
                                                                   __Ast_Type__ *result_type)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the path. */
    __LLVM_Value__ path;
    /* Stores the contents. */
    __LLVM_Value__ contents;
    /* References the content type. */
    __Ast_Type__ *content_type;
    /* Stores the content resolved. */
    __Resolved_Type__ content_resolved;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the fopen parameters. */
    LLVMTypeRef fopen_parameters[2] = {i8_pointer, i8_pointer};
    /* Stores the fopen type. */
    LLVMTypeRef fopen_type;
    /* Stores the fopen function. */
    LLVMValueRef fopen_function;
    /* Stores the fwrite parameters. */
    LLVMTypeRef fwrite_parameters[4] = {i8_pointer, i64, i64, i8_pointer};
    /* Stores the fwrite type. */
    LLVMTypeRef fwrite_type;
    /* Stores the fwrite function. */
    LLVMValueRef fwrite_function;
    /* Stores the fclose parameters. */
    LLVMTypeRef fclose_parameters[1] = {i8_pointer};
    /* Stores the fclose type. */
    LLVMTypeRef fclose_type;
    /* Stores the fclose function. */
    LLVMValueRef fclose_function;
    /* Stores the unlink parameters. */
    LLVMTypeRef unlink_parameters[1] = {i8_pointer};
    /* Stores the unlink type. */
    LLVMTypeRef unlink_type = NULL;
    /* Stores the unlink function. */
    LLVMValueRef unlink_function = NULL;
    /* Stores the chmod parameters. */
    LLVMTypeRef chmod_parameters[2] = {i8_pointer, i32};
    /* Stores the chmod type. */
    LLVMTypeRef chmod_type = NULL;
    /* Stores the chmod function. */
    LLVMValueRef chmod_function = NULL;
    /* Stores the C path string. */
    LLVMValueRef c_path;
    /* Stores the mode. */
    LLVMValueRef mode;
    /* Stores the file. */
    LLVMValueRef file;
    /* Tracks whether the file pointer is null. */
    LLVMValueRef file_is_null;
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the status slot. */
    LLVMValueRef status_slot;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the write block. */
    LLVMBasicBlockRef write_block;
    /* Tracks the done block state. */
    LLVMBasicBlockRef done_block;
    /* Stores the written. */
    LLVMValueRef written;
    /* Stores the close result. */
    LLVMValueRef close_result;
    /* Stores the complete. */
    LLVMValueRef complete;
    /* Stores the closed. */
    LLVMValueRef closed;
    /* Tracks the success condition state. */
    LLVMValueRef success_condition;
    /* Stores the status. */
    LLVMValueRef status;
    /* Tracks the success state. */
    __LLVM_Value__ success;

    if (expression->__As__.__Call__.__Argument_Count__ != 2U)
    {
        __LLVM_Fail__("write_executable_bytes disagrees with canonical builtin arity");
        return result;
    }
    path = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_String_Type__);
    if (path.value == NULL)
        return result;
    content_type = __LLVM_Expression_Type__(emitter, expression->__As__.__Call__.__Arguments__[1]);
    if (content_type == NULL ||
        !__Type_Resolve__(emitter->semantic, content_type, &content_resolved) ||
        content_resolved.__Kind__ != __Resolved_Type_Vector__)
    {
        __LLVM_Fail__("write_executable_bytes content disagrees with canonical semantic Type");
        return result;
    }
    contents = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[1], content_type);
    if (contents.value == NULL)
        return result;
    c_path = __LLVM_Runtime_C_String__(emitter, path.value, "runtime.write.path");
    if (c_path == NULL)
        return result;

    fopen_function = __LLVM_Declare_Runtime_Function__(
        emitter, "fopen", i8_pointer, fopen_parameters, 2U, &fopen_type);
    fwrite_function = __LLVM_Declare_Runtime_Function__(
        emitter, "fwrite", i64, fwrite_parameters, 4U, &fwrite_type);
    fclose_function = __LLVM_Declare_Runtime_Function__(
        emitter, "fclose", i32, fclose_parameters, 1U, &fclose_type);
    unlink_function = __LLVM_Declare_Runtime_Function__(
        emitter, "unlink", i32, unlink_parameters, 1U, &unlink_type);
    chmod_function =
        __LLVM_Declare_Runtime_Function__(emitter, "chmod", i32, chmod_parameters, 2U, &chmod_type);
    if (fopen_function == NULL || fwrite_function == NULL || fclose_function == NULL ||
        unlink_function == NULL || chmod_function == NULL)
        return result;

    data = LLVMBuildPointerCast(
        emitter->builder,
        LLVMBuildExtractValue(emitter->builder, contents.value, 0U, "runtime.write.data"),
        i8_pointer,
        "runtime.write.bytes");
    length = LLVMBuildExtractValue(emitter->builder, contents.value, 1U, "runtime.write.length");
    status_slot = __LLVM_Allocate_Stack__(emitter, i64, "runtime.write.status");
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, (unsigned long long)-1LL, 1), status_slot);
    {
        /* Stores the call arguments. */
        LLVMValueRef arguments[1] = {c_path};
        (void)LLVMBuildCall2(emitter->builder,
                             unlink_type,
                             unlink_function,
                             arguments,
                             1U,
                             "runtime.write.unlink.old");
    }
    mode = __LLVM_Runtime_Literal_C_String__(emitter, "wbx", "runtime.write.mode");
    {
        /* Stores the call arguments. */
        LLVMValueRef arguments[2] = {c_path, mode};
        file = LLVMBuildCall2(
            emitter->builder, fopen_type, fopen_function, arguments, 2U, "runtime.write.file");
    }
    file_is_null = LLVMBuildICmp(
        emitter->builder, LLVMIntEQ, file, LLVMConstNull(i8_pointer), "runtime.write.open.failed");
    function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    if (function == NULL)
        return result;
    write_block = LLVMAppendBasicBlockInContext(emitter->context, function, "runtime.write.opened");
    done_block = LLVMAppendBasicBlockInContext(emitter->context, function, "runtime.write.done");
    LLVMBuildCondBr(emitter->builder, file_is_null, done_block, write_block);

    LLVMPositionBuilderAtEnd(emitter->builder, write_block);
    {
        /* Stores the call arguments. */
        LLVMValueRef arguments[4] = {data, LLVMConstInt(i64, 1U, 0), length, file};
        written = LLVMBuildCall2(
            emitter->builder, fwrite_type, fwrite_function, arguments, 4U, "runtime.write.count");
    }
    {
        /* Stores the call arguments. */
        LLVMValueRef arguments[1] = {file};
        close_result = LLVMBuildCall2(emitter->builder,
                                      fclose_type,
                                      fclose_function,
                                      arguments,
                                      1U,
                                      "runtime.write.close.status");
    }
    complete =
        LLVMBuildICmp(emitter->builder, LLVMIntEQ, written, length, "runtime.write.complete");
    closed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           close_result,
                           LLVMConstInt(i32, 0U, 0),
                           "runtime.write.closed");
    success_condition = LLVMBuildAnd(emitter->builder, complete, closed, "runtime.write.success");
    {
        /* Stores the call arguments. */
        LLVMValueRef arguments[2] = {c_path, LLVMConstInt(i32, 0755U, 0)};
        /* Stores the chmod result. */
        LLVMValueRef chmod_result = LLVMBuildCall2(emitter->builder,
                                                   chmod_type,
                                                   chmod_function,
                                                   arguments,
                                                   2U,
                                                   "runtime.write.chmod.status");
        /* Stores the chmod ok. */
        LLVMValueRef chmod_ok = LLVMBuildICmp(emitter->builder,
                                              LLVMIntEQ,
                                              chmod_result,
                                              LLVMConstInt(i32, 0U, 0),
                                              "runtime.write.chmod.ok");
        success_condition = LLVMBuildAnd(
            emitter->builder, success_condition, chmod_ok, "runtime.write.executable.success");
    }
    status = LLVMBuildSelect(emitter->builder,
                             success_condition,
                             written,
                             LLVMConstInt(i64, (unsigned long long)-1LL, 1),
                             "runtime.write.result");
    LLVMBuildStore(emitter->builder, status, status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    if (!__LLVM_Runtime_Free__(emitter, c_path))
        return result;
    status = LLVMBuildLoad2(emitter->builder, i64, status_slot, "runtime.write.status.value");
    success.value = status;
    success.type = &__LLVM_Integer_Type__;
    return __LLVM_Build_Runtime_Result__(emitter, result_type, status, success);
}
