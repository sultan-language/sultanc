#include "semantic/body_internal.h"

int __Body_Report_Borrow_Conflict__(__Semantic_Body_Context__ *__Context__,
                                    __Semantic_Local__ *__Source__,
                                    __Source_Span__ __Span__)
{
    __Diagnostic__ *__Diagnostic_State__ =
        __Body_Begin_Diagnostic__(__Context__, __E1302_Borrow_Conflict__, __Span__);

    if (__Diagnostic_State__ != NULL)
    {
        __Diagnostic_Set_Message_Key__(__Diagnostic_State__, __Diag_Word_Borrow_Conflict_Causal__);
        __Diagnostic_Set_Primary_Role__(__Diagnostic_State__,
                                        __Diagnostic_Span_Role_Conflicting_Borrow__);
        if (__Source__ != NULL && __Source__->__Name_Kind__ == __Ast_Lvalue_Base_Identifier__)
        {
            __Diagnostic_Set_Argument_Text__(
                __Diagnostic_State__, __Diagnostic_Argument_Value__, __Source__->__Name__);
        }
        if (__Source__ != NULL && __Source__->__Safety__.__Has_Active_Borrow_Span__)
        {
            __Diagnostic_Add_Related_Span__(__Diagnostic_State__,
                                            __Source__->__Safety__.__Active_Borrow_Span__,
                                            __Diagnostic_Span_Role_Mutable_Borrow_Start__);
        }
        if (__Source__ != NULL && __Source__->__Safety__.__Has_Origin_Span__)
        {
            __Diagnostic_Add_Related_Span__(__Diagnostic_State__,
                                            __Source__->__Safety__.__Origin_Span__,
                                            __Diagnostic_Span_Role_Original_Owner__);
        }
    }
    return 0;
}

int __Body_Report_Lifetime_Escape__(__Semantic_Body_Context__ *__Context__,
                                    __Semantic_Local__ *__Source__,
                                    __Source_Span__ __Span__)
{
    __Diagnostic__ *__Diagnostic_State__ =
        __Body_Begin_Diagnostic__(__Context__, __E1303_Reference_Escape__, __Span__);

    if (__Diagnostic_State__ != NULL)
    {
        __Source_Span__ __Scope_End__ = {0};

        __Diagnostic_Set_Message_Key__(__Diagnostic_State__, __Diag_Word_Lifetime_Escape_Causal__);
        __Diagnostic_Set_Primary_Role__(__Diagnostic_State__,
                                        __Diagnostic_Span_Role_Reference_Escape__);
        if (__Source__ != NULL && __Source__->__Name_Kind__ == __Ast_Lvalue_Base_Identifier__)
        {
            __Diagnostic_Set_Argument_Text__(
                __Diagnostic_State__, __Diagnostic_Argument_Value__, __Source__->__Name__);
            __Diagnostic_Add_Related_Span__(__Diagnostic_State__,
                                            __Source__->__Declaration_Span__,
                                            __Diagnostic_Span_Role_Local_Created__);
        }
        if (__Context__->__Function__ != NULL && __Context__->__Function__->__Function__ != NULL &&
            __Context__->__Function__->__Function__->__Body__ != NULL)
        {
            __Scope_End__.__Start__ =
                __Context__->__Function__->__Function__->__Body__->__Header__.__Span__.__End__;
            __Scope_End__.__End__ = __Scope_End__.__Start__;
            __Diagnostic_Add_Related_Span__(
                __Diagnostic_State__, __Scope_End__, __Diagnostic_Span_Role_Scope_End__);
        }
        __Diagnostic_Add_Help__(__Diagnostic_State__, __Diag_Word_Help_Return_Owned__);
    }
    return 0;
}

static __Ast_Lvalue__ *__Body_Root_Lvalue__(__Ast_Lvalue__ *__Lvalue__)
{
    if (__Lvalue__ == NULL)
    {
        return NULL;
    }
    if (__Lvalue__->__Kind__ == __Ast_Lvalue_Base__)
    {
        return __Lvalue__;
    }
    if (__Lvalue__->__Kind__ == __Ast_Lvalue_Field__)
    {
        return __Body_Root_Lvalue__(__Lvalue__->__As__.__Field__.__Parent__);
    }
    if (__Lvalue__->__Kind__ == __Ast_Lvalue_Index__)
    {
        return __Body_Root_Lvalue__(__Lvalue__->__As__.__Index__.__Parent__);
    }
    if (__Lvalue__->__Kind__ == __Ast_Lvalue_Dereference__)
    {
        return __Body_Root_Lvalue__(__Lvalue__->__As__.__Dereference_Parent__);
    }
    return NULL;
}

