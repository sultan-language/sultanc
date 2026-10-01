/* Declares private Stage0 LLVM pattern and match lowering. */

#ifndef SULTANC_BOOTSTRAP_LLVM_PATTERNS_H
#define SULTANC_BOOTSTRAP_LLVM_PATTERNS_H

#include "llvm/llvm_internal.h"

int __LLVM_Emit_Match__(__LLVM_Emitter__ *emitter,
                        __Ast_Statement__ *statement,
                        __Ast_Type__ *return_type,
                        int *path_terminated);

#endif
