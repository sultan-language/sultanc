/* Defines Stage0 implementations for SultanC runtime ABI symbols.
 *
 * Source calls to these symbols are ordinary typed extern calls. This file is a
 * runtime provider: it supplies definitions for the subset required while
 * Stage0 builds the self-hosted compiler. It does not classify source calls as
 * language builtins or convert runtime symbol names back to primitive IDs.
 */

#include "llvm/runtime/llvm_runtime_exports.h"
#include "llvm/llvm_builtins.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_types.h"
#include "llvm/runtime/llvm_runtime_memory.h"
#include "llvm/runtime/llvm_runtime_process.h"
#include <stdint.h>
#include <string.h>

static LLVMValueRef runtime_export(__LLVM_Emitter__ *emitter,
                                   const char *name,
                                   unsigned arity)
{
    LLVMValueRef function = LLVMGetNamedFunction(emitter->module, name);
    if (function == NULL)
        return NULL;
    if (LLVMCountParams(function) != arity)
    {
        __LLVM_Fail__("Stage0 runtime export declaration has the wrong arity");
        return (LLVMValueRef)(uintptr_t)1U;
    }
    if (LLVMCountBasicBlocks(function) != 0U)
    {
        __LLVM_Fail__("Stage0 runtime export was defined more than once");
        return (LLVMValueRef)(uintptr_t)1U;
    }
    LLVMPositionBuilderAtEnd(
        emitter->builder,
        LLVMAppendBasicBlockInContext(emitter->context, function, "entry"));
    return function;
}

static int runtime_export_failed(LLVMValueRef function)
{
    return function == (LLVMValueRef)(uintptr_t)1U;
}

static LLVMValueRef i64_constant(__LLVM_Emitter__ *emitter, long long value)
{
    return LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U),
                        (unsigned long long)value,
                        value < 0);
}

static LLVMValueRef runtime_call(__LLVM_Emitter__ *emitter,
                                 const char *name,
                                 LLVMTypeRef result_type,
                                 LLVMTypeRef *parameter_types,
                                 unsigned parameter_count,
                                 LLVMValueRef *arguments,
                                 const char *value_name)
{
    LLVMTypeRef function_type = NULL;
    LLVMValueRef function = __LLVM_Declare_Runtime_Function__(emitter,
                                                              name,
                                                              result_type,
                                                              parameter_types,
                                                              parameter_count,
                                                              &function_type);
    if (function == NULL)
        return NULL;
    return LLVMBuildCall2(emitter->builder,
                          function_type,
                          function,
                          arguments,
                          parameter_count,
                          value_name);
}

static LLVMValueRef vector_data(__LLVM_Emitter__ *emitter, LLVMValueRef vector, const char *name)
{
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    return LLVMBuildPointerCast(emitter->builder,
                                LLVMBuildExtractValue(emitter->builder, vector, 0U, name),
                                LLVMPointerType(i8, 0U),
                                name);
}

static LLVMValueRef vector_length(__LLVM_Emitter__ *emitter,
                                  LLVMValueRef vector,
                                  const char *name)
{
    return LLVMBuildExtractValue(emitter->builder, vector, 1U, name);
}

static int define_text_from_bytes(__LLVM_Emitter__ *emitter)
{
    LLVMValueRef function = runtime_export(emitter, "تشغيل_نص_من_بايتات", 1U);
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef string_type;
    LLVMValueRef vector;
    LLVMValueRef source;
    LLVMValueRef length;
    LLVMValueRef result_slot;
    LLVMValueRef is_empty;
    LLVMBasicBlockRef empty_block;
    LLVMBasicBlockRef copy_block;
    LLVMBasicBlockRef done_block;
    LLVMValueRef data;
    LLVMValueRef failed;
    LLVMValueRef value;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;

    string_type = LLVMGetReturnType(LLVMGlobalGetValueType(function));
    vector = LLVMGetParam(function, 0U);
    source = vector_data(emitter, vector, "runtime.text.from.bytes.source");
    length = vector_length(emitter, vector, "runtime.text.from.bytes.length");
    result_slot = LLVMBuildAlloca(emitter->builder, string_type, "runtime.text.from.bytes.result");
    is_empty = LLVMBuildICmp(emitter->builder,
                             LLVMIntEQ,
                             length,
                             LLVMConstInt(i64, 0U, 0),
                             "runtime.text.from.bytes.empty");
    empty_block = LLVMAppendBasicBlockInContext(emitter->context, function, "empty");
    copy_block = LLVMAppendBasicBlockInContext(emitter->context, function, "copy");
    done_block = LLVMAppendBasicBlockInContext(emitter->context, function, "done");
    LLVMBuildCondBr(emitter->builder, is_empty, empty_block, copy_block);

    LLVMPositionBuilderAtEnd(emitter->builder, empty_block);
    LLVMBuildStore(emitter->builder, LLVMConstNull(string_type), result_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, copy_block);
    data = __LLVM_Runtime_Malloc__(
        emitter, length, "runtime.text.from.bytes.data");
    if (data == NULL)
        return 0;
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           data,
                           LLVMConstNull(i8_pointer),
                           "runtime.text.from.bytes.allocate.failed");
    if (!__LLVM_Emit_Trap_If__(emitter, failed, "runtime.text.from.bytes.allocate.failure"))
        return 0;
    if (LLVMBuildMemCpy(emitter->builder, data, 1U, source, 1U, length) == NULL)
        return __LLVM_Fail__("Stage0 text-from-bytes copy failed");
    value = LLVMConstNull(string_type);
    value = LLVMBuildInsertValue(
        emitter->builder, value, data, 0U, "runtime.text.from.bytes.with.data");
    value = LLVMBuildInsertValue(
        emitter->builder, value, length, 1U, "runtime.text.from.bytes.with.length");
    value = LLVMBuildInsertValue(
        emitter->builder, value, length, 2U, "runtime.text.from.bytes.with.capacity");
    LLVMBuildStore(emitter->builder, value, result_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    value = LLVMBuildLoad2(
        emitter->builder, string_type, result_slot, "runtime.text.from.bytes.value");
    LLVMBuildRet(emitter->builder, value);
    return 1;
}

