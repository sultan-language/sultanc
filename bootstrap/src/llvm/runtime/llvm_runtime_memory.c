/* Owns existing Stage0 LLVM runtime memory bridges and helpers. */

#include "llvm/runtime/llvm_runtime_memory.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_values.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Allocates the LLVM bytes. */
LLVMValueRef __LLVM_Allocate_Bytes__(__LLVM_Emitter__ *emitter,
                                            size_t byte_count,
                                            LLVMTypeRef target_pointer_type,
                                            const char *name)
{
    /* Stores the byte type. */
    LLVMTypeRef byte_type = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the byte pointer. */
    LLVMTypeRef byte_pointer = LLVMPointerType(byte_type, 0U);
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    /* Stores the allocated value. */
    LLVMValueRef allocated;
    /* Tracks the failed state. */
    LLVMValueRef failed;

    if (byte_count == 0U)
    {
        return LLVMConstNull(target_pointer_type);
    }
    if (emitter->malloc_function == NULL)
    {
        /* Stores the parameter values. */
        LLVMTypeRef params[1];
        params[0] = LLVMIntTypeInContext(emitter->context, 64U);
        emitter->malloc_type = LLVMFunctionType(byte_pointer, params, 1U, 0);
        emitter->malloc_function = LLVMGetNamedFunction(emitter->module, "malloc");
        if (emitter->malloc_function == NULL)
        {
            emitter->malloc_function =
                LLVMAddFunction(emitter->module, "malloc", emitter->malloc_type);
        }
        if (emitter->malloc_function == NULL)
        {
            __LLVM_Fail__("L2.6/L2.7 could not declare canonical heap allocation boundary");
            return NULL;
        }
    }
    arguments[0] = LLVMConstInt(
        LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)byte_count, 0);
    allocated = LLVMBuildCall2(emitter->builder,
                               emitter->malloc_type,
                               emitter->malloc_function,
                               arguments,
                               1U,
                               "heap.allocate");
    if (allocated == NULL)
    {
        __LLVM_Fail__("L2.6/L2.7 heap allocation call failed");
        return NULL;
    }
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           allocated,
                           LLVMConstNull(byte_pointer),
                           "heap.allocate.failed");
    if (!__LLVM_Emit_Trap_If__(emitter, failed, "heap.allocate.failure"))
    {
        return NULL;
    }
    return LLVMBuildPointerCast(emitter->builder, allocated, target_pointer_type, name);
}

/* Returns the LLVM reallocate bytes. */
LLVMValueRef __LLVM_Reallocate_Bytes__(__LLVM_Emitter__ *emitter,
                                              LLVMValueRef pointer,
                                              LLVMValueRef byte_count,
                                              LLVMTypeRef target_pointer_type,
                                              const char *name)
{
    /* Stores the byte type. */
    LLVMTypeRef byte_type = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the byte pointer. */
    LLVMTypeRef byte_pointer = LLVMPointerType(byte_type, 0U);
    /* Stores the call arguments. */
    LLVMValueRef arguments[2];
    /* Stores the allocated value. */
    LLVMValueRef allocated;
    /* Tracks the failed state. */
    LLVMValueRef failed;

    if (pointer == NULL || byte_count == NULL || target_pointer_type == NULL)
    {
        __LLVM_Fail__("L2.7 vector reallocation is missing canonical storage facts");
        return NULL;
    }
    if (emitter->realloc_function == NULL)
    {
        /* Stores the parameter values. */
        LLVMTypeRef params[2];
        params[0] = byte_pointer;
        params[1] = LLVMIntTypeInContext(emitter->context, 64U);
        emitter->realloc_type = LLVMFunctionType(byte_pointer, params, 2U, 0);
        emitter->realloc_function = LLVMGetNamedFunction(emitter->module, "realloc");
        if (emitter->realloc_function == NULL)
            emitter->realloc_function =
                LLVMAddFunction(emitter->module, "realloc", emitter->realloc_type);
        if (emitter->realloc_function == NULL)
        {
            __LLVM_Fail__("L2.7 could not declare canonical vector reallocation boundary");
            return NULL;
        }
    }
    arguments[0] =
        LLVMBuildPointerCast(emitter->builder, pointer, byte_pointer, "vector.realloc.bytes");
    arguments[1] = byte_count;
    allocated = LLVMBuildCall2(emitter->builder,
                               emitter->realloc_type,
                               emitter->realloc_function,
                               arguments,
                               2U,
                               "vector.reallocate");
    if (allocated == NULL)
    {
        __LLVM_Fail__("L2.7 vector reallocation call failed");
        return NULL;
    }
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           allocated,
                           LLVMConstNull(byte_pointer),
                           "vector.reallocate.failed");
    if (!__LLVM_Emit_Trap_If__(emitter, failed, "vector.reallocate.failure"))
        return NULL;
    return LLVMBuildPointerCast(emitter->builder, allocated, target_pointer_type, name);
}

