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
LLVMValueRef __LLVM_Declare_Runtime_Function__(__LLVM_Emitter__ *emitter,
                                               const char *name,
                                               LLVMTypeRef return_type,
                                               LLVMTypeRef *parameters,
                                               unsigned parameter_count,
                                               LLVMTypeRef *out_function_type);
LLVMValueRef __LLVM_Runtime_C_String__(__LLVM_Emitter__ *emitter,
                                       LLVMValueRef text,
                                       const char *name);
LLVMValueRef __LLVM_Runtime_Literal_C_String__(__LLVM_Emitter__ *emitter,
                                               const char *literal,
                                               const char *name);
int __LLVM_Runtime_Free__(__LLVM_Emitter__ *emitter, LLVMValueRef pointer);

#endif
