/* Declares Stage0 LLVM emission entry points. */

#ifndef SULTANC_BOOTSTRAP_LLVM_EMITTER_H
/* Defines the sultanc bootstrap LLVM emitter h macro. */
#define SULTANC_BOOTSTRAP_LLVM_EMITTER_H

#include "semantic/context.h"
#include "target/bootstrap_target.h"

/* Emits the bootstrap LLVM object. */
int __Bootstrap_Emit_LLVM_Object__(__Semantic_Context__ *semantic,
                                  const char *path,
                                  const __Bootstrap_Target__ *target);

/* Returns the bootstrap LLVM emitter error. */
const char *__Bootstrap_LLVM_Emitter_Error__(void);

#endif