static LLVMValueRef checked_segment_status(__LLVM_Emitter__ *emitter,
                                           LLVMValueRef function,
                                           LLVMValueRef descriptor,
                                           LLVMValueRef vector,
                                           LLVMValueRef offset,
                                           LLVMValueRef requested,
                                           int write_mode,
                                           const char *prefix)
{
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef parameters[3] = {i32, i8_pointer, i64};
    LLVMValueRef data = vector_data(emitter, vector, prefix);
    LLVMValueRef length = vector_length(emitter, vector, prefix);
    LLVMValueRef negative_offset = LLVMBuildICmp(
        emitter->builder, LLVMIntSLT, offset, i64_constant(emitter, 0), "runtime.segment.offset.negative");
    LLVMValueRef negative_count = LLVMBuildICmp(
        emitter->builder, LLVMIntSLT, requested, i64_constant(emitter, 0), "runtime.segment.count.negative");
    LLVMValueRef offset_too_large = LLVMBuildICmp(
        emitter->builder, LLVMIntUGT, offset, length, "runtime.segment.offset.large");
    LLVMValueRef remaining = LLVMBuildSub(emitter->builder, length, offset, "runtime.segment.remaining");
    LLVMValueRef count_too_large = LLVMBuildICmp(
        emitter->builder, LLVMIntUGT, requested, remaining, "runtime.segment.count.large");
    LLVMValueRef invalid = LLVMBuildOr(emitter->builder,
                                       negative_offset,
                                       negative_count,
                                       "runtime.segment.invalid.sign");
    LLVMValueRef status_slot = LLVMBuildAlloca(emitter->builder, i64, "runtime.segment.status");
    LLVMBasicBlockRef invalid_block;
    LLVMBasicBlockRef io_block;
    LLVMBasicBlockRef done_block;
    LLVMValueRef pointer;
    LLVMValueRef arguments[3];
    LLVMValueRef status;
    LLVMValueRef failed;

    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          offset_too_large,
                          "runtime.segment.invalid.offset");
    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          count_too_large,
                          "runtime.segment.invalid.count");
    invalid_block = LLVMAppendBasicBlockInContext(emitter->context, function, "invalid");
    io_block = LLVMAppendBasicBlockInContext(emitter->context, function, "io");
    done_block = LLVMAppendBasicBlockInContext(emitter->context, function, "done");
    LLVMBuildCondBr(emitter->builder, invalid, invalid_block, io_block);

    LLVMPositionBuilderAtEnd(emitter->builder, invalid_block);
    LLVMBuildStore(emitter->builder, i64_constant(emitter, -2), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, io_block);
    pointer = LLVMBuildGEP2(emitter->builder, i8, data, &offset, 1U, "runtime.segment.pointer");
    arguments[0] = LLVMBuildTrunc(emitter->builder, descriptor, i32, "runtime.segment.fd");
    arguments[1] = pointer;
    arguments[2] = requested;
    status = runtime_call(emitter,
                          write_mode ? "write" : "read",
                          i64,
                          parameters,
                          3U,
                          arguments,
                          "runtime.segment.count");
    if (status == NULL)
        return NULL;
    failed = LLVMBuildICmp(
        emitter->builder, LLVMIntSLT, status, i64_constant(emitter, 0), "runtime.segment.failed");
    status = LLVMBuildSelect(emitter->builder,
                             failed,
                             i64_constant(emitter, -2),
                             status,
                             "runtime.segment.result");
    LLVMBuildStore(emitter->builder, status, status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    return LLVMBuildLoad2(emitter->builder, i64, status_slot, "runtime.segment.status.value");
}

static int define_open_file(__LLVM_Emitter__ *emitter, const char *name, int create_for_write)
{
    LLVMValueRef function = runtime_export(emitter, name, 1U);
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef parameters[3] = {i8_pointer, i32, i32};
    LLVMValueRef c_path;
    LLVMValueRef arguments[3];
    LLVMValueRef descriptor32;
    LLVMValueRef descriptor64;
    LLVMValueRef failed;
    unsigned write_flags;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    c_path = __LLVM_Runtime_C_String__(emitter, LLVMGetParam(function, 0U), "runtime.file.path");
    if (c_path == NULL)
        return 0;
    write_flags = emitter->target.platform == __Bootstrap_Target_Platform_Darwin__
                      ? (1U | 512U | 1024U)
                      : (1U | 64U | 512U);
    arguments[0] = c_path;
    arguments[1] = LLVMConstInt(i32, create_for_write ? write_flags : 0U, 0);
    arguments[2] = LLVMConstInt(i32, 0666U, 0);
    descriptor32 = runtime_call(
        emitter, "open", i32, parameters, 3U, arguments, "runtime.file.open");
    if (descriptor32 == NULL || !__LLVM_Runtime_Free__(emitter, c_path))
        return 0;
    descriptor64 = LLVMBuildSExt(emitter->builder, descriptor32, i64, "runtime.file.descriptor");
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           descriptor32,
                           LLVMConstInt(i32, 0U, 0),
                           "runtime.file.open.failed");
    LLVMBuildRet(emitter->builder,
                 LLVMBuildSelect(emitter->builder,
                                 failed,
                                 i64_constant(emitter, -2),
                                 descriptor64,
                                 "runtime.file.open.result"));
    return 1;
}

static int define_read_byte(__LLVM_Emitter__ *emitter, const char *name, int stdin_mode)
{
    LLVMValueRef function = runtime_export(emitter, name, stdin_mode ? 0U : 1U);
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef parameters[3] = {i32, i8_pointer, i64};
    LLVMValueRef byte_slot;
    LLVMValueRef arguments[3];
    LLVMValueRef count;
    LLVMValueRef byte;
    LLVMValueRef is_one;
    LLVMValueRef is_eof;
    LLVMValueRef non_success;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    byte_slot = LLVMBuildAlloca(emitter->builder, i8, "runtime.read.byte");
    arguments[0] = stdin_mode
                       ? LLVMConstInt(i32, 0U, 0)
                       : LLVMBuildTrunc(emitter->builder,
                                        LLVMGetParam(function, 0U),
                                        i32,
                                        "runtime.read.fd");
    arguments[1] = LLVMBuildPointerCast(emitter->builder, byte_slot, i8_pointer, "runtime.read.ptr");
    arguments[2] = LLVMConstInt(i64, 1U, 0);
    count = runtime_call(emitter, "read", i64, parameters, 3U, arguments, "runtime.read.count");
    if (count == NULL)
        return 0;
    byte = LLVMBuildZExt(emitter->builder,
                         LLVMBuildLoad2(emitter->builder, i8, byte_slot, "runtime.read.byte.value"),
                         i64,
                         "runtime.read.byte.i64");
    is_one = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           count,
                           LLVMConstInt(i64, 1U, 0),
                           "runtime.read.one");
    is_eof = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           count,
                           LLVMConstInt(i64, 0U, 0),
                           "runtime.read.eof");
    non_success = LLVMBuildSelect(emitter->builder,
                                  is_eof,
                                  i64_constant(emitter, -1),
                                  i64_constant(emitter, -2),
                                  "runtime.read.non.success");
    LLVMBuildRet(emitter->builder,
                 LLVMBuildSelect(
                     emitter->builder, is_one, byte, non_success, "runtime.read.result"));
    return 1;
}

