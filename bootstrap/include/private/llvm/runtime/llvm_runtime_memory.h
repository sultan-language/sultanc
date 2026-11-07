/* Declares private Stage0 LLVM runtime memory bridges. */

#ifndef SULTANC_BOOTSTRAP_LLVM_RUNTIME_MEMORY_H
#define SULTANC_BOOTSTRAP_LLVM_RUNTIME_MEMORY_H

#include "llvm/llvm_internal.h"

LLVMValueRef __LLVM_Allocate_Bytes__(__LLVM_Emitter__ *emitter,
                                     size_t byte_count,
                                     LLVMTypeRef target_pointer_type,
                                     const char *name);
LLVMValueRef __LLVM_Reallocate_Bytes__(__LLVM_Emitter__ *emitter,
                                       LLVMValueRef pointer,
                                       LLVMValueRef byte_count,
                                       LLVMTypeRef target_pointer_type,
                                       const char *name);
/* Releases a heap pointer previously produced by __LLVM_Allocate_Bytes__ or
 * __LLVM_Reallocate_Bytes__. Declares and calls the same canonical "free"
 * boundary symmetric with the existing malloc/realloc declarations; this is
 * not a new allocator API, it is the release half of the pair already used
 * by L2.6/L2.7 heap-backed vector/box/string storage. */
/* Grows a sequence buffer to BYTE_COUNT bytes. A capacity of 0 marks a buffer
 * the sequence does not own (static text or module-constant data, or none):
 * it is never passed to realloc; a fresh buffer receives a copy of the first
 * LIVE_BYTES bytes instead. */
LLVMValueRef __LLVM_Grow_Sequence_Bytes__(__LLVM_Emitter__ *emitter,
                                          LLVMValueRef data,
                                          LLVMValueRef capacity,
                                          LLVMValueRef live_bytes,
                                          LLVMValueRef byte_count,
                                          LLVMTypeRef target_pointer_type,
                                          const char *name);

int __LLVM_Free_Bytes__(__LLVM_Emitter__ *emitter, LLVMValueRef pointer);
#endif