static __Semantic_Local__ *__Body_Borrow_Source__(__Semantic_Body_Context__ *__Context__,
                                                  __Ast_Expression__ *__Expression__,
                                                  int *__Out_Mutable__)
{
    __Ast_Expression__ *__Operand__;
    __Ast_Lvalue__ *__Root__;

    if (__Expression__ == NULL || __Expression__->__Kind__ != __Ast_Expression_Unary__ ||
        (__Expression__->__As__.__Unary__.__Operation__ != __Unary_Address__ &&
         __Expression__->__As__.__Unary__.__Operation__ != __Unary_Address_Mutable__))
    {
        return NULL;
    }

    *__Out_Mutable__ = __Expression__->__As__.__Unary__.__Operation__ == __Unary_Address_Mutable__;
    __Operand__ = __Expression__->__As__.__Unary__.__Operand__;
    if (__Operand__ == NULL || __Operand__->__Kind__ != __Ast_Expression_Atom__ ||
        __Operand__->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__)
    {
        return NULL;
    }

    __Root__ = __Body_Root_Lvalue__(__Operand__->__As__.__Atom__.__As__.__Lvalue__);
    return __Root__ == NULL ? NULL : __Body_Find_Local__(__Context__, __Root__);
}

static void __Body_Safety_Release_Local_Borrow__(__Semantic_Body_Context__ *__Context__,
                                                 __Semantic_Local__ *__Local__)
{
    size_t __Owner_Index__;
    __Semantic_Local__ *__Owner__;

    if (__Local__ == NULL || __Local__->__Safety__.__Borrow_Kind__ == __Safety_Borrow_None__ ||
        __Local__->__Safety__.__Borrowed_From__ == SIZE_MAX)
    {
        return;
    }

    __Owner_Index__ = __Local__->__Safety__.__Borrowed_From__;
    __Owner__ = (__Semantic_Local__ *)__Vector_At__(&__Context__->__Locals__, __Owner_Index__);
    if (__Owner__ != NULL)
    {
        __Safety_Fact_Release_Borrow__(&__Owner__->__Safety__,
                                       __Local__->__Safety__.__Borrow_Kind__);
    }
    __Local__->__Safety__.__Borrowed_From__ = SIZE_MAX;
    __Local__->__Safety__.__Borrow_Kind__ = __Safety_Borrow_None__;
    __Local__->__Safety__.__Lifetime_Region__ = SIZE_MAX;
    __Local__->__Safety__.__Origin_Parameter__ = SIZE_MAX;
}

void __Body_Safety_Release_Scope__(__Semantic_Body_Context__ *__Context__, size_t __Saved_Count__)
{
    size_t __Index__ = __Context__->__Locals__.__Count__;

    while (__Index__ > __Saved_Count__)
    {
        __Semantic_Local__ *__Local__ =
            (__Semantic_Local__ *)__Vector_At__(&__Context__->__Locals__, __Index__ - 1U);
        --__Index__;
        __Body_Safety_Release_Local_Borrow__(__Context__, __Local__);
    }
}