static int define_segment(__LLVM_Emitter__ *emitter,
                          const char *name,
                          int stdin_mode,
                          int write_mode)
{
    unsigned arity = stdin_mode ? 3U : 4U;
    LLVMValueRef function = runtime_export(emitter, name, arity);
    LLVMValueRef descriptor;
    LLVMValueRef vector;
    LLVMValueRef offset;
    LLVMValueRef requested;
    LLVMValueRef status;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    descriptor = stdin_mode ? i64_constant(emitter, 0) : LLVMGetParam(function, 0U);
    vector = LLVMGetParam(function, stdin_mode ? 0U : 1U);
    offset = LLVMGetParam(function, stdin_mode ? 1U : 2U);
    requested = LLVMGetParam(function, stdin_mode ? 2U : 3U);
    status = checked_segment_status(
        emitter, function, descriptor, vector, offset, requested, write_mode, "runtime.segment.data");
    if (status == NULL)
        return 0;
    LLVMBuildRet(emitter->builder, status);
    return 1;
}

static int define_close_file(__LLVM_Emitter__ *emitter)
{
    LLVMValueRef function = runtime_export(emitter, "تشغيل_إغلاق_ملف", 1U);
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    LLVMTypeRef parameters[1] = {i32};
    LLVMValueRef arguments[1];
    LLVMValueRef status;
    LLVMValueRef failed;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    arguments[0] = LLVMBuildTrunc(
        emitter->builder, LLVMGetParam(function, 0U), i32, "runtime.close.fd");
    status = runtime_call(emitter, "close", i32, parameters, 1U, arguments, "runtime.close.status");
    if (status == NULL)
        return 0;
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           status,
                           LLVMConstInt(i32, 0U, 0),
                           "runtime.close.failed");
    LLVMBuildRet(emitter->builder,
                 LLVMBuildSelect(emitter->builder,
                                 failed,
                                 i64_constant(emitter, -2),
                                 i64_constant(emitter, 0),
                                 "runtime.close.result"));
    return 1;
}

static int define_stream(__LLVM_Emitter__ *emitter, const char *name, unsigned descriptor)
{
    LLVMValueRef function = runtime_export(emitter, name, 1U);
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef parameters[3] = {i32, i8_pointer, i64};
    LLVMValueRef text;
    LLVMValueRef length;
    LLVMValueRef arguments[3];
    LLVMValueRef written;
    LLVMValueRef complete;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    text = LLVMGetParam(function, 0U);
    length = LLVMBuildExtractValue(emitter->builder, text, 1U, "runtime.stream.length");
    arguments[0] = LLVMConstInt(i32, descriptor, 0);
    arguments[1] = LLVMBuildPointerCast(
        emitter->builder,
        LLVMBuildExtractValue(emitter->builder, text, 0U, "runtime.stream.data"),
        i8_pointer,
        "runtime.stream.ptr");
    arguments[2] = length;
    written = runtime_call(emitter, "write", i64, parameters, 3U, arguments, "runtime.stream.written");
    if (written == NULL)
        return 0;
    complete = LLVMBuildICmp(
        emitter->builder, LLVMIntEQ, written, length, "runtime.stream.complete");
    LLVMBuildRet(emitter->builder,
                 LLVMBuildSelect(emitter->builder,
                                 complete,
                                 written,
                                 i64_constant(emitter, -1),
                                 "runtime.stream.result"));
    return 1;
}

static int define_open_directory(__LLVM_Emitter__ *emitter)
{
    LLVMValueRef function = runtime_export(emitter, "تشغيل_فتح_دليل", 1U);
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef parameters[1] = {i8_pointer};
    LLVMValueRef c_path;
    LLVMValueRef arguments[1];
    LLVMValueRef directory;
    LLVMValueRef failed;
    LLVMValueRef handle;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    c_path = __LLVM_Runtime_C_String__(
        emitter, LLVMGetParam(function, 0U), "runtime.directory.path");
    if (c_path == NULL)
        return 0;
    arguments[0] = c_path;
    directory = runtime_call(
        emitter, "opendir", i8_pointer, parameters, 1U, arguments, "runtime.directory.open");
    if (directory == NULL || !__LLVM_Runtime_Free__(emitter, c_path))
        return 0;
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           directory,
                           LLVMConstNull(i8_pointer),
                           "runtime.directory.open.failed");
    handle = LLVMBuildPtrToInt(emitter->builder, directory, i64, "runtime.directory.handle");
    LLVMBuildRet(emitter->builder,
                 LLVMBuildSelect(emitter->builder,
                                 failed,
                                 i64_constant(emitter, -2),
                                 handle,
                                 "runtime.directory.open.result"));
    return 1;
}

static int define_close_directory(__LLVM_Emitter__ *emitter)
{
    LLVMValueRef function = runtime_export(emitter, "تشغيل_إغلاق_دليل", 1U);
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    LLVMTypeRef parameters[1] = {i8_pointer};
    LLVMValueRef arguments[1];
    LLVMValueRef status;
    LLVMValueRef failed;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    arguments[0] = LLVMBuildIntToPtr(
        emitter->builder, LLVMGetParam(function, 0U), i8_pointer, "runtime.directory.pointer");
    status = runtime_call(
        emitter, "closedir", i32, parameters, 1U, arguments, "runtime.directory.close.status");
    if (status == NULL)
        return 0;
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           status,
                           LLVMConstInt(i32, 0U, 0),
                           "runtime.directory.close.failed");
    LLVMBuildRet(emitter->builder,
                 LLVMBuildSelect(emitter->builder,
                                 failed,
                                 i64_constant(emitter, -2),
                                 i64_constant(emitter, 0),
                                 "runtime.directory.close.result"));
    return 1;
}

