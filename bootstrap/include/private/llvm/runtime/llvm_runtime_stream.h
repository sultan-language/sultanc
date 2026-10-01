/* Declares private Stage0 LLVM runtime stream bridges. */

#ifndef SULTANC_BOOTSTRAP_LLVM_RUNTIME_STREAM_H
#define SULTANC_BOOTSTRAP_LLVM_RUNTIME_STREAM_H

#include "llvm/llvm_internal.h"

__LLVM_Value__ __LLVM_Emit_Runtime_Stream__(__LLVM_Emitter__ *emitter,
                                            __Ast_Expression__ *expression,
                                            __Ast_Type__ *expected,
                                            int descriptor);

#endif
