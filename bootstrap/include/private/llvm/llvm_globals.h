/* Declares Stage0 LLVM lowering of module-level external data. */

#ifndef SULTANC_BOOTSTRAP_LLVM_GLOBALS_H
#define SULTANC_BOOTSTRAP_LLVM_GLOBALS_H

#include "llvm/llvm_internal.h"

/* Resolves a base identifier that names a module-level value visible from the
 * current function. Returns 0 when the lvalue does not name such a value (no
 * failure is recorded). Returns 1 when it does; *out_place is then the external
 * global's place, or has a NULL address after a recorded LLVM failure. */
int __LLVM_Resolve_Global_Place__(__LLVM_Emitter__ *emitter,
                                  __Ast_Lvalue__ *lvalue,
                                  __LLVM_Place__ *out_place);

/* Declares or reuses one external LLVM global with an exact linker name and
 * value type. Never renames: a conflicting function or differently typed global
 * with the same name is a failure. Returns NULL after a recorded failure. */
LLVMValueRef __LLVM_Declare_External_Global__(__LLVM_Emitter__ *emitter,
                                              __Text_Slice__ link_name,
                                              LLVMTypeRef value_type);

#endif