static int define_read_directory(__LLVM_Emitter__ *emitter)
{
    LLVMValueRef function = runtime_export(emitter, "تشغيل_قراءة_مدخل_دليل", 4U);
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef parameters[1] = {i8_pointer};
    LLVMValueRef handle;
    LLVMValueRef vector;
    LLVMValueRef offset;
    LLVMValueRef requested;
    LLVMValueRef data;
    LLVMValueRef length;
    LLVMValueRef invalid;
    LLVMValueRef remaining;
    LLVMValueRef status_slot;
    LLVMBasicBlockRef invalid_block;
    LLVMBasicBlockRef read_block;
    LLVMBasicBlockRef eof_block;
    LLVMBasicBlockRef entry_block;
    LLVMBasicBlockRef name_error_block;
    LLVMBasicBlockRef copy_block;
    LLVMBasicBlockRef done_block;
    LLVMValueRef arguments[1];
    LLVMValueRef entry;
    LLVMValueRef name_pointer;
    LLVMValueRef name_length;
    LLVMValueRef name_invalid;
    LLVMValueRef destination;
    LLVMValueRef type_pointer;
    LLVMValueRef raw_type;
    LLVMValueRef kind;
    LLVMValueRef encoded;
    unsigned name_offset;
    unsigned type_offset;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    handle = LLVMGetParam(function, 0U);
    vector = LLVMGetParam(function, 1U);
    offset = LLVMGetParam(function, 2U);
    requested = LLVMGetParam(function, 3U);
    data = vector_data(emitter, vector, "runtime.directory.data");
    length = vector_length(emitter, vector, "runtime.directory.length");
    remaining = LLVMBuildSub(emitter->builder, length, offset, "runtime.directory.remaining");
    invalid = LLVMBuildICmp(
        emitter->builder, LLVMIntSLE, handle, i64_constant(emitter, 0), "runtime.directory.bad.handle");
    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          LLVMBuildICmp(emitter->builder,
                                        LLVMIntSLT,
                                        offset,
                                        i64_constant(emitter, 0),
                                        "runtime.directory.bad.offset"),
                          "runtime.directory.invalid.offset");
    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          LLVMBuildICmp(emitter->builder,
                                        LLVMIntSLT,
                                        requested,
                                        i64_constant(emitter, 0),
                                        "runtime.directory.bad.count"),
                          "runtime.directory.invalid.count.sign");
    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          LLVMBuildICmp(emitter->builder,
                                        LLVMIntUGT,
                                        offset,
                                        length,
                                        "runtime.directory.offset.large"),
                          "runtime.directory.invalid.offset.large");
    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          LLVMBuildICmp(emitter->builder,
                                        LLVMIntUGT,
                                        requested,
                                        remaining,
                                        "runtime.directory.count.large"),
                          "runtime.directory.invalid.count.large");
    status_slot = LLVMBuildAlloca(emitter->builder, i64, "runtime.directory.status");
    invalid_block = LLVMAppendBasicBlockInContext(emitter->context, function, "invalid");
    read_block = LLVMAppendBasicBlockInContext(emitter->context, function, "read");
    eof_block = LLVMAppendBasicBlockInContext(emitter->context, function, "eof");
    entry_block = LLVMAppendBasicBlockInContext(emitter->context, function, "entry.value");
    name_error_block = LLVMAppendBasicBlockInContext(emitter->context, function, "name.error");
    copy_block = LLVMAppendBasicBlockInContext(emitter->context, function, "copy");
    done_block = LLVMAppendBasicBlockInContext(emitter->context, function, "done");
    LLVMBuildCondBr(emitter->builder, invalid, invalid_block, read_block);

    LLVMPositionBuilderAtEnd(emitter->builder, invalid_block);
    LLVMBuildStore(emitter->builder, i64_constant(emitter, -2), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, read_block);
    arguments[0] = LLVMBuildIntToPtr(emitter->builder, handle, i8_pointer, "runtime.directory.ptr");
    entry = runtime_call(
        emitter, "readdir", i8_pointer, parameters, 1U, arguments, "runtime.directory.entry");
    if (entry == NULL)
        return 0;
    LLVMBuildCondBr(emitter->builder,
                    LLVMBuildICmp(emitter->builder,
                                  LLVMIntEQ,
                                  entry,
                                  LLVMConstNull(i8_pointer),
                                  "runtime.directory.eof"),
                    eof_block,
                    entry_block);

    LLVMPositionBuilderAtEnd(emitter->builder, eof_block);
    LLVMBuildStore(emitter->builder, i64_constant(emitter, 0), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, entry_block);
    name_offset = emitter->target.platform == __Bootstrap_Target_Platform_Darwin__ ? 21U : 19U;
    type_offset = emitter->target.platform == __Bootstrap_Target_Platform_Darwin__ ? 20U : 18U;
    {
        LLVMValueRef index = LLVMConstInt(i64, name_offset, 0);
        name_pointer = LLVMBuildGEP2(
            emitter->builder, i8, entry, &index, 1U, "runtime.directory.name");
    }
    name_length = __LLVM_Strlen__(emitter, name_pointer);
    if (name_length == NULL)
        return 0;
    name_invalid = LLVMBuildOr(
        emitter->builder,
        LLVMBuildICmp(emitter->builder,
                      LLVMIntUGT,
                      name_length,
                      requested,
                      "runtime.directory.name.large"),
        LLVMBuildICmp(emitter->builder,
                      LLVMIntUGE,
                      name_length,
                      LLVMConstInt(i64, 4096U, 0),
                      "runtime.directory.name.encoding.large"),
        "runtime.directory.name.invalid");
    LLVMBuildCondBr(emitter->builder, name_invalid, name_error_block, copy_block);

    LLVMPositionBuilderAtEnd(emitter->builder, name_error_block);
    LLVMBuildStore(emitter->builder, i64_constant(emitter, -2), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, copy_block);
    destination = LLVMBuildGEP2(
        emitter->builder, i8, data, &offset, 1U, "runtime.directory.destination");
    if (LLVMBuildMemCpy(emitter->builder, destination, 1U, name_pointer, 1U, name_length) == NULL)
        return 0;
    {
        LLVMValueRef index = LLVMConstInt(i64, type_offset, 0);
        type_pointer = LLVMBuildGEP2(
            emitter->builder, i8, entry, &index, 1U, "runtime.directory.type.ptr");
    }
    raw_type = LLVMBuildLoad2(emitter->builder, i8, type_pointer, "runtime.directory.type");
    kind = i64_constant(emitter, 0);
    kind = LLVMBuildSelect(emitter->builder,
                           LLVMBuildICmp(emitter->builder,
                                         LLVMIntEQ,
                                         raw_type,
                                         LLVMConstInt(i8, 10U, 0),
                                         "runtime.directory.symlink"),
                           i64_constant(emitter, 3),
                           kind,
                           "runtime.directory.kind.symlink");
    kind = LLVMBuildSelect(emitter->builder,
                           LLVMBuildICmp(emitter->builder,
                                         LLVMIntEQ,
                                         raw_type,
                                         LLVMConstInt(i8, 4U, 0),
                                         "runtime.directory.dir"),
                           i64_constant(emitter, 2),
                           kind,
                           "runtime.directory.kind.dir");
    kind = LLVMBuildSelect(emitter->builder,
                           LLVMBuildICmp(emitter->builder,
                                         LLVMIntEQ,
                                         raw_type,
                                         LLVMConstInt(i8, 8U, 0),
                                         "runtime.directory.regular"),
                           i64_constant(emitter, 1),
                           kind,
                           "runtime.directory.kind.regular");
    encoded = LLVMBuildAdd(emitter->builder,
                           LLVMBuildShl(emitter->builder,
                                        kind,
                                        LLVMConstInt(i64, 12U, 0),
                                        "runtime.directory.kind.encoded"),
                           name_length,
                           "runtime.directory.result");
    LLVMBuildStore(emitter->builder, encoded, status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    LLVMBuildRet(emitter->builder,
                 LLVMBuildLoad2(emitter->builder, i64, status_slot, "runtime.directory.status.value"));
    return 1;
}

