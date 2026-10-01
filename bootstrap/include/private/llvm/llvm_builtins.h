/* Declares private Stage0 LLVM builtin lowering. */

#ifndef SULTANC_BOOTSTRAP_LLVM_BUILTINS_H
#define SULTANC_BOOTSTRAP_LLVM_BUILTINS_H

#include "llvm/llvm_internal.h"
#include "kernel/name/name.h"
#include "kernel/type/tagged.h"

__Name_Builtin_Function__ __LLVM_Builtin_Call_Identity__(__LLVM_Emitter__ *emitter,
                                                         __Ast_Expression__ *expression);
__LLVM_Value__ __LLVM_Emit_Builtin_Call__(__LLVM_Emitter__ *emitter,
                                          __Ast_Expression__ *expression,
                                          __Ast_Type__ *expected,
                                          __Ast_Type__ *tagged_target,
                                          __Name_Builtin_Function__ builtin);
__LLVM_Value__ __LLVM_Build_Runtime_Result__(__LLVM_Emitter__ *emitter,
                                             __Ast_Type__ *result_type,
                                             LLVMValueRef status,
                                             __LLVM_Value__ success_payload);
int __LLVM_Resolve_Builtin_Tagged_Construct__(__Ast_Type__ *target_type,
                                              __Ast_Expression__ *expression,
                                              __Type_Tagged_Constructor__ *out_constructor);

#endif
