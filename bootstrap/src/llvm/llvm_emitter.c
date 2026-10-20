/* Exposes the Stage0 LLVM object emitter status. */

#include "llvm/llvm_internal.h"
#include "llvm/llvm_context.h"

/* Returns the bootstrap LLVM emitter error. */
const char *__Bootstrap_LLVM_Emitter_Error__(void)
{
    return __LLVM_Error__;
}