static int define_path_metadata(__LLVM_Emitter__ *emitter, const char *name, unsigned kind)
{
    LLVMValueRef function = runtime_export(emitter, name, 1U);
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef stat_type = LLVMArrayType(i8, 144U);
    LLVMTypeRef parameters[2] = {i8_pointer, i8_pointer};
    LLVMValueRef c_path;
    LLVMValueRef stat_buffer;
    LLVMValueRef stat_bytes;
    LLVMValueRef arguments[2];
    LLVMValueRef status;
    LLVMValueRef failed;
    LLVMValueRef value = NULL;
    int darwin;
    unsigned mode_offset;
    unsigned size_offset;
    unsigned sec_offset;
    unsigned nsec_offset;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    c_path = __LLVM_Runtime_C_String__(emitter, LLVMGetParam(function, 0U), "runtime.metadata.path");
    if (c_path == NULL)
        return 0;
    stat_buffer = LLVMBuildAlloca(emitter->builder, stat_type, "runtime.metadata.stat");
    LLVMBuildStore(emitter->builder, LLVMConstNull(stat_type), stat_buffer);
    stat_bytes = LLVMBuildPointerCast(emitter->builder, stat_buffer, i8_pointer, "runtime.metadata.bytes");
    arguments[0] = c_path;
    arguments[1] = stat_bytes;
    status = runtime_call(emitter, "lstat", i32, parameters, 2U, arguments, "runtime.metadata.status");
    if (status == NULL || !__LLVM_Runtime_Free__(emitter, c_path))
        return 0;
    failed = LLVMBuildICmp(
        emitter->builder, LLVMIntSLT, status, LLVMConstInt(i32, 0U, 0), "runtime.metadata.failed");
    darwin = emitter->target.platform == __Bootstrap_Target_Platform_Darwin__;
    mode_offset = darwin ? 4U : 24U;
    size_offset = darwin ? 96U : 48U;
    sec_offset = darwin ? 48U : 88U;
    nsec_offset = darwin ? 56U : 96U;
    if (kind == 0U)
    {
        LLVMValueRef index = LLVMConstInt(i64, mode_offset, 0);
        LLVMValueRef bytes = LLVMBuildGEP2(emitter->builder, i8, stat_bytes, &index, 1U, "runtime.metadata.mode.bytes");
        LLVMTypeRef mode_type = LLVMIntTypeInContext(emitter->context, darwin ? 16U : 32U);
        LLVMValueRef pointer = LLVMBuildPointerCast(
            emitter->builder, bytes, LLVMPointerType(mode_type, 0U), "runtime.metadata.mode.ptr");
        LLVMValueRef mode = LLVMBuildZExt(
            emitter->builder,
            LLVMBuildLoad2(emitter->builder, mode_type, pointer, "runtime.metadata.mode.raw"),
            i64,
            "runtime.metadata.mode");
        LLVMValueRef type = LLVMBuildAnd(
            emitter->builder, mode, LLVMConstInt(i64, 0xF000U, 0), "runtime.metadata.kind");
        value = i64_constant(emitter, 4);
        value = LLVMBuildSelect(emitter->builder,
                                LLVMBuildICmp(emitter->builder, LLVMIntEQ, type, LLVMConstInt(i64, 0xA000U, 0), "runtime.metadata.symlink"),
                                i64_constant(emitter, 3), value, "runtime.metadata.kind.symlink");
        value = LLVMBuildSelect(emitter->builder,
                                LLVMBuildICmp(emitter->builder, LLVMIntEQ, type, LLVMConstInt(i64, 0x4000U, 0), "runtime.metadata.directory"),
                                i64_constant(emitter, 2), value, "runtime.metadata.kind.directory");
        value = LLVMBuildSelect(emitter->builder,
                                LLVMBuildICmp(emitter->builder, LLVMIntEQ, type, LLVMConstInt(i64, 0x8000U, 0), "runtime.metadata.regular"),
                                i64_constant(emitter, 1), value, "runtime.metadata.kind.regular");
    }
    else if (kind == 1U)
    {
        LLVMValueRef index = LLVMConstInt(i64, size_offset, 0);
        LLVMValueRef bytes = LLVMBuildGEP2(emitter->builder, i8, stat_bytes, &index, 1U, "runtime.metadata.size.bytes");
        LLVMValueRef pointer = LLVMBuildPointerCast(
            emitter->builder, bytes, LLVMPointerType(i64, 0U), "runtime.metadata.size.ptr");
        value = LLVMBuildLoad2(emitter->builder, i64, pointer, "runtime.metadata.size");
    }
    else
    {
        LLVMValueRef sec_index = LLVMConstInt(i64, sec_offset, 0);
        LLVMValueRef nsec_index = LLVMConstInt(i64, nsec_offset, 0);
        LLVMValueRef sec_bytes = LLVMBuildGEP2(emitter->builder, i8, stat_bytes, &sec_index, 1U, "runtime.metadata.sec.bytes");
        LLVMValueRef nsec_bytes = LLVMBuildGEP2(emitter->builder, i8, stat_bytes, &nsec_index, 1U, "runtime.metadata.nsec.bytes");
        LLVMValueRef sec_ptr = LLVMBuildPointerCast(emitter->builder, sec_bytes, LLVMPointerType(i64, 0U), "runtime.metadata.sec.ptr");
        LLVMValueRef nsec_ptr = LLVMBuildPointerCast(emitter->builder, nsec_bytes, LLVMPointerType(i64, 0U), "runtime.metadata.nsec.ptr");
        LLVMValueRef seconds = LLVMBuildLoad2(emitter->builder, i64, sec_ptr, "runtime.metadata.sec");
        LLVMValueRef nanoseconds = LLVMBuildLoad2(emitter->builder, i64, nsec_ptr, "runtime.metadata.nsec");
        value = LLVMBuildAdd(emitter->builder,
                             LLVMBuildMul(emitter->builder,
                                          seconds,
                                          LLVMConstInt(i64, 1000000000ULL, 0),
                                          "runtime.metadata.sec.ns"),
                             nanoseconds,
                             "runtime.metadata.mtime.ns");
    }
    LLVMBuildRet(emitter->builder,
                 LLVMBuildSelect(emitter->builder,
                                 failed,
                                 i64_constant(emitter, -2),
                                 value,
                                 "runtime.metadata.result"));
    return 1;
}

