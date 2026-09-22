#ifndef SULTANC_BOOTSTRAP_LLVM_EMITTER_H
#define SULTANC_BOOTSTRAP_LLVM_EMITTER_H

#include "semantic/context.h"

int __Bootstrap_Emit_LLVM_Object__(__Semantic_Context__ *semantic, const char *path);

int __Bootstrap_Run_LLVM_Program__(__Semantic_Context__ *semantic,
                                   int argc,
                                   const char *const *argv,
                                   int *status);

const char *__Bootstrap_LLVM_Emitter_Error__(void);

#endif
