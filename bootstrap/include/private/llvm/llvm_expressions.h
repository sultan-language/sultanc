/* Declares private Stage0 LLVM expression lowering. */

#ifndef SULTANC_BOOTSTRAP_LLVM_EXPRESSIONS_H
#define SULTANC_BOOTSTRAP_LLVM_EXPRESSIONS_H

#include "llvm/llvm_internal.h"

/* Emits the LLVM trap if. */
int __LLVM_Emit_Trap_If__(__LLVM_Emitter__ *emitter,
                          LLVMValueRef condition,
                          const char *reason);
__LLVM_Value__ __LLVM_Emit_Atom__(__LLVM_Emitter__ *emitter,
                                  __Ast_Atom__ *atom,
                                  __Ast_Type__ *expected);
/* Emits the LLVM expression. */
__LLVM_Value__ __LLVM_Emit_Expression__(__LLVM_Emitter__ *emitter,
                                        __Ast_Expression__ *expression,
                                        __Ast_Type__ *expected);

#endif