static LLVMValueRef error_class(__LLVM_Emitter__ *emitter,
                                LLVMValueRef error,
                                unsigned code,
                                unsigned class_code,
                                LLVMValueRef current)
{
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    return LLVMBuildSelect(emitter->builder,
                           LLVMBuildICmp(emitter->builder,
                                         LLVMIntEQ,
                                         error,
                                         LLVMConstInt(i64, code, 0),
                                         "runtime.error.match"),
                           LLVMConstInt(i64, class_code, 0),
                           current,
                           "runtime.error.class");
}

static int define_error_classification(__LLVM_Emitter__ *emitter)
{
    LLVMValueRef function = runtime_export(emitter, "تشغيل_تصنيف_خطأ_نظام", 1U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMValueRef error;
    LLVMValueRef negative;
    LLVMValueRef normalized;
    LLVMValueRef result;
    unsigned unsupported;
    unsigned name_too_long;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    error = LLVMGetParam(function, 0U);
    negative = LLVMBuildICmp(
        emitter->builder, LLVMIntSLT, error, i64_constant(emitter, 0), "runtime.error.negative");
    normalized = LLVMBuildSelect(emitter->builder,
                                 negative,
                                 LLVMBuildNeg(emitter->builder, error, "runtime.error.absolute"),
                                 error,
                                 "runtime.error.normalized");
    result = LLVMConstInt(i64, 10U, 0);
    result = error_class(emitter, normalized, 4U, 9U, result);   /* EINTR */
    result = error_class(emitter, normalized, 28U, 8U, result);  /* ENOSPC */
    result = error_class(emitter, normalized, 24U, 8U, result);  /* EMFILE */
    result = error_class(emitter, normalized, 23U, 8U, result);  /* ENFILE */
    result = error_class(emitter, normalized, 12U, 8U, result);  /* ENOMEM */
    result = error_class(emitter, normalized, 20U, 7U, result);  /* ENOTDIR */
    result = error_class(emitter, normalized, 21U, 6U, result);  /* EISDIR */
    unsupported = emitter->target.platform == __Bootstrap_Target_Platform_Darwin__ ? 45U : 95U;
    result = error_class(emitter, normalized, unsupported, 5U, result);
    result = error_class(
        emitter,
        normalized,
        emitter->target.platform == __Bootstrap_Target_Platform_Darwin__ ? 78U : 38U,
        5U,
        result); /* ENOSYS */
    name_too_long = emitter->target.platform == __Bootstrap_Target_Platform_Darwin__ ? 63U : 36U;
    result = error_class(emitter, normalized, name_too_long, 4U, result);
    result = error_class(emitter, normalized, 22U, 4U, result);  /* EINVAL */
    result = error_class(emitter, normalized, 17U, 3U, result);  /* EEXIST */
    result = error_class(emitter, normalized, 1U, 2U, result);   /* EPERM */
    result = error_class(emitter, normalized, 13U, 2U, result);  /* EACCES */
    result = error_class(emitter, normalized, 2U, 1U, result);   /* ENOENT */
    result = LLVMBuildSelect(emitter->builder,
                             LLVMBuildICmp(emitter->builder,
                                           LLVMIntEQ,
                                           normalized,
                                           i64_constant(emitter, 0),
                                           "runtime.error.none"),
                             i64_constant(emitter, 0),
                             result,
                             "runtime.error.result");
    LLVMBuildRet(emitter->builder, result);
    return 1;
}

static int define_argument_count(__LLVM_Emitter__ *emitter)
{
    LLVMValueRef function = runtime_export(emitter, "تشغيل_عدد_الوسائط", 0U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function) || !__LLVM_Ensure_Process_Globals__(emitter))
        return 0;
    LLVMBuildRet(emitter->builder,
                 LLVMBuildLoad2(emitter->builder,
                                i64,
                                emitter->process_argc_global,
                                "runtime.argument.count"));
    return 1;
}

