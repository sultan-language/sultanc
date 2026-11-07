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
/* Returns the live owner of an owned match scrutinee, when the source is
 * a local value (rather than a borrowed projection or fresh temporary). */
LLVMValueRef __LLVM_Match_Source_Owner_Flag__(__LLVM_Emitter__ *emitter,
                                             __Ast_Expression__ *expression);
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

/* Runtime borrowed provenance; owns no new storage. */
LLVMValueRef __LLVM_Expression_Borrowed_Flag__(__LLVM_Emitter__ *emitter,
                                              __Ast_Expression__ *expression);
int __LLVM_Mark_Copy_Ownership__(__LLVM_Emitter__ *emitter,
                                  __Ast_Lvalue__ *destination,
                                  LLVMValueRef borrowed_flag);

/* Releases the previous owned value before replacing it. Base locals use
 * their runtime drop flag; projections and globals release the already-
 * evaluated destination place directly. The caller must evaluate a place
 * only once, since an indexed place can have side effects. */
int __LLVM_Drop_Before_Overwrite__(__LLVM_Emitter__ *emitter,
                                   __Ast_Lvalue__ *destination,
                                   __LLVM_Place__ place);
/* Marks a Base-identifier local's drop flag true: it now holds a live
 * value this function must release. No-op when there is no drop flag. */
int __LLVM_Mark_Local_Owning__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *destination);
/* Marks a parameter's local (already bound by identity, not looked up by
 * lvalue) drop flag true at function entry. */
int __LLVM_Mark_Parameter_Owning__(__LLVM_Emitter__ *emitter, __LLVM_Local__ *local);
/* Drops every still-flagged local in the current function, in reverse
 * declaration order, before a `ret`/`ret void` is built -- covers both the
 * explicit Return statement and the implicit function-body fallthrough. */
int __LLVM_Drop_All_Locals__(__LLVM_Emitter__ *emitter);
/* Scope-exit drop of locals declared at index `first` and later (match
 * arms, which pop their locals on exit). Flag-gated; clears the flags. */
int __LLVM_Drop_Locals_From__(__LLVM_Emitter__ *emitter, size_t first);

#endif
