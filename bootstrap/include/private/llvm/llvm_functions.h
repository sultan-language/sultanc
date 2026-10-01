/* Declares private Stage0 LLVM function lowering. */

#ifndef SULTANC_BOOTSTRAP_LLVM_FUNCTIONS_H
#define SULTANC_BOOTSTRAP_LLVM_FUNCTIONS_H

#include "llvm/llvm_internal.h"

__LLVM_Function__ *__LLVM_Find_Function_By_Semantic__(__LLVM_Emitter__ *emitter,
                                                       __Semantic_Function_Entry__ *semantic);
__Semantic_Function_Entry__ *__LLVM_Resolve_Ordinary_Callee__(
    __LLVM_Emitter__ *emitter,
    __Ast_Expression__ *expression);
int __LLVM_Declare_Functions__(__LLVM_Emitter__ *emitter);
int __LLVM_Emit_Function_Body__(__LLVM_Emitter__ *emitter,
                                __LLVM_Function__ *llvm_function);
int __LLVM_Emit_Process_Entry__(__LLVM_Emitter__ *emitter);

#endif
