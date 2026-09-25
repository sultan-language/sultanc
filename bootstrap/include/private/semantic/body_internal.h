/* Defines shared body-checking state and helpers. */

#ifndef SULTANC__SEMANTIC_BODY_INTERNAL_H__
#define SULTANC__SEMANTIC_BODY_INTERNAL_H__

#include "kernel/type/type.h"
#include "kernel/layout/layout.h"
#include "kernel/name/name.h"
#include "kernel/safety/flow.h"
#include "body.h"
#include "literal.h"

#include <stddef.h>

/* Defines the semantic local structure. */
typedef struct
{
    /* Stores the name kind. */
    __Ast_Lvalue_Base_Kind__ __Name_Kind__;
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* Stores the temporary. */
    __Temporary_Id__ __Temporary__;
    /* References the type. */
    __Ast_Type__ *__Type__;
    /* References the slot. */
    __Ast_Slot__ *__Slot__;
    /* Stores the scope depth. */
    size_t __Scope_Depth__;
    /* Tracks the mutable state. */
    int __Mutable__;
    /* Tracks whether the value is parameter. */
    int __Is_Parameter__;
    /* Tracks whether the known length is present. */
    int __Has_Known_Length__;
    /* Stores the known length. */
    size_t __Known_Length__;
    /* Stores the safety. */
    __Safety_Value_Fact__ __Safety__;
    /* Stores the declaration span. */
    __Source_Span__ __Declaration_Span__;
} __Semantic_Local__;

/* Defines the body safety local state structure. */
typedef struct
{
    /* Stores the safety. */
    __Safety_Value_Fact__ __Safety__;
    /* Tracks whether the known length is present. */
    int __Has_Known_Length__;
    /* Stores the known length. */
    size_t __Known_Length__;
} __Body_Safety_Local_State__;

/* Defines the body safety snapshot structure. */
typedef struct
{
    /* References the states. */
    __Body_Safety_Local_State__ *__States__;
    /* Stores the count. */
    size_t __Count__;
} __Body_Safety_Snapshot__;

/* Defines the semantic body context structure. */
typedef struct
{
    /* References the semantic. */
    __Semantic_Context__ *__Semantic__;
    /* References the function. */
    __Semantic_Function_Entry__ *__Function__;
    /* Stores the locals. */
    __Vector__ __Locals__;
    /* Stores the synthetic types. */
    __Arena__ __Synthetic_Types__;
    /* Stores the scope depth. */
    size_t __Scope_Depth__;
    /* Tracks the falls through state. */
    int __Falls_Through__;
} __Semantic_Body_Context__;

/* Stores the body builtin int type. */
extern __Ast_Type__ __Body_Builtin_Int_Type__;
/* Stores the body builtin bool type. */
extern __Ast_Type__ __Body_Builtin_Bool_Type__;
/* Stores the body builtin char type. */
extern __Ast_Type__ __Body_Builtin_Char_Type__;
/* Stores the body builtin string type. */
extern __Ast_Type__ __Body_Builtin_String_Type__;
/* Stores the body builtin void type. */
extern __Ast_Type__ __Body_Builtin_Void_Type__;
/* Stores the body builtin u 8 type. */
extern __Ast_Type__ __Body_Builtin_U8_Type__;

/* Begins the body diagnostic. */
__Diagnostic__ *
__Body_Begin_Diagnostic__(__Semantic_Body_Context__ *, __Error_Id__, __Source_Span__);

/* Records a failure for the body. */
int __Body_Fail__(__Semantic_Body_Context__ *, __Error_Id__, __Source_Span__);

/* Finds the body local. */
__Semantic_Local__ *__Body_Find_Local__(__Semantic_Body_Context__ *, const __Ast_Lvalue__ *);

/* Returns the body local index. */
size_t __Body_Local_Index__(__Semantic_Body_Context__ *, const __Semantic_Local__ *);

/* Returns the body name exists in current scope. */
int __Body_Name_Exists_In_Current_Scope__(__Semantic_Body_Context__ *, const __Semantic_Local__ *);