static int define_argument(__LLVM_Emitter__ *emitter)
{
    LLVMValueRef function = runtime_export(emitter, "تشغيل_الوصول_إلى_وسيط", 1U);
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    LLVMTypeRef argv_type = LLVMPointerType(i8_pointer, 0U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef string_type;
    LLVMValueRef index;
    LLVMValueRef argc;
    LLVMValueRef invalid;
    LLVMValueRef argv;
    LLVMValueRef host_index;
    LLVMValueRef slot;
    LLVMValueRef data;
    LLVMValueRef length;
    LLVMValueRef value;
    LLVMValueRef abort_function;
    LLVMTypeRef abort_type;
    LLVMBasicBlockRef fail_block;
    LLVMBasicBlockRef ok_block;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function) || !__LLVM_Ensure_Process_Globals__(emitter))
        return 0;
    string_type = LLVMGetReturnType(LLVMGlobalGetValueType(function));
    index = LLVMGetParam(function, 0U);
    argc = LLVMBuildLoad2(emitter->builder, i64, emitter->process_argc_global, "runtime.argument.count");
    invalid = LLVMBuildOr(emitter->builder,
                          LLVMBuildICmp(emitter->builder, LLVMIntSLT, index, i64_constant(emitter, 0), "runtime.argument.negative"),
                          LLVMBuildICmp(emitter->builder, LLVMIntUGE, index, argc, "runtime.argument.large"),
                          "runtime.argument.invalid");
    fail_block = LLVMAppendBasicBlockInContext(emitter->context, function, "invalid");
    ok_block = LLVMAppendBasicBlockInContext(emitter->context, function, "valid");
    LLVMBuildCondBr(emitter->builder, invalid, fail_block, ok_block);
    LLVMPositionBuilderAtEnd(emitter->builder, fail_block);
    abort_type = LLVMFunctionType(LLVMVoidTypeInContext(emitter->context), NULL, 0U, 0);
    abort_function = LLVMGetNamedFunction(emitter->module, "abort");
    if (abort_function == NULL)
        abort_function = LLVMAddFunction(emitter->module, "abort", abort_type);
    LLVMBuildCall2(emitter->builder, abort_type, abort_function, NULL, 0U, "");
    LLVMBuildUnreachable(emitter->builder);
    LLVMPositionBuilderAtEnd(emitter->builder, ok_block);
    argv = LLVMBuildLoad2(emitter->builder, argv_type, emitter->process_argv_global, "runtime.argv");
    host_index = LLVMBuildAdd(emitter->builder, index, i64_constant(emitter, 1), "runtime.argument.host.index");
    slot = LLVMBuildGEP2(emitter->builder, i8_pointer, argv, &host_index, 1U, "runtime.argument.slot");
    data = LLVMBuildLoad2(emitter->builder, i8_pointer, slot, "runtime.argument.data");
    length = __LLVM_Strlen__(emitter, data);
    if (length == NULL)
        return 0;
    value = LLVMConstNull(string_type);
    value = LLVMBuildInsertValue(emitter->builder, value, data, 0U, "runtime.argument.with.data");
    value = LLVMBuildInsertValue(emitter->builder, value, length, 1U, "runtime.argument.with.length");
    value = LLVMBuildInsertValue(emitter->builder, value, i64_constant(emitter, 0), 2U, "runtime.argument.with.capacity");
    LLVMBuildRet(emitter->builder, value);
    return 1;
}

static int define_process_exit(__LLVM_Emitter__ *emitter)
{
    LLVMValueRef function = runtime_export(emitter, "تشغيل_خروج_العملية", 1U);
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    LLVMTypeRef parameters[1] = {i32};
    LLVMValueRef arguments[1];
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    arguments[0] = LLVMBuildTrunc(
        emitter->builder, LLVMGetParam(function, 0U), i32, "runtime.exit.status");
    if (runtime_call(emitter,
                     "exit",
                     LLVMVoidTypeInContext(emitter->context),
                     parameters,
                     1U,
                     arguments,
                     "") == NULL)
        return 0;
    LLVMBuildUnreachable(emitter->builder);
    return 1;
}

