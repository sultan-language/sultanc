/* Declares private Stage0 LLVM runtime directory bridges. */

#ifndef SULTANC_BOOTSTRAP_LLVM_RUNTIME_DIRECTORY_H
#define SULTANC_BOOTSTRAP_LLVM_RUNTIME_DIRECTORY_H

#include "llvm/llvm_internal.h"
#include "kernel/name/name.h"

__LLVM_Value__ __LLVM_Emit_Runtime_Path_Metadata_Service__(
    __LLVM_Emitter__ *emitter,
    __Ast_Expression__ *expression,
    __Ast_Type__ *expected,
    __Name_Builtin_Function__ builtin);
__LLVM_Value__ __LLVM_Emit_Runtime_Open_Directory_Service__(__LLVM_Emitter__ *emitter,
                                                            __Ast_Expression__ *expression,
                                                            __Ast_Type__ *expected);
__LLVM_Value__ __LLVM_Emit_Runtime_Read_Directory_Entry_Service__(
    __LLVM_Emitter__ *emitter,
    __Ast_Expression__ *expression,
    __Ast_Type__ *expected);
__LLVM_Value__ __LLVM_Emit_Runtime_Close_Directory_Service__(__LLVM_Emitter__ *emitter,
                                                             __Ast_Expression__ *expression,
                                                             __Ast_Type__ *expected);

#endif
