/* Owns existing Stage0 LLVM runtime memory bridges and helpers. */

#include "llvm/runtime/llvm_runtime_memory.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_values.h"

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

/* Releases the LLVM heap bytes. The canonical release boundary symmetric
 * with __LLVM_Allocate_Bytes__/__LLVM_Reallocate_Bytes__ above: it declares
 * "free" lazily and caches it on the emitter exactly the way malloc/realloc
 * are declared and cached, so drop elaboration reuses the existing heap
 * boundary rather than inventing a second allocator surface. */
int __LLVM_Free_Bytes__(__LLVM_Emitter__ *emitter, LLVMValueRef pointer)
{
    /* Stores the byte pointer type. */
    LLVMTypeRef byte_pointer;
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];

    if (emitter == NULL || pointer == NULL)
    {
        return __LLVM_Fail__("L2 drop elaboration released a missing heap pointer");
    }
    byte_pointer = LLVMPointerType(LLVMIntTypeInContext(emitter->context, 8U), 0U);
    if (emitter->free_function == NULL)
    {
        /* Stores the parameter values. */
        LLVMTypeRef params[1];
        params[0] = byte_pointer;
        emitter->free_type =
            LLVMFunctionType(LLVMVoidTypeInContext(emitter->context), params, 1U, 0);
        emitter->free_function = LLVMGetNamedFunction(emitter->module, "free");
        if (emitter->free_function == NULL)
        {
            emitter->free_function =
                LLVMAddFunction(emitter->module, "free", emitter->free_type);
        }
        if (emitter->free_function == NULL)
        {
            return __LLVM_Fail__(
                "L2 drop elaboration could not declare canonical heap release boundary");
        }
    }
    arguments[0] =
        LLVMBuildPointerCast(emitter->builder, pointer, byte_pointer, "drop.release.bytes");
    if (LLVMBuildCall2(emitter->builder,
                       emitter->free_type,
                       emitter->free_function,
                       arguments,
                       1U,
                       "") == NULL)
    {
        return __LLVM_Fail__("L2 drop elaboration heap release call failed");
    }
    return 1;
}

LLVMValueRef __LLVM_Grow_Sequence_Bytes__(__LLVM_Emitter__ *emitter,
                                          LLVMValueRef data,
                                          LLVMValueRef capacity,
                                          LLVMValueRef live_bytes,
                                          LLVMValueRef byte_count,
                                          LLVMTypeRef target_pointer_type,
                                          const char *name)
{
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    LLVMValueRef unowned;
    LLVMValueRef source;
    LLVMValueRef grown;
    LLVMValueRef copy_bytes;
    if (data == NULL || capacity == NULL || live_bytes == NULL || byte_count == NULL)
    {
        __LLVM_Fail__("sequence growth is missing canonical storage facts");
        return NULL;
    }
    unowned = LLVMBuildICmp(emitter->builder, LLVMIntEQ, capacity, LLVMConstInt(i64, 0U, 0),
                            "grow.unowned");
    source = LLVMBuildSelect(emitter->builder, unowned, LLVMConstNull(LLVMTypeOf(data)), data,
                             "grow.source");
    grown = __LLVM_Reallocate_Bytes__(emitter, source, byte_count, target_pointer_type, name);
    if (grown == NULL)
        return NULL;
    copy_bytes = LLVMBuildSelect(emitter->builder, unowned, live_bytes, LLVMConstInt(i64, 0U, 0),
                                 "grow.copy.bytes");
    LLVMBuildMemCpy(emitter->builder, grown, 1U, data, 1U, copy_bytes);
    return grown;
}
