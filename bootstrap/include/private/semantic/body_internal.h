#ifndef SULTANC__SEMANTIC_BODY_INTERNAL_H__
#define SULTANC__SEMANTIC_BODY_INTERNAL_H__

#include "kernel/type/type.h"
#include "kernel/layout/layout.h"
#include "kernel/name/name.h"
#include "kernel/safety/flow.h"
#include "body.h"
#include "literal.h"

#include <stddef.h>

typedef struct
{
    __Ast_Lvalue_Base_Kind__ __Name_Kind__;
    __Text_Slice__ __Name__;
    __Temporary_Id__ __Temporary__;
    __Ast_Type__ *__Type__;
    __Ast_Slot__ *__Slot__;
    size_t __Scope_Depth__;
    int __Mutable__;
    int __Is_Parameter__;
    int __Has_Known_Length__;
    size_t __Known_Length__;
    __Safety_Value_Fact__ __Safety__;
    __Source_Span__ __Declaration_Span__;
} __Semantic_Local__;

typedef struct
{
    __Safety_Value_Fact__ __Safety__;
    int __Has_Known_Length__;
    size_t __Known_Length__;
} __Body_Safety_Local_State__;

typedef struct
{
    __Body_Safety_Local_State__ *__States__;
    size_t __Count__;
} __Body_Safety_Snapshot__;

typedef struct
{
    __Semantic_Context__ *__Semantic__;
    __Semantic_Function_Entry__ *__Function__;
    __Vector__ __Locals__;
    __Arena__ __Synthetic_Types__;
    size_t __Scope_Depth__;
    int __Falls_Through__;
} __Semantic_Body_Context__;

extern __Ast_Type__ __Body_Builtin_Int_Type__;
extern __Ast_Type__ __Body_Builtin_Bool_Type__;
extern __Ast_Type__ __Body_Builtin_Char_Type__;
extern __Ast_Type__ __Body_Builtin_String_Type__;
extern __Ast_Type__ __Body_Builtin_Void_Type__;
extern __Ast_Type__ __Body_Builtin_U8_Type__;

__Diagnostic__ *
__Body_Begin_Diagnostic__(__Semantic_Body_Context__ *, __Error_Id__, __Source_Span__);

int __Body_Fail__(__Semantic_Body_Context__ *, __Error_Id__, __Source_Span__);

__Semantic_Local__ *__Body_Find_Local__(__Semantic_Body_Context__ *, const __Ast_Lvalue__ *);

size_t __Body_Local_Index__(__Semantic_Body_Context__ *, const __Semantic_Local__ *);

int __Body_Name_Exists_In_Current_Scope__(__Semantic_Body_Context__ *, const __Semantic_Local__ *);

int __Body_Report_Borrow_Conflict__(__Semantic_Body_Context__ *,
                                    __Semantic_Local__ *,
                                    __Source_Span__);

__Ast_Type__ *__Body_Synthetic_Named_Type__(__Semantic_Body_Context__ *, __Text_Slice__);

__Ast_Type__ *__Body_Synthetic_Reference_Type__(__Semantic_Body_Context__ *, __Ast_Type__ *, int);

__Ast_Type__ *
__Body_Synthetic_Result_Type__(__Semantic_Body_Context__ *, __Ast_Type__ *, __Ast_Type__ *);

int __Body_Is_Integer_Like__(__Semantic_Body_Context__ *, __Ast_Type__ *, int *);

int __Body_Is_Bool_Convertible__(__Semantic_Body_Context__ *, __Ast_Type__ *);

int __Body_Check_Compatible__(__Semantic_Body_Context__ *,
                              __Ast_Type__ *,
                              __Ast_Type__ *,
                              __Source_Span__);

int __Body_Check_Atom_Compatible__(__Semantic_Body_Context__ *,
                                   __Ast_Type__ *,
                                   const __Ast_Atom__ *,
                                   __Source_Span__);

int __Body_Check_Expression_Compatible__(__Semantic_Body_Context__ *,
                                         __Ast_Type__ *,
                                         __Ast_Expression__ *,
                                         __Source_Span__);

int __Body_Infer_Lvalue__(__Semantic_Body_Context__ *,
                          __Ast_Lvalue__ *,
                          __Ast_Type__ **,
                          __Semantic_Local__ **);

int __Body_Infer_Lvalue_For_Write__(__Semantic_Body_Context__ *,
                                    __Ast_Lvalue__ *,
                                    __Ast_Type__ **,
                                    __Semantic_Local__ **);

int __Body_Check_Assignable__(__Semantic_Body_Context__ *,
                              __Ast_Lvalue__ *,
                              __Ast_Type__ **,
                              __Semantic_Local__ **);