static int define_write_executable(__LLVM_Emitter__ *emitter)
{
    LLVMValueRef function = runtime_export(emitter, "تشغيل_كتابة_بايتات_تنفيذية", 2U);
    __Ast_Function__ *semantic_function;
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMTypeRef fopen_parameters[2] = {i8_pointer, i8_pointer};
    LLVMTypeRef fwrite_parameters[4] = {i8_pointer, i64, i64, i8_pointer};
    LLVMTypeRef fclose_parameters[1] = {i8_pointer};
    LLVMTypeRef unlink_parameters[1] = {i8_pointer};
    LLVMTypeRef chmod_parameters[2] = {i8_pointer, i32};
    LLVMValueRef path;
    LLVMValueRef contents;
    LLVMValueRef c_path;
    LLVMValueRef data;
    LLVMValueRef length;
    LLVMValueRef status_slot;
    LLVMValueRef result_storage;
    LLVMTypeRef result_llvm_type;
    LLVMValueRef mode;
    LLVMValueRef file;
    LLVMValueRef open_failed;
    LLVMBasicBlockRef write_block;
    LLVMBasicBlockRef done_block;
    LLVMValueRef written;
    LLVMValueRef close_status;
    LLVMValueRef chmod_status;
    LLVMValueRef success_condition;
    LLVMValueRef status;
    __LLVM_Value__ success;
    __LLVM_Value__ result;
    if (function == NULL)
        return 1;
    if (runtime_export_failed(function))
        return 0;
    semantic_function = NULL;
    {
        size_t index;
        for (index = 0U; index < emitter->function_count; ++index)
            if (emitter->functions[index].value == function && emitter->functions[index].semantic != NULL)
            {
                semantic_function = emitter->functions[index].semantic->__Function__;
                break;
            }
    }
    if (semantic_function == NULL)
        return __LLVM_Fail__("Stage0 executable writer export has no semantic declaration");
    result_llvm_type = LLVMGetReturnType(LLVMGlobalGetValueType(function));
    result_storage = LLVMBuildAlloca(emitter->builder, result_llvm_type, "runtime.write.result.storage");
    path = LLVMGetParam(function, 0U);
    contents = LLVMGetParam(function, 1U);
    c_path = __LLVM_Runtime_C_String__(emitter, path, "runtime.write.path");
    if (c_path == NULL)
        return 0;
    data = vector_data(emitter, contents, "runtime.write.data");
    length = vector_length(emitter, contents, "runtime.write.length");
    status_slot = LLVMBuildAlloca(emitter->builder, i64, "runtime.write.status");
    LLVMBuildStore(emitter->builder, i64_constant(emitter, -1), status_slot);
    {
        LLVMValueRef arguments[1] = {c_path};
        if (runtime_call(emitter, "unlink", i32, unlink_parameters, 1U, arguments, "runtime.write.unlink") == NULL)
            return 0;
    }
    mode = LLVMBuildGlobalStringPtr(emitter->builder, "wbx", "runtime.write.mode");
    {
        LLVMValueRef arguments[2] = {c_path, mode};
        file = runtime_call(emitter, "fopen", i8_pointer, fopen_parameters, 2U, arguments, "runtime.write.file");
    }
    if (file == NULL)
        return 0;
    open_failed = LLVMBuildICmp(
        emitter->builder, LLVMIntEQ, file, LLVMConstNull(i8_pointer), "runtime.write.open.failed");
    write_block = LLVMAppendBasicBlockInContext(emitter->context, function, "opened");
    done_block = LLVMAppendBasicBlockInContext(emitter->context, function, "done");
    LLVMBuildCondBr(emitter->builder, open_failed, done_block, write_block);
    LLVMPositionBuilderAtEnd(emitter->builder, write_block);
    {
        LLVMValueRef arguments[4] = {data, LLVMConstInt(i64, 1U, 0), length, file};
        written = runtime_call(emitter, "fwrite", i64, fwrite_parameters, 4U, arguments, "runtime.write.count");
    }
    {
        LLVMValueRef arguments[1] = {file};
        close_status = runtime_call(emitter, "fclose", i32, fclose_parameters, 1U, arguments, "runtime.write.close");
    }
    {
        LLVMValueRef arguments[2] = {c_path, LLVMConstInt(i32, 0755U, 0)};
        chmod_status = runtime_call(emitter, "chmod", i32, chmod_parameters, 2U, arguments, "runtime.write.chmod");
    }
    if (written == NULL || close_status == NULL || chmod_status == NULL)
        return 0;
    success_condition = LLVMBuildAnd(
        emitter->builder,
        LLVMBuildICmp(emitter->builder, LLVMIntEQ, written, length, "runtime.write.complete"),
        LLVMBuildICmp(emitter->builder, LLVMIntEQ, close_status, LLVMConstInt(i32, 0U, 0), "runtime.write.closed"),
        "runtime.write.closed.complete");
    success_condition = LLVMBuildAnd(
        emitter->builder,
        success_condition,
        LLVMBuildICmp(emitter->builder, LLVMIntEQ, chmod_status, LLVMConstInt(i32, 0U, 0), "runtime.write.chmod.ok"),
        "runtime.write.success");
    status = LLVMBuildSelect(
        emitter->builder, success_condition, written, i64_constant(emitter, -1), "runtime.write.result");
    LLVMBuildStore(emitter->builder, status, status_slot);
    LLVMBuildBr(emitter->builder, done_block);
    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    if (!__LLVM_Runtime_Free__(emitter, c_path))
        return 0;
    status = LLVMBuildLoad2(emitter->builder, i64, status_slot, "runtime.write.status.value");
    success.value = status;
    success.type = &__LLVM_Integer_Type__;
    /* Tagged payload copies need entry-owned temporary storage. Insert before
     * the existing branch so allocations dominate both Result arms without
     * placing instructions after a terminator. */
    emitter->allocation_block = LLVMGetEntryBasicBlock(function);
    LLVMPositionBuilderBefore(
        emitter->allocation_builder,
        LLVMGetBasicBlockTerminator(emitter->allocation_block));
    result = __LLVM_Build_Runtime_Result__(
        emitter, semantic_function->__Output__.__Type__, result_storage, status, success);
    emitter->allocation_block = NULL;
    LLVMClearInsertionPosition(emitter->allocation_builder);
    if (result.value == NULL)
        return 0;
    LLVMBuildRet(emitter->builder, result.value);
    return 1;
}

int __LLVM_Define_Runtime_Exports__(__LLVM_Emitter__ *emitter)
{
    if (emitter == NULL || emitter->module == NULL || emitter->builder == NULL)
        return __LLVM_Fail__("invalid Stage0 runtime export provider");
    if (!__Bootstrap_Target_Runtime_Qualified__(&emitter->target))
        return __LLVM_Fail__("Stage0 runtime exports require a qualified target runtime");

    /* Keep the first migrated operation explicit: integer error classification. */
    if (!define_error_classification(emitter))
        return 0;

    if (!define_open_file(emitter, "تشغيل_فتح_ملف_للقراءة", 0) ||
        !define_read_byte(emitter, "تشغيل_قراءة_بايت_ملف", 0) ||
        !define_segment(emitter, "تشغيل_قراءة_مقطع_ملف", 0, 0) ||
        !define_open_file(emitter, "تشغيل_إنشاء_ملف_للكتابة", 1) ||
        !define_segment(emitter, "تشغيل_كتابة_مقطع_ملف", 0, 1) ||
        !define_close_file(emitter) ||
        !define_open_directory(emitter) ||
        !define_read_directory(emitter) ||
        !define_close_directory(emitter) ||
        !define_read_byte(emitter, "تشغيل_قراءة_بايت_الدخل_القياسي", 1) ||
        !define_segment(emitter, "تشغيل_قراءة_مقطع_الدخل_القياسي", 1, 0) ||
        !define_write_executable(emitter) ||
        !define_stream(emitter, "تشغيل_كتابة_الخرج_القياسي", 1U) ||
        !define_stream(emitter, "تشغيل_كتابة_الخطأ_القياسي", 2U) ||
        !define_argument_count(emitter) ||
        !define_argument(emitter) ||
        !define_process_exit(emitter) ||
        !define_text_from_bytes(emitter) ||
        !define_path_metadata(emitter, "تشغيل_نوع_مسار", 0U) ||
        !define_path_metadata(emitter, "تشغيل_حجم_مسار", 1U) ||
        !define_path_metadata(emitter, "تشغيل_وقت_تعديل_مسار", 2U))
        return 0;
    return 1;
}