/* Returns the LLVM declare runtime function. */
LLVMValueRef __LLVM_Declare_Runtime_Function__(__LLVM_Emitter__ *emitter,
                                                      const char *name,
                                                      LLVMTypeRef return_type,
                                                      LLVMTypeRef *parameters,
                                                      unsigned parameter_count,
                                                      LLVMTypeRef *out_function_type)
{
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    function_type = LLVMFunctionType(return_type, parameters, parameter_count, 0);
    function = LLVMGetNamedFunction(emitter->module, name);
    if (function == NULL)
        function = LLVMAddFunction(emitter->module, name, function_type);
    if (function == NULL)
    {
        __LLVM_Fail__("L2.8 could not declare native runtime function");
        return NULL;
    }
    if (out_function_type != NULL)
        *out_function_type = function_type;
    return function;
}

/* Returns the LLVM runtime malloc. */
static LLVMValueRef
__LLVM_Runtime_Malloc__(__LLVM_Emitter__ *emitter, LLVMValueRef byte_count, const char *name)
{
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    if (byte_count == NULL)
        return NULL;
    if (emitter->malloc_function == NULL)
    {
        /* Stores the parameters. */
        LLVMTypeRef parameters[1] = {LLVMIntTypeInContext(emitter->context, 64U)};
        emitter->malloc_type = LLVMFunctionType(i8_pointer, parameters, 1U, 0);
        emitter->malloc_function = LLVMGetNamedFunction(emitter->module, "malloc");
        if (emitter->malloc_function == NULL)
            emitter->malloc_function =
                LLVMAddFunction(emitter->module, "malloc", emitter->malloc_type);
        if (emitter->malloc_function == NULL)
        {
            __LLVM_Fail__("L2.8 could not declare native runtime allocation");
            return NULL;
        }
    }
    arguments[0] = byte_count;
    return LLVMBuildCall2(
        emitter->builder, emitter->malloc_type, emitter->malloc_function, arguments, 1U, name);
}

/* Returns the LLVM runtime c string. */
LLVMValueRef
__LLVM_Runtime_C_String__(__LLVM_Emitter__ *emitter, LLVMValueRef text, const char *name)
{
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the allocation size. */
    LLVMValueRef allocation_size;
    /* Stores the buffer. */
    LLVMValueRef buffer;
    /* Tracks the failed state. */
    LLVMValueRef failed;
    /* Stores the end. */
    LLVMValueRef end;
    /* Stores the indices. */
    LLVMValueRef indices[1];
    if (text == NULL)
        return NULL;
    data = LLVMBuildExtractValue(emitter->builder, text, 0U, "runtime.path.data");
    length = LLVMBuildExtractValue(emitter->builder, text, 1U, "runtime.path.length");
    allocation_size =
        LLVMBuildAdd(emitter->builder, length, LLVMConstInt(i64, 1U, 0), "runtime.path.size");
    buffer = __LLVM_Runtime_Malloc__(emitter, allocation_size, name);
    if (buffer == NULL)
        return NULL;
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           buffer,
                           LLVMConstNull(i8_pointer),
                           "runtime.path.allocate.failed");
    if (!__LLVM_Emit_Trap_If__(emitter, failed, "runtime.path.allocate.failure"))
        return NULL;
    if (LLVMBuildMemCpy(emitter->builder, buffer, 1U, data, 1U, length) == NULL)
        return NULL;
    indices[0] = length;
    end = LLVMBuildGEP2(emitter->builder, i8, buffer, indices, 1U, "runtime.path.end");
    if (end == NULL)
        return NULL;
    LLVMBuildStore(emitter->builder, LLVMConstInt(i8, 0U, 0), end);
    return buffer;
}

/* Returns the LLVM runtime literal c string. */
LLVMValueRef
__LLVM_Runtime_Literal_C_String__(__LLVM_Emitter__ *emitter, const char *literal, const char *name)
{
    /* Stores the length. */
    size_t length;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8;
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer;
    /* Stores the array type. */
    LLVMTypeRef array_type;
    /* Stores the storage. */
    LLVMValueRef storage;
    /* Stores the constant. */
    LLVMValueRef constant;
    if (literal == NULL)
        return NULL;
    length = strlen(literal);
    if (length > UINT_MAX - 1U)
        return NULL;
    i8 = LLVMIntTypeInContext(emitter->context, 8U);
    i8_pointer = LLVMPointerType(i8, 0U);
    array_type = LLVMArrayType(i8, (unsigned)length + 1U);
    storage = __LLVM_Allocate_Stack__(emitter, array_type, name);
    if (storage == NULL)
        return NULL;
    constant = LLVMConstStringInContext(emitter->context, literal, (unsigned)length, 0);
    if (constant == NULL)
        return NULL;
    LLVMBuildStore(emitter->builder, constant, storage);
    return LLVMBuildPointerCast(emitter->builder, storage, i8_pointer, name);
}

/* Releases the LLVM runtime. */
int __LLVM_Runtime_Free__(__LLVM_Emitter__ *emitter, LLVMValueRef pointer)
{
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[1] = {i8_pointer};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    function = __LLVM_Declare_Runtime_Function__(
        emitter, "free", LLVMVoidTypeInContext(emitter->context), parameters, 1U, &function_type);
    if (function == NULL)
        return 0;
    arguments[0] = pointer;
    return LLVMBuildCall2(emitter->builder, function_type, function, arguments, 1U, "") != NULL;
}