/* Returns the body report borrow conflict. */
int __Body_Report_Borrow_Conflict__(__Semantic_Body_Context__ *,
                                    __Semantic_Local__ *,
                                    __Source_Span__);

/* Returns the body synthetic named type. */
__Ast_Type__ *__Body_Synthetic_Named_Type__(__Semantic_Body_Context__ *, __Text_Slice__);

/* Returns the body synthetic reference type. */
__Ast_Type__ *__Body_Synthetic_Reference_Type__(__Semantic_Body_Context__ *, __Ast_Type__ *, int);

/* Returns the body synthetic result type. */
__Ast_Type__ *
__Body_Synthetic_Result_Type__(__Semantic_Body_Context__ *, __Ast_Type__ *, __Ast_Type__ *);

/* Checks whether the body is integer like. */
int __Body_Is_Integer_Like__(__Semantic_Body_Context__ *, __Ast_Type__ *, int *);

/* Checks whether the body is bool convertible. */
int __Body_Is_Bool_Convertible__(__Semantic_Body_Context__ *, __Ast_Type__ *);

/* Checks the body compatible. */
int __Body_Check_Compatible__(__Semantic_Body_Context__ *,
                              __Ast_Type__ *,
                              __Ast_Type__ *,
                              __Source_Span__);

/* Checks the body atom compatible. */
int __Body_Check_Atom_Compatible__(__Semantic_Body_Context__ *,
                                   __Ast_Type__ *,
                                   const __Ast_Atom__ *,
                                   __Source_Span__);

/* Checks the body expression compatible. */
int __Body_Check_Expression_Compatible__(__Semantic_Body_Context__ *,
                                         __Ast_Type__ *,
                                         __Ast_Expression__ *,
                                         __Source_Span__);

/* Infers the body lvalue. */
int __Body_Infer_Lvalue__(__Semantic_Body_Context__ *,
                          __Ast_Lvalue__ *,
                          __Ast_Type__ **,
                          __Semantic_Local__ **);

/* Infers the body lvalue for write. */
int __Body_Infer_Lvalue_For_Write__(__Semantic_Body_Context__ *,
                                    __Ast_Lvalue__ *,
                                    __Ast_Type__ **,
                                    __Semantic_Local__ **);

/* Checks the body assignable. */
int __Body_Check_Assignable__(__Semantic_Body_Context__ *,
                              __Ast_Lvalue__ *,
                              __Ast_Type__ **,
                              __Semantic_Local__ **);

/* Infers the body atom. */
int __Body_Infer_Atom__(__Semantic_Body_Context__ *, const __Ast_Atom__ *, __Ast_Type__ **);

/* Infers the body expression. */
int __Body_Infer_Expression__(__Semantic_Body_Context__ *, __Ast_Expression__ *, __Ast_Type__ **);

/* Returns the body declare local. */
int __Body_Declare_Local__(__Semantic_Body_Context__ *,
                           __Ast_Variable_Declaration__ *,
                           __Source_Span__);

/* Checks the body box initializer. */
int __Body_Check_Box_Init__(__Semantic_Body_Context__ *, __Ast_Statement__ *);

/* Checks the body record initializer. */
int __Body_Check_Record_Init__(__Semantic_Body_Context__ *, __Ast_Statement__ *);

/* Checks the body vector initializer. */
int __Body_Check_Vector_Init__(__Semantic_Body_Context__ *, __Ast_Statement__ *);

/* Checks the body match. */
int __Body_Check_Match__(__Semantic_Body_Context__ *, __Ast_Statement__ *, int *);

/* Checks the body block. */
int __Body_Check_Block__(__Semantic_Body_Context__ *, __Ast_Block__ *);

/* Checks the body block flow. */
int __Body_Check_Block_Flow__(__Semantic_Body_Context__ *, __Ast_Block__ *, int *);

/* Captures the body safety. */
int __Body_Safety_Capture__(__Semantic_Body_Context__ *, __Body_Safety_Snapshot__ *);

/* Restores the body safety. */
void __Body_Safety_Restore__(__Semantic_Body_Context__ *, const __Body_Safety_Snapshot__ *);