static int __Body_Safety_Acquire_Borrow__(__Semantic_Body_Context__ *__Context__,
                                          __Ast_Expression__ *__Expression__,
                                          __Semantic_Local__ *__Destination__)
{
    int __Mutable__ = 0;
    __Semantic_Local__ *__Source__ =
        __Body_Borrow_Source__(__Context__, __Expression__, &__Mutable__);
    size_t __Source_Index__;
    size_t __Destination_Index__ = SIZE_MAX;

    if (__Source__ == NULL)
    {
        return 0;
    }

    __Source_Index__ = __Body_Local_Index__(__Context__, __Source__);
    if (__Source_Index__ == SIZE_MAX)
    {
        return 0;
    }
    if (__Destination__ != NULL)
    {
        __Destination_Index__ = __Body_Local_Index__(__Context__, __Destination__);
    }

    if (__Mutable__ && !__Source__->__Mutable__)
    {
        return __Body_Fail__(
            __Context__, __E0703_Write_To_Immutable__, __Expression__->__Header__.__Span__);
    }

    if (__Mutable__ ? !__Safety_Fact_Acquire_Mutable_At__(&__Source__->__Safety__,
                                                          __Expression__->__Header__.__Span__,
                                                          __Destination_Index__)
                    : !__Safety_Fact_Acquire_Shared_At__(&__Source__->__Safety__,
                                                         __Expression__->__Header__.__Span__))
    {
        return __Body_Report_Borrow_Conflict__(
            __Context__, __Source__, __Expression__->__Header__.__Span__);
    }

    if (__Destination__ == NULL)
    {
        __Safety_Fact_Release_Borrow__(&__Source__->__Safety__,
                                       __Mutable__ ? __Safety_Borrow_Mutable__
                                                   : __Safety_Borrow_Shared__);
        return 1;
    }

    if (__Destination__->__Scope_Depth__ < __Source__->__Scope_Depth__)
    {
        __Safety_Fact_Release_Borrow__(&__Source__->__Safety__,
                                       __Mutable__ ? __Safety_Borrow_Mutable__
                                                   : __Safety_Borrow_Shared__);
        return __Body_Report_Lifetime_Escape__(
            __Context__, __Source__, __Expression__->__Header__.__Span__);
    }

    __Destination__->__Safety__.__Borrowed_From__ = __Source_Index__;
    __Destination__->__Safety__.__Borrow_Kind__ =
        __Mutable__ ? __Safety_Borrow_Mutable__ : __Safety_Borrow_Shared__;
    __Destination__->__Safety__.__Lifetime_Region__ = __Source__->__Scope_Depth__;
    __Destination__->__Safety__.__Origin_Parameter__ = SIZE_MAX;
    return 1;
}

static int __Body_Safety_Copy_Reference__(__Semantic_Body_Context__ *__Context__,
                                          __Semantic_Local__ *__Source__,
                                          __Semantic_Local__ *__Destination__,
                                          __Source_Span__ __Span__)
{
    int __Mutable__ = 0;

    if (!__Safety_Type_Is_Reference__(__Source__->__Type__, &__Mutable__))
    {
        return 0;
    }

    if (__Mutable__)
    {
        size_t __Borrowed_From__ = __Source__->__Safety__.__Borrowed_From__;
        __Safety_Borrow_Kind__ __Borrow_Kind__ = __Source__->__Safety__.__Borrow_Kind__;
        size_t __Region__ = __Source__->__Safety__.__Lifetime_Region__;
        size_t __Origin_Parameter__ = __Source__->__Safety__.__Origin_Parameter__;

        if (!__Safety_Fact_Can_Move__(&__Source__->__Safety__))
        {
            return __Body_Fail__(__Context__, __E1302_Borrow_Conflict__, __Span__);
        }
        __Safety_Fact_Mark_Moved_At__(&__Source__->__Safety__, __Span__);
        __Source__->__Safety__.__Borrowed_From__ = SIZE_MAX;
        __Source__->__Safety__.__Borrow_Kind__ = __Safety_Borrow_None__;
        __Destination__->__Safety__.__Borrowed_From__ = __Borrowed_From__;
        __Destination__->__Safety__.__Borrow_Kind__ = __Borrow_Kind__;
        __Destination__->__Safety__.__Lifetime_Region__ = __Region__;
        __Destination__->__Safety__.__Origin_Parameter__ = __Origin_Parameter__;
        if (__Borrowed_From__ != SIZE_MAX)
        {
            __Semantic_Local__ *__Owner__ =
                (__Semantic_Local__ *)__Vector_At__(&__Context__->__Locals__, __Borrowed_From__);
            const size_t __Destination_Index__ = __Body_Local_Index__(__Context__, __Destination__);
            __Safety_Fact_Set_Mutable_Borrow_Holder__(
                __Owner__ != NULL ? &__Owner__->__Safety__ : NULL, __Destination_Index__);
        }
        return 1;
    }

    if (__Source__->__Safety__.__Borrowed_From__ != SIZE_MAX)
    {
        __Semantic_Local__ *__Owner__ = (__Semantic_Local__ *)__Vector_At__(
            &__Context__->__Locals__, __Source__->__Safety__.__Borrowed_From__);
        if (__Owner__ == NULL ||
            !__Safety_Fact_Acquire_Shared_At__(&__Owner__->__Safety__, __Span__))
        {
            return __Body_Fail__(__Context__, __E1302_Borrow_Conflict__, __Span__);
        }
    }

    __Destination__->__Safety__.__Borrowed_From__ = __Source__->__Safety__.__Borrowed_From__;
    __Destination__->__Safety__.__Borrow_Kind__ = __Safety_Borrow_Shared__;
    __Destination__->__Safety__.__Lifetime_Region__ = __Source__->__Safety__.__Lifetime_Region__;
    __Destination__->__Safety__.__Origin_Parameter__ = __Source__->__Safety__.__Origin_Parameter__;
    return 1;
}

