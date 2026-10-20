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
#endif
