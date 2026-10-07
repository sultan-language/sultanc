/* Declares private Stage0 LLVM runtime process bridges. */

#ifndef SULTANC_BOOTSTRAP_LLVM_RUNTIME_PROCESS_H
#define SULTANC_BOOTSTRAP_LLVM_RUNTIME_PROCESS_H

#include "llvm/llvm_internal.h"
#include "kernel/name/name.h"

int __LLVM_Ensure_Process_Globals__(__LLVM_Emitter__ *emitter);
LLVMValueRef __LLVM_Strlen__(__LLVM_Emitter__ *emitter, LLVMValueRef data);
__LLVM_Value__ __LLVM_Emit_Host_Identity__(__LLVM_Emitter__ *emitter,
                                           __Name_Builtin_Function__ builtin,
                                           __Ast_Type__ *expected);

#endif