static __Ast_Expression__ *__Body_Reference_Call_Source__(__Semantic_Body_Context__ *__Context__,
                                                          __Ast_Expression__ *__Expression__)
{
    __Ast_Lvalue__ *__Function_Lvalue__;
    __Semantic_Function_Entry__ *__Callee__;
    size_t __Parameter_Index__;

    if (__Expression__ == NULL || __Expression__->__Kind__ != __Ast_Expression_Call__)
    {
        return NULL;
    }
    __Function_Lvalue__ = __Expression__->__As__.__Call__.__Function__;
    if (__Function_Lvalue__ == NULL || __Function_Lvalue__->__Kind__ != __Ast_Lvalue_Base__ ||
        __Function_Lvalue__->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__ ||
        __Body_Find_Local__(__Context__, __Function_Lvalue__) != NULL)
    {
        return NULL;
    }
    __Callee__ = __Name_Find_Function__(__Context__->__Semantic__,
                                        __Function_Lvalue__->__As__.__Base__.__As__.__Identifier__);
    if (__Callee__ == NULL || __Callee__->__Reference_Return_Parameter__ == SIZE_MAX)
    {
        return NULL;
    }
    __Parameter_Index__ = __Callee__->__Reference_Return_Parameter__;
    if (__Parameter_Index__ >= __Expression__->__As__.__Call__.__Argument_Count__)
    {
        return NULL;
    }
    return __Expression__->__As__.__Call__.__Arguments__[__Parameter_Index__];
}

int __Body_Safety_Transfer_Expression__(__Semantic_Body_Context__ *__Context__,
                                        __Ast_Expression__ *__Expression__,
                                        __Semantic_Local__ *__Destination__)
{
    __Ast_Lvalue__ *__Lvalue__;
    __Semantic_Local__ *__Source__;
    __Ast_Expression__ *__Reference_Source__ = __Expression__;

    if (__Destination__ != NULL && __Safety_Type_Is_Reference__(__Destination__->__Type__, NULL))
    {
        __Ast_Expression__ *__Call_Source__ =
            __Body_Reference_Call_Source__(__Context__, __Expression__);
        if (__Call_Source__ != NULL)
        {
            __Reference_Source__ = __Call_Source__;
        }
        else if (__Expression__ != NULL && __Expression__->__Kind__ == __Ast_Expression_Call__)
        {
            return __Body_Fail__(
                __Context__, __E1303_Reference_Escape__, __Expression__->__Header__.__Span__);
        }

        __Body_Safety_Release_Local_Borrow__(__Context__, __Destination__);
        if (__Body_Borrow_Source__(__Context__, __Reference_Source__, &(int){0}) != NULL)
        {
            return __Body_Safety_Acquire_Borrow__(
                __Context__, __Reference_Source__, __Destination__);
        }

        if (__Reference_Source__ != NULL &&
            __Reference_Source__->__Kind__ == __Ast_Expression_Atom__ &&
            __Reference_Source__->__As__.__Atom__.__Kind__ == __Ast_Atom_Lvalue__)
        {
            __Lvalue__ = __Reference_Source__->__As__.__Atom__.__As__.__Lvalue__;
            if (__Lvalue__ != NULL && __Lvalue__->__Kind__ == __Ast_Lvalue_Base__)
            {
                __Source__ = __Body_Find_Local__(__Context__, __Lvalue__);
                if (__Source__ != NULL && __Safety_Type_Is_Reference__(__Source__->__Type__, NULL))
                {
                    return __Body_Safety_Copy_Reference__(
                        __Context__,
                        __Source__,
                        __Destination__,
                        __Reference_Source__->__Header__.__Span__);
                }
            }
        }
    }

    if (__Destination__ != NULL)
    {
        int __Composite_Matched__ = 0;
        if (!__Body_Safety_Transfer_Composite_Expression__(
                __Context__, __Expression__, __Destination__, &__Composite_Matched__))
        {
            return 0;
        }
        if (__Composite_Matched__)
        {
            return 1;
        }
    }

    return __Body_Safety_Move_Expression__(__Context__, __Expression__, __Destination__);
}

