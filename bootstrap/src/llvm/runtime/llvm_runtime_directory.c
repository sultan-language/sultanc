/* Owns existing Stage0 LLVM runtime directory bridges. */

#include "llvm/runtime/llvm_runtime_directory.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "llvm/runtime/llvm_runtime_memory.h"
#include "llvm/runtime/llvm_runtime_process.h"

/* Emits the LLVM runtime path metadata service. */
__LLVM_Value__ __LLVM_Emit_Runtime_Path_Metadata_Service__(
    __LLVM_Emitter__ *emitter,
    __Ast_Expression__ *expression,
    __Ast_Type__ *expected,
    __Name_Builtin_Function__ builtin)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the path. */
    __LLVM_Value__ path;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the stat buffer type. */
    LLVMTypeRef stat_type = LLVMArrayType(i8, 144U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[2] = {i8_pointer, i8_pointer};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the C path string. */
    LLVMValueRef c_path;
    /* Stores the stat buffer. */
    LLVMValueRef stat_buffer;
    /* Stores the stat bytes pointer. */
    LLVMValueRef stat_bytes;
    /* Stores the call arguments. */
    LLVMValueRef arguments[2];
    /* Stores the call status. */
    LLVMValueRef status;
    /* Tracks a failed call. */
    LLVMValueRef failed;
    /* Stores the service value. */
    LLVMValueRef value = NULL;
#if defined(__APPLE__)
    const unsigned mode_offset = 4U;
    const unsigned size_offset = 96U;
    const unsigned mtime_seconds_offset = 48U;
    const unsigned mtime_nanoseconds_offset = 56U;
#else
    const unsigned mode_offset = 24U;
    const unsigned size_offset = 48U;
    const unsigned mtime_seconds_offset = 88U;
    const unsigned mtime_nanoseconds_offset = 96U;
#endif

    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("low-level path metadata service disagrees with canonical builtin arity");
        return result;
    }
    path = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_String_Type__);
    if (path.value == NULL)
        return result;
    c_path = __LLVM_Runtime_C_String__(emitter, path.value, "runtime.metadata.path");
    if (c_path == NULL)
        return result;

    stat_buffer = __LLVM_Allocate_Stack__(emitter, stat_type, "runtime.metadata.stat");
    if (stat_buffer == NULL)
        return result;
    LLVMBuildStore(emitter->builder, LLVMConstNull(stat_type), stat_buffer);
    stat_bytes = LLVMBuildPointerCast(
        emitter->builder, stat_buffer, i8_pointer, "runtime.metadata.stat.bytes");
    function = __LLVM_Declare_Runtime_Function__(
        emitter, "lstat", i32, parameters, 2U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] = c_path;
    arguments[1] = stat_bytes;
    status = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 2U, "runtime.metadata.status");
    if (!__LLVM_Runtime_Free__(emitter, c_path))
        return result;
    failed = LLVMBuildICmp(
        emitter->builder, LLVMIntSLT, status, LLVMConstInt(i32, 0U, 0), "runtime.metadata.failed");

    if (builtin == __Name_Builtin_Path_Type__)
    {
        /* Stores the mode field pointer. */
        LLVMValueRef mode_index = LLVMConstInt(i64, mode_offset, 0);
        LLVMValueRef mode_bytes = LLVMBuildGEP2(
            emitter->builder, i8, stat_bytes, &mode_index, 1U, "runtime.metadata.mode.bytes");
#if defined(__APPLE__)
        LLVMTypeRef i16 = LLVMIntTypeInContext(emitter->context, 16U);
        LLVMValueRef mode_pointer = LLVMBuildPointerCast(
            emitter->builder, mode_bytes, LLVMPointerType(i16, 0U), "runtime.metadata.mode.ptr");
        LLVMValueRef mode = LLVMBuildZExt(
            emitter->builder,
            LLVMBuildLoad2(emitter->builder, i16, mode_pointer, "runtime.metadata.mode16"),
            i64,
            "runtime.metadata.mode");
#else
        LLVMValueRef mode_pointer = LLVMBuildPointerCast(
            emitter->builder, mode_bytes, LLVMPointerType(i32, 0U), "runtime.metadata.mode.ptr");
        LLVMValueRef mode = LLVMBuildZExt(
            emitter->builder,
            LLVMBuildLoad2(emitter->builder, i32, mode_pointer, "runtime.metadata.mode32"),
            i64,
            "runtime.metadata.mode");
#endif
        LLVMValueRef kind = LLVMBuildAnd(
            emitter->builder, mode, LLVMConstInt(i64, 0xF000U, 0), "runtime.metadata.kind");
        LLVMValueRef regular = LLVMBuildICmp(
            emitter->builder, LLVMIntEQ, kind, LLVMConstInt(i64, 0x8000U, 0), "runtime.metadata.regular");
        LLVMValueRef directory = LLVMBuildICmp(
            emitter->builder, LLVMIntEQ, kind, LLVMConstInt(i64, 0x4000U, 0), "runtime.metadata.directory");
        LLVMValueRef symlink = LLVMBuildICmp(
            emitter->builder, LLVMIntEQ, kind, LLVMConstInt(i64, 0xA000U, 0), "runtime.metadata.symlink");
        value = LLVMConstInt(i64, 4U, 0);
        value = LLVMBuildSelect(
            emitter->builder, symlink, LLVMConstInt(i64, 3U, 0), value, "runtime.metadata.kind.symlink");
        value = LLVMBuildSelect(
            emitter->builder, directory, LLVMConstInt(i64, 2U, 0), value, "runtime.metadata.kind.directory");
        value = LLVMBuildSelect(
            emitter->builder, regular, LLVMConstInt(i64, 1U, 0), value, "runtime.metadata.kind.regular");
    }
    else if (builtin == __Name_Builtin_Path_Size__)
    {
        /* Stores the size field pointer. */
        LLVMValueRef size_index = LLVMConstInt(i64, size_offset, 0);
        LLVMValueRef size_bytes = LLVMBuildGEP2(
            emitter->builder, i8, stat_bytes, &size_index, 1U, "runtime.metadata.size.bytes");
        LLVMValueRef size_pointer = LLVMBuildPointerCast(
            emitter->builder, size_bytes, LLVMPointerType(i64, 0U), "runtime.metadata.size.ptr");
        value = LLVMBuildLoad2(emitter->builder, i64, size_pointer, "runtime.metadata.size");
    }
    else if (builtin == __Name_Builtin_Path_Modified_Time__)
    {
        /* Stores the modified-time field pointers. */
        LLVMValueRef seconds_index = LLVMConstInt(i64, mtime_seconds_offset, 0);
        LLVMValueRef nanoseconds_index = LLVMConstInt(i64, mtime_nanoseconds_offset, 0);
        LLVMValueRef seconds_bytes = LLVMBuildGEP2(
            emitter->builder, i8, stat_bytes, &seconds_index, 1U, "runtime.metadata.mtime.sec.bytes");
        LLVMValueRef nanoseconds_bytes = LLVMBuildGEP2(
            emitter->builder, i8, stat_bytes, &nanoseconds_index, 1U, "runtime.metadata.mtime.nsec.bytes");
        LLVMValueRef seconds_pointer = LLVMBuildPointerCast(
            emitter->builder, seconds_bytes, LLVMPointerType(i64, 0U), "runtime.metadata.mtime.sec.ptr");
        LLVMValueRef nanoseconds_pointer = LLVMBuildPointerCast(
            emitter->builder, nanoseconds_bytes, LLVMPointerType(i64, 0U), "runtime.metadata.mtime.nsec.ptr");
        LLVMValueRef seconds = LLVMBuildLoad2(
            emitter->builder, i64, seconds_pointer, "runtime.metadata.mtime.sec");
        LLVMValueRef nanoseconds = LLVMBuildLoad2(
            emitter->builder, i64, nanoseconds_pointer, "runtime.metadata.mtime.nsec");
        value = LLVMBuildAdd(
            emitter->builder,
            LLVMBuildMul(
                emitter->builder,
                seconds,
                LLVMConstInt(i64, 1000000000ULL, 0),
                "runtime.metadata.mtime.seconds.ns"),
            nanoseconds,
            "runtime.metadata.mtime.ns");
    }
    else
    {
        __LLVM_Fail__("unknown Bootstrap path metadata builtin");
        return result;
    }

    result.value = LLVMBuildSelect(
        emitter->builder,
        failed,
        LLVMConstInt(i64, (unsigned long long)-2LL, 1),
        value,
        "runtime.metadata.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime open directory service. */
__LLVM_Value__ __LLVM_Emit_Runtime_Open_Directory_Service__(__LLVM_Emitter__ *emitter,
                                                                   __Ast_Expression__ *expression,
                                                                   __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the path. */
    __LLVM_Value__ path;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[1] = {i8_pointer};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the C path string. */
    LLVMValueRef c_path;
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    /* Stores the directory. */
    LLVMValueRef directory;
    /* Tracks the failed state. */
    LLVMValueRef failed;
    /* Stores the handle. */
    LLVMValueRef handle;

    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("low-level directory-open service disagrees with canonical builtin arity");
        return result;
    }
    path = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_String_Type__);
    if (path.value == NULL)
        return result;
    c_path = __LLVM_Runtime_C_String__(emitter, path.value, "runtime.directory.path");
    if (c_path == NULL)
        return result;
    function = __LLVM_Declare_Runtime_Function__(
        emitter, "opendir", i8_pointer, parameters, 1U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] = c_path;
    directory = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 1U, "runtime.directory.open");
    if (!__LLVM_Runtime_Free__(emitter, c_path))
        return result;
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           directory,
                           LLVMConstNull(i8_pointer),
                           "runtime.directory.open.failed");
    handle = LLVMBuildPtrToInt(emitter->builder, directory, i64, "runtime.directory.handle");
    result.value = LLVMBuildSelect(emitter->builder,
                                   failed,
                                   LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                   handle,
                                   "runtime.directory.open.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime read directory entry service. */
__LLVM_Value__ __LLVM_Emit_Runtime_Read_Directory_Entry_Service__(
    __LLVM_Emitter__ *emitter,
    __Ast_Expression__ *expression,
    __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the handle. */
    __LLVM_Value__ handle;
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
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the readdir parameters. */
    LLVMTypeRef readdir_parameters[1] = {i8_pointer};
    /* Stores the readdir type. */
    LLVMTypeRef readdir_type;
    /* Stores the readdir function. */
    LLVMValueRef readdir_function;
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the invalid. */
    LLVMValueRef invalid;
    /* Stores the handle invalid. */
    LLVMValueRef handle_invalid;
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
    /* Stores the status slot. */
    LLVMValueRef status_slot;
    /* Stores the current function. */
    LLVMValueRef current_function;
    /* Stores the invalid block. */
    LLVMBasicBlockRef invalid_block;
    /* Stores the read block. */
    LLVMBasicBlockRef read_block;
    /* Stores the EOF block. */
    LLVMBasicBlockRef eof_block;
    /* Stores the entry block. */
    LLVMBasicBlockRef entry_block;
    /* Stores the copy block. */
    LLVMBasicBlockRef copy_block;
    /* Stores the name error block. */
    LLVMBasicBlockRef name_error_block;
    /* Tracks the done block state. */
    LLVMBasicBlockRef done_block;
    /* Stores the directory pointer. */
    LLVMValueRef directory_pointer;
    /* Stores the readdir arguments. */
    LLVMValueRef readdir_args[1];
    /* Stores the entry. */
    LLVMValueRef entry;
    /* Tracks whether the value is EOF. */
    LLVMValueRef is_eof;
    /* Stores the name pointer. */
    LLVMValueRef name_pointer;
    /* Stores the name length. */
    LLVMValueRef name_length;
    /* Stores the name too long. */
    LLVMValueRef name_too_long;
    /* Stores the name encoding too long. */
    LLVMValueRef name_encoding_too_long;
    /* Stores the invalid name. */
    LLVMValueRef invalid_name;
    /* Stores the destination. */
    LLVMValueRef destination;
    /* Stores the type pointer. */
    LLVMValueRef type_pointer;
    /* Stores the raw type. */
    LLVMValueRef raw_type;
    /* Stores the kind. */
    LLVMValueRef kind;
    /* Stores the encoded kind. */
    LLVMValueRef encoded_kind;
    /* Stores the encoded result. */
    LLVMValueRef encoded_result;
#if defined(__APPLE__)
    const unsigned directory_name_offset = 21U;
    const unsigned directory_type_offset = 20U;
#else
    /* Stores the directory name offset. */
    const unsigned directory_name_offset = 19U;
    /* Stores the directory type offset. */
    const unsigned directory_type_offset = 18U;
#endif

    if (expression->__As__.__Call__.__Argument_Count__ != 4U)
    {
        __LLVM_Fail__("low-level directory-read service disagrees with canonical builtin arity");
        return result;
    }

    handle = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
    bytes_type = __LLVM_Expression_Type__(emitter, expression->__As__.__Call__.__Arguments__[1]);
    if (bytes_type == NULL)
        return result;
    bytes = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[1], bytes_type);
    offset = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[2], &__LLVM_Integer_Type__);
    requested = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[3], &__LLVM_Integer_Type__);
    if (handle.value == NULL || bytes.value == NULL || offset.value == NULL || requested.value == NULL)
        return result;
    handle = __LLVM_Coerce__(emitter, handle, &__LLVM_Integer_Type__);
    offset = __LLVM_Coerce__(emitter, offset, &__LLVM_Integer_Type__);
    requested = __LLVM_Coerce__(emitter, requested, &__LLVM_Integer_Type__);
    if (handle.value == NULL || offset.value == NULL || requested.value == NULL)
        return result;

    data = LLVMBuildPointerCast(
        emitter->builder,
        LLVMBuildExtractValue(emitter->builder, bytes.value, 0U, "runtime.directory.bytes.data"),
        i8_pointer,
        "runtime.directory.bytes");
    length = LLVMBuildExtractValue(
        emitter->builder, bytes.value, 1U, "runtime.directory.bytes.length");
    handle_invalid = LLVMBuildICmp(emitter->builder,
                                   LLVMIntSLE,
                                   handle.value,
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.directory.handle.invalid");
    negative_offset = LLVMBuildICmp(emitter->builder,
                                    LLVMIntSLT,
                                    offset.value,
                                    LLVMConstInt(i64, 0U, 0),
                                    "runtime.directory.offset.negative");
    negative_count = LLVMBuildICmp(emitter->builder,
                                   LLVMIntSLT,
                                   requested.value,
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.directory.count.negative");
    offset_too_large = LLVMBuildICmp(emitter->builder,
                                     LLVMIntUGT,
                                     offset.value,
                                     length,
                                     "runtime.directory.offset.large");
    remaining = LLVMBuildSub(emitter->builder, length, offset.value, "runtime.directory.remaining");
    count_too_large = LLVMBuildICmp(emitter->builder,
                                    LLVMIntUGT,
                                    requested.value,
                                    remaining,
                                    "runtime.directory.count.large");
    invalid = LLVMBuildOr(emitter->builder,
                          handle_invalid,
                          negative_offset,
                          "runtime.directory.invalid.handle.offset");
    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          negative_count,
                          "runtime.directory.invalid.count.sign");
    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          offset_too_large,
                          "runtime.directory.invalid.offset");
    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          count_too_large,
                          "runtime.directory.invalid.count");

    status_slot = __LLVM_Allocate_Stack__(emitter, i64, "runtime.directory.status");
    if (status_slot == NULL)
        return result;
    current_function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    if (current_function == NULL)
        return result;
    invalid_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.invalid");
    read_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.read");
    eof_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.eof");
    entry_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.entry");
    copy_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.copy");
    name_error_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.name.error");
    done_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.done");
    LLVMBuildCondBr(emitter->builder, invalid, invalid_block, read_block);

    LLVMPositionBuilderAtEnd(emitter->builder, invalid_block);
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, (unsigned long long)-2LL, 1), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, read_block);
    readdir_function = __LLVM_Declare_Runtime_Function__(
        emitter, "readdir", i8_pointer, readdir_parameters, 1U, &readdir_type);
    if (readdir_function == NULL)
        return result;
    directory_pointer = LLVMBuildIntToPtr(
        emitter->builder, handle.value, i8_pointer, "runtime.directory.pointer");
    readdir_args[0] = directory_pointer;
    entry = LLVMBuildCall2(
        emitter->builder, readdir_type, readdir_function, readdir_args, 1U, "runtime.directory.entry.ptr");
    is_eof = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           entry,
                           LLVMConstNull(i8_pointer),
                           "runtime.directory.entry.eof");
    LLVMBuildCondBr(emitter->builder, is_eof, eof_block, entry_block);

    LLVMPositionBuilderAtEnd(emitter->builder, eof_block);
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, 0U, 0), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, entry_block);
    name_pointer = LLVMBuildGEP2(emitter->builder,
                                 i8,
                                 entry,
                                 &(LLVMValueRef){LLVMConstInt(i64, directory_name_offset, 0)},
                                 1U,
                                 "runtime.directory.entry.name");
    name_length = __LLVM_Strlen__(emitter, name_pointer);
    if (name_length == NULL)
        return result;
    name_too_long = LLVMBuildICmp(emitter->builder,
                                  LLVMIntUGT,
                                  name_length,
                                  requested.value,
                                  "runtime.directory.name.too.long");
    name_encoding_too_long = LLVMBuildICmp(emitter->builder,
                                           LLVMIntUGE,
                                           name_length,
                                           LLVMConstInt(i64, 4096U, 0),
                                           "runtime.directory.name.encoding.too.long");
    invalid_name = LLVMBuildOr(emitter->builder,
                               name_too_long,
                               name_encoding_too_long,
                               "runtime.directory.name.invalid");
    LLVMBuildCondBr(emitter->builder, invalid_name, name_error_block, copy_block);

    LLVMPositionBuilderAtEnd(emitter->builder, name_error_block);
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, (unsigned long long)-2LL, 1), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, copy_block);
    destination = LLVMBuildGEP2(
        emitter->builder, i8, data, &offset.value, 1U, "runtime.directory.destination");
    if (LLVMBuildMemCpy(emitter->builder, destination, 1U, name_pointer, 1U, name_length) == NULL)
        return result;
    {
        /* Tracks the type index. */
        LLVMValueRef type_index = LLVMConstInt(i64, directory_type_offset, 0);
        /* Tracks whether the value is regular. */
        LLVMValueRef is_regular;
        /* Tracks whether the value is directory. */
        LLVMValueRef is_directory;
        /* Tracks whether the value is symlink. */
        LLVMValueRef is_symlink;
        /* Stores the one. */
        LLVMValueRef one = LLVMConstInt(i64, 1U, 0);
        /* Stores the two. */
        LLVMValueRef two = LLVMConstInt(i64, 2U, 0);
        /* Stores the three. */
        LLVMValueRef three = LLVMConstInt(i64, 3U, 0);
        /* Stores the zero. */
        LLVMValueRef zero = LLVMConstInt(i64, 0U, 0);
        type_pointer = LLVMBuildGEP2(
            emitter->builder, i8, entry, &type_index, 1U, "runtime.directory.entry.type.ptr");
        raw_type = LLVMBuildLoad2(emitter->builder, i8, type_pointer, "runtime.directory.entry.type");
        is_regular = LLVMBuildICmp(emitter->builder,
                                   LLVMIntEQ,
                                   raw_type,
                                   LLVMConstInt(i8, 8U, 0),
                                   "runtime.directory.type.regular");
        is_directory = LLVMBuildICmp(emitter->builder,
                                     LLVMIntEQ,
                                     raw_type,
                                     LLVMConstInt(i8, 4U, 0),
                                     "runtime.directory.type.directory");
        is_symlink = LLVMBuildICmp(emitter->builder,
                                   LLVMIntEQ,
                                   raw_type,
                                   LLVMConstInt(i8, 10U, 0),
                                   "runtime.directory.type.symlink");
        kind = LLVMBuildSelect(emitter->builder, is_symlink, three, zero, "runtime.directory.kind.symlink");
        kind = LLVMBuildSelect(emitter->builder, is_directory, two, kind, "runtime.directory.kind.directory");
        kind = LLVMBuildSelect(emitter->builder, is_regular, one, kind, "runtime.directory.kind.regular");
    }
    encoded_kind = LLVMBuildShl(
        emitter->builder, kind, LLVMConstInt(i64, 12U, 0), "runtime.directory.kind.encoded");
    encoded_result = LLVMBuildAdd(
        emitter->builder, encoded_kind, name_length, "runtime.directory.entry.result");
    LLVMBuildStore(emitter->builder, encoded_result, status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    result.value = LLVMBuildLoad2(
        emitter->builder, i64, status_slot, "runtime.directory.status.value");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime close directory service. */
__LLVM_Value__ __LLVM_Emit_Runtime_Close_Directory_Service__(__LLVM_Emitter__ *emitter,
                                                                    __Ast_Expression__ *expression,
                                                                    __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the handle. */
    __LLVM_Value__ handle;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[1] = {i8_pointer};
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
        __LLVM_Fail__("low-level directory-close service disagrees with canonical builtin arity");
        return result;
    }
    handle = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
    if (handle.value == NULL)
        return result;
    handle = __LLVM_Coerce__(emitter, handle, &__LLVM_Integer_Type__);
    if (handle.value == NULL)
        return result;
    function = __LLVM_Declare_Runtime_Function__(
        emitter, "closedir", i32, parameters, 1U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] = LLVMBuildIntToPtr(
        emitter->builder, handle.value, i8_pointer, "runtime.directory.close.pointer");
    status = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 1U, "runtime.directory.close.status");
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           status,
                           LLVMConstInt(i32, 0U, 0),
                           "runtime.directory.close.failed");
    result.value = LLVMBuildSelect(emitter->builder,
                                   failed,
                                   LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.directory.close.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}
