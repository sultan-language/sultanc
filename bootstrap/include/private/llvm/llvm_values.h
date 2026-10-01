/* Declares private Stage0 LLVM value and place lowering. */

#ifndef SULTANC_BOOTSTRAP_LLVM_VALUES_H
#define SULTANC_BOOTSTRAP_LLVM_VALUES_H

#include "llvm/llvm_internal.h"

LLVMValueRef __LLVM_Allocate_Stack__(__LLVM_Emitter__ *emitter,
                                     LLVMTypeRef type,
                                     const char *name);
LLVMValueRef __LLVM_Compare_Text_Values__(__LLVM_Emitter__ *emitter,
                                          LLVMValueRef left,
                                          LLVMValueRef right,
                                          __Ast_Binary_Operation__ operation);
__LLVM_Value__ __LLVM_Emit_String_Literal__(__LLVM_Emitter__ *emitter,
                                            __Text_Slice__ text,
                                            __Ast_Type__ *expected);
__LLVM_Local__ *__LLVM_Find_Local__(__LLVM_Emitter__ *emitter, __Text_Slice__ name);
__LLVM_Local__ *__LLVM_Add_Local_Identity__(__LLVM_Emitter__ *emitter,
                                            __Ast_Lvalue_Base_Kind__ name_kind,
                                            __Text_Slice__ name,
                                            __Temporary_Id__ temporary,
                                            __Ast_Type__ *type);
__LLVM_Local__ *__LLVM_Add_Local__(__LLVM_Emitter__ *emitter,
                                   __Text_Slice__ name,
                                   __Ast_Type__ *type);
/* Returns the LLVM invalid value. */
__LLVM_Value__ __LLVM_Invalid_Value__(void);
/* Returns the LLVM coerce. */
__LLVM_Value__ __LLVM_Coerce__(__LLVM_Emitter__ *emitter,
                               __LLVM_Value__ source,
                               __Ast_Type__ *target);
__Ast_Type__ *__LLVM_Lvalue_Type__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue);
__LLVM_Place__ __LLVM_Emit_Place__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue);
__LLVM_Value__ __LLVM_Emit_Lvalue__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue);
__LLVM_Value__ __LLVM_Make_Text_Value__(__LLVM_Emitter__ *emitter,
                                        LLVMValueRef data,
                                        LLVMValueRef length,
                                        LLVMValueRef capacity,
                                        __Ast_Type__ *target_type);

#endif