/* Merges the body safety. */
void __Body_Safety_Merge__(__Semantic_Body_Context__ *,
                           const __Body_Safety_Snapshot__ *,
                           const __Body_Safety_Snapshot__ *);

/* Releases the body safety snapshot. */
void __Body_Safety_Snapshot_Destroy__(__Body_Safety_Snapshot__ *);

/* Returns the body safety move expression. */
int __Body_Safety_Move_Expression__(__Semantic_Body_Context__ *,
                                    __Ast_Expression__ *,
                                    __Semantic_Local__ *);

/* Returns the body safety move atom. */
int __Body_Safety_Move_Atom__(__Semantic_Body_Context__ *, const __Ast_Atom__ *, __Source_Span__);

/* Returns the body safety transfer expression. */
int __Body_Safety_Transfer_Expression__(__Semantic_Body_Context__ *,
                                        __Ast_Expression__ *,
                                        __Semantic_Local__ *);

/* Checks the body safety ephemeral borrow. */
int __Body_Safety_Check_Ephemeral_Borrow__(__Semantic_Body_Context__ *, __Ast_Expression__ *);

/* Returns the body report lifetime escape. */
int __Body_Report_Lifetime_Escape__(__Semantic_Body_Context__ *,
                                    __Semantic_Local__ *,
                                    __Source_Span__);

/* Checks the body safety return reference. */
int __Body_Safety_Check_Return_Reference__(__Semantic_Body_Context__ *, __Ast_Expression__ *);

/* Returns the body safety consume contained atom. */
int __Body_Safety_Consume_Contained_Atom__(__Semantic_Body_Context__ *,
                                           __Ast_Type__ *,
                                           const __Ast_Atom__ *,
                                           __Semantic_Local__ *,
                                           __Source_Span__);

/* Returns the body safety transfer composite expression. */
int __Body_Safety_Transfer_Composite_Expression__(__Semantic_Body_Context__ *,
                                                  __Ast_Expression__ *,
                                                  __Semantic_Local__ *,
                                                  int *);

/* Returns the body safety transfer local dependency. */
int __Body_Safety_Transfer_Local_Dependency__(
    __Semantic_Body_Context__ *, __Semantic_Local__ *, __Semantic_Local__ *, __Source_Span__, int);

/* Checks the body safety return composite view. */
int __Body_Safety_Check_Return_Composite_View__(__Semantic_Body_Context__ *, __Ast_Expression__ *);

/* Releases the body safety scope. */
void __Body_Safety_Release_Scope__(__Semantic_Body_Context__ *, size_t);

/* Checks the body try builtin tagged construct. */
int __Body_Try_Check_Builtin_Tagged_Construct__(__Semantic_Body_Context__ *,
                                                __Ast_Type__ *,
                                                __Ast_Expression__ *,
                                                int *);

/* Infers the body try enum construct. */
int __Body_Try_Infer_Enum_Construct__(__Semantic_Body_Context__ *,
                                      __Ast_Expression__ *,
                                      __Ast_Type__ **,
                                      int *);

/* Infers the body try builtin call. */
int __Body_Try_Infer_Builtin_Call__(__Semantic_Body_Context__ *,
                                    __Ast_Expression__ *,
                                    __Ast_Type__ **,
                                    int *);

/* Infers the body call. */
int __Body_Infer_Call__(__Semantic_Body_Context__ *__Context__,
                        __Ast_Expression__ *__Expression__,
                        __Ast_Type__ **__Out_Type__);

/* Infers the body unary. */
int __Body_Infer_Unary__(__Semantic_Body_Context__ *__Context__,
                         __Ast_Expression__ *__Expression__,
                         __Ast_Type__ **__Out_Type__);

/* Infers the body binary. */
int __Body_Infer_Binary__(__Semantic_Body_Context__ *__Context__,
                          __Ast_Expression__ *__Expression__,
                          __Ast_Type__ **__Out_Type__);

/* Infers the body conversion. */
int __Body_Infer_Conversion__(__Semantic_Body_Context__ *__Context__,
                              __Ast_Expression__ *__Expression__,
                              __Ast_Type__ **__Out_Type__);

#endif
