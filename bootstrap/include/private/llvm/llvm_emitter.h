/* Declares Stage0 LLVM emission entry points. */

#ifndef SULTANC_BOOTSTRAP_LLVM_EMITTER_H
/* Defines the sultanc bootstrap LLVM emitter h macro. */
#define SULTANC_BOOTSTRAP_LLVM_EMITTER_H

#include "semantic/context.h"

/* Emits the bootstrap LLVM object. */
int __Bootstrap_Emit_LLVM_Object__(__Semantic_Context__ *semantic, const char *path);

/* Runs the bootstrap LLVM program. */
int __Bootstrap_Run_LLVM_Program__(__Semantic_Context__ *semantic,
                                   int argc,
                                   const char *const *argv,
                                   int *status);

/* Returns the bootstrap LLVM emitter error. */
const char *__Bootstrap_LLVM_Emitter_Error__(void);

#endif