int __Body_Safety_Check_Ephemeral_Borrow__(__Semantic_Body_Context__ *__Context__,
                                           __Ast_Expression__ *__Expression__)
{
    int __Mutable__ = 0;
    __Semantic_Local__ *__Source__ =
        __Body_Borrow_Source__(__Context__, __Expression__, &__Mutable__);

    if (__Source__ == NULL)
    {
        return 1;
    }
    if (__Mutable__ && !__Source__->__Mutable__)
    {
        return __Body_Fail__(
            __Context__, __E0703_Write_To_Immutable__, __Expression__->__Header__.__Span__);
    }
    if (__Mutable__ ? !__Safety_Fact_Acquire_Mutable_At__(
                          &__Source__->__Safety__, __Expression__->__Header__.__Span__, SIZE_MAX)
                    : !__Safety_Fact_Acquire_Shared_At__(&__Source__->__Safety__,
                                                         __Expression__->__Header__.__Span__))
    {
        return __Body_Report_Borrow_Conflict__(
            __Context__, __Source__, __Expression__->__Header__.__Span__);
    }
    __Safety_Fact_Release_Borrow__(&__Source__->__Safety__,
                                   __Mutable__ ? __Safety_Borrow_Mutable__
                                               : __Safety_Borrow_Shared__);
    return 1;
}

int __Body_Safety_Check_Return_Reference__(__Semantic_Body_Context__ *__Context__,
                                           __Ast_Expression__ *__Expression__)
{
    __Resolved_Type__ __Return_Type__;
    __Ast_Expression__ *__Origin_Expression__ = __Expression__;

    if (!__Type_Resolve__(__Context__->__Semantic__,
                          __Context__->__Function__->__Function__->__Output__.__Type__,
                          &__Return_Type__) ||
        __Return_Type__.__Kind__ != __Resolved_Type_Reference__)
    {
        return 1;
    }

    if (__Expression__ == NULL)
    {
        return 1;
    }

    {
        __Ast_Expression__ *__Call_Source__ =
            __Body_Reference_Call_Source__(__Context__, __Expression__);
        if (__Call_Source__ != NULL)
        {
            __Origin_Expression__ = __Call_Source__;
        }
    }

    if (__Origin_Expression__->__Kind__ == __Ast_Expression_Atom__ &&
        __Origin_Expression__->__As__.__Atom__.__Kind__ == __Ast_Atom_Lvalue__)
    {
        __Ast_Lvalue__ *__Lvalue__ = __Origin_Expression__->__As__.__Atom__.__As__.__Lvalue__;
        __Semantic_Local__ *__Local__ =
            __Lvalue__ != NULL && __Lvalue__->__Kind__ == __Ast_Lvalue_Base__
                ? __Body_Find_Local__(__Context__, __Lvalue__)
                : NULL;
        if (__Local__ != NULL && __Safety_Type_Is_Reference__(__Local__->__Type__, NULL) &&
            __Local__->__Safety__.__Lifetime_Region__ == 0U &&
            __Local__->__Safety__.__Origin_Parameter__ != SIZE_MAX)
        {
            size_t __Origin__ = __Local__->__Safety__.__Origin_Parameter__;
            if (__Context__->__Function__->__Reference_Return_Parameter__ == SIZE_MAX)
            {
                __Context__->__Function__->__Reference_Return_Parameter__ = __Origin__;
            }
            else if (__Context__->__Function__->__Reference_Return_Parameter__ != __Origin__)
            {
                return __Body_Fail__(
                    __Context__, __E1303_Reference_Escape__, __Expression__->__Header__.__Span__);
            }
            return 1;
        }
    }

    {
        int __Mutable__ = 0;
        __Semantic_Local__ *__Source__ =
            __Body_Borrow_Source__(__Context__, __Origin_Expression__, &__Mutable__);
        (void)__Mutable__;
        return __Body_Report_Lifetime_Escape__(
            __Context__, __Source__, __Expression__->__Header__.__Span__);
    }
}