int __Body_Infer_Atom__(__Semantic_Body_Context__ *, const __Ast_Atom__ *, __Ast_Type__ **);

int __Body_Infer_Expression__(__Semantic_Body_Context__ *, __Ast_Expression__ *, __Ast_Type__ **);

int __Body_Declare_Local__(__Semantic_Body_Context__ *,
                           __Ast_Variable_Declaration__ *,
                           __Source_Span__);

int __Body_Check_Box_Init__(__Semantic_Body_Context__ *, __Ast_Statement__ *);

int __Body_Check_Record_Init__(__Semantic_Body_Context__ *, __Ast_Statement__ *);

int __Body_Check_Vector_Init__(__Semantic_Body_Context__ *, __Ast_Statement__ *);

int __Body_Check_Match__(__Semantic_Body_Context__ *, __Ast_Statement__ *, int *);

int __Body_Check_Block__(__Semantic_Body_Context__ *, __Ast_Block__ *);

int __Body_Check_Block_Flow__(__Semantic_Body_Context__ *, __Ast_Block__ *, int *);

int __Body_Safety_Capture__(__Semantic_Body_Context__ *, __Body_Safety_Snapshot__ *);

void __Body_Safety_Restore__(__Semantic_Body_Context__ *, const __Body_Safety_Snapshot__ *);

void __Body_Safety_Merge__(__Semantic_Body_Context__ *,
                           const __Body_Safety_Snapshot__ *,
                           const __Body_Safety_Snapshot__ *);

void __Body_Safety_Snapshot_Destroy__(__Body_Safety_Snapshot__ *);

int __Body_Safety_Move_Expression__(__Semantic_Body_Context__ *,
                                    __Ast_Expression__ *,
                                    __Semantic_Local__ *);

int __Body_Safety_Move_Atom__(__Semantic_Body_Context__ *, const __Ast_Atom__ *, __Source_Span__);

int __Body_Safety_Transfer_Expression__(__Semantic_Body_Context__ *,
                                        __Ast_Expression__ *,
                                        __Semantic_Local__ *);

int __Body_Safety_Check_Ephemeral_Borrow__(__Semantic_Body_Context__ *, __Ast_Expression__ *);

int __Body_Report_Lifetime_Escape__(__Semantic_Body_Context__ *,
                                    __Semantic_Local__ *,
                                    __Source_Span__);

int __Body_Safety_Check_Return_Reference__(__Semantic_Body_Context__ *, __Ast_Expression__ *);

int __Body_Safety_Consume_Contained_Atom__(__Semantic_Body_Context__ *,
                                           __Ast_Type__ *,
                                           const __Ast_Atom__ *,
                                           __Semantic_Local__ *,
                                           __Source_Span__);

int __Body_Safety_Transfer_Composite_Expression__(__Semantic_Body_Context__ *,
                                                  __Ast_Expression__ *,
                                                  __Semantic_Local__ *,
                                                  int *);

int __Body_Safety_Transfer_Local_Dependency__(
    __Semantic_Body_Context__ *, __Semantic_Local__ *, __Semantic_Local__ *, __Source_Span__, int);

int __Body_Safety_Check_Return_Composite_View__(__Semantic_Body_Context__ *, __Ast_Expression__ *);

void __Body_Safety_Release_Scope__(__Semantic_Body_Context__ *, size_t);

int __Body_Try_Check_Builtin_Tagged_Construct__(__Semantic_Body_Context__ *,
                                                __Ast_Type__ *,
                                                __Ast_Expression__ *,
                                                int *);

int __Body_Try_Infer_Enum_Construct__(__Semantic_Body_Context__ *,
                                      __Ast_Expression__ *,
                                      __Ast_Type__ **,
                                      int *);

int __Body_Try_Infer_Builtin_Call__(__Semantic_Body_Context__ *,
                                    __Ast_Expression__ *,
                                    __Ast_Type__ **,
                                    int *);

int __Body_Infer_Call__(__Semantic_Body_Context__ *__Context__,
                        __Ast_Expression__ *__Expression__,
                        __Ast_Type__ **__Out_Type__);

int __Body_Infer_Unary__(__Semantic_Body_Context__ *__Context__,
                         __Ast_Expression__ *__Expression__,
                         __Ast_Type__ **__Out_Type__);

int __Body_Infer_Binary__(__Semantic_Body_Context__ *__Context__,
                          __Ast_Expression__ *__Expression__,
                          __Ast_Type__ **__Out_Type__);

int __Body_Infer_Conversion__(__Semantic_Body_Context__ *__Context__,
                              __Ast_Expression__ *__Expression__,
                              __Ast_Type__ **__Out_Type__);

#endif
