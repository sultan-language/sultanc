/* Declares private Stage0 LLVM statement lowering. */

#ifndef SULTANC_BOOTSTRAP_LLVM_STATEMENTS_H
#define SULTANC_BOOTSTRAP_LLVM_STATEMENTS_H

#include "llvm/llvm_internal.h"

int __LLVM_Emit_Statement_List__(__LLVM_Emitter__ *emitter,
                                 __Ast_Block__ *block,
                                 __Ast_Type__ *return_type,
                                 int *path_terminated);

#endif
