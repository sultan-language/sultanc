#include "semantic/body_internal.h"
#include "kernel/memory/memory.h"

static __Semantic_Local__ *__Body_Composite_Source_Local__(__Semantic_Body_Context__ *__Context__,
                                                           const __Ast_Atom__ *__Atom__)
{
    if (__Atom__ == NULL || __Atom__->__Kind__ != __Ast_Atom_Lvalue__ ||
        __Atom__->__As__.__Lvalue__ == NULL ||
        __Atom__->__As__.__Lvalue__->__Kind__ != __Ast_Lvalue_Base__)
    {
        return NULL;
    }
    return __Body_Find_Local__(__Context__, __Atom__->__As__.__Lvalue__);
}

static int __Body_Local_Has_View_Dependency__(const __Semantic_Local__ *__Local__)
{
    return __Local__ != NULL && (__Local__->__Safety__.__Borrowed_From__ != SIZE_MAX ||
                                 __Local__->__Safety__.__Borrow_Kind__ != __Safety_Borrow_None__ ||
                                 __Local__->__Safety__.__Lifetime_Region__ != SIZE_MAX);
}

int __Body_Safety_Transfer_Local_Dependency__(__Semantic_Body_Context__ *__Context__,
                                              __Semantic_Local__ *__Source__,
                                              __Semantic_Local__ *__Destination__,
                                              __Source_Span__ __Span__,
                                              int __Move__)
{
    __Semantic_Local__ *__Owner__ = NULL;
    size_t __Destination_Index__;

    if (__Destination__ == NULL || !__Body_Local_Has_View_Dependency__(__Source__))
    {
        return 1;
    }
    if (__Body_Local_Has_View_Dependency__(__Destination__))
    {
        return __Body_Fail__(__Context__, __E1303_Reference_Escape__, __Span__);
    }
    if (__Source__->__Safety__.__Lifetime_Region__ != SIZE_MAX &&
        __Source__->__Safety__.__Lifetime_Region__ != 0U &&
        __Destination__->__Scope_Depth__ < __Source__->__Safety__.__Lifetime_Region__)
    {
        return __Body_Report_Lifetime_Escape__(__Context__, __Source__, __Span__);
    }

    if (__Source__->__Safety__.__Borrowed_From__ != SIZE_MAX)
    {
        __Owner__ = (__Semantic_Local__ *)__Vector_At__(&__Context__->__Locals__,
                                                        __Source__->__Safety__.__Borrowed_From__);
        if (__Owner__ == NULL)
        {
            return __Body_Fail__(__Context__, __E1100_Internal_Context_Error__, __Span__);
        }
        if (!__Move__)
        {
            if (__Source__->__Safety__.__Borrow_Kind__ != __Safety_Borrow_Shared__ ||
                !__Safety_Fact_Acquire_Shared_At__(&__Owner__->__Safety__, __Span__))
            {
                return __Body_Report_Borrow_Conflict__(__Context__, __Owner__, __Span__);
            }
        }
    }

    __Destination__->__Safety__.__Borrowed_From__ = __Source__->__Safety__.__Borrowed_From__;
    __Destination__->__Safety__.__Borrow_Kind__ = __Source__->__Safety__.__Borrow_Kind__;
    __Destination__->__Safety__.__Lifetime_Region__ = __Source__->__Safety__.__Lifetime_Region__;
    __Destination__->__Safety__.__Origin_Parameter__ = __Source__->__Safety__.__Origin_Parameter__;

    if (__Move__)
    {
        if (__Owner__ != NULL &&
            __Source__->__Safety__.__Borrow_Kind__ == __Safety_Borrow_Mutable__)
        {
            __Destination_Index__ = __Body_Local_Index__(__Context__, __Destination__);
            __Safety_Fact_Set_Mutable_Borrow_Holder__(&__Owner__->__Safety__,
                                                      __Destination_Index__);
        }
        __Source__->__Safety__.__Borrowed_From__ = SIZE_MAX;
        __Source__->__Safety__.__Borrow_Kind__ = __Safety_Borrow_None__;
        __Source__->__Safety__.__Lifetime_Region__ = SIZE_MAX;
        __Source__->__Safety__.__Origin_Parameter__ = SIZE_MAX;
    }
    return 1;
}

int __Body_Safety_Consume_Contained_Atom__(__Semantic_Body_Context__ *__Context__,
                                           __Ast_Type__ *__Expected_Type__,
                                           const __Ast_Atom__ *__Atom__,
                                           __Semantic_Local__ *__Destination__,
                                           __Source_Span__ __Span__)
{
    __Semantic_Local__ *__Source__ = __Body_Composite_Source_Local__(__Context__, __Atom__);
    int __Move__ = __Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Expected_Type__);

    if (__Source__ != NULL && __Body_Local_Has_View_Dependency__(__Source__))
    {
        if (__Move__ && !__Body_Safety_Move_Atom__(__Context__, __Atom__, __Span__))
        {
            return 0;
        }
        if (!__Body_Safety_Transfer_Local_Dependency__(
                __Context__, __Source__, __Destination__, __Span__, __Move__))
        {
            return 0;
        }
        return 1;
    }

    return !__Move__ || __Body_Safety_Move_Atom__(__Context__, __Atom__, __Span__);
}

static __Semantic_Local__ *
__Body_Composite_Expression_Local__(__Semantic_Body_Context__ *__Context__,
                                    __Ast_Expression__ *__Expression__)
{
    if (__Expression__ == NULL || __Expression__->__Kind__ != __Ast_Expression_Atom__ ||
        __Expression__->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
        __Expression__->__As__.__Atom__.__As__.__Lvalue__ == NULL ||
        __Expression__->__As__.__Atom__.__As__.__Lvalue__->__Kind__ != __Ast_Lvalue_Base__)
    {
        return NULL;
    }
    return __Body_Find_Local__(__Context__, __Expression__->__As__.__Atom__.__As__.__Lvalue__);
}

static __Ast_Expression__ *
__Body_Composite_Call_Provenance_Source__(__Semantic_Body_Context__ *__Context__,
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
    if (__Callee__ == NULL || __Callee__->__Composite_View_Return_Parameter__ == SIZE_MAX)
    {
        return NULL;
    }
    __Parameter_Index__ = __Callee__->__Composite_View_Return_Parameter__;
    return __Parameter_Index__ < __Expression__->__As__.__Call__.__Argument_Count__
               ? __Expression__->__As__.__Call__.__Arguments__[__Parameter_Index__]
               : NULL;
}

int __Body_Safety_Transfer_Composite_Expression__(__Semantic_Body_Context__ *__Context__,
                                                  __Ast_Expression__ *__Expression__,
                                                  __Semantic_Local__ *__Destination__,
                                                  int *__Matched__)
{
    __Ast_Expression__ *__Source_Expression__ = __Expression__;
    __Semantic_Local__ *__Source__;
    int __Move__;

    *__Matched__ = 0;
    if (__Destination__ == NULL || __Memory_Type_Is_View__(__Destination__->__Type__) ||
        !__Memory_Type_Contains_View__(__Context__->__Semantic__, __Destination__->__Type__))
    {
        return 1;
    }
    *__Matched__ = 1;

    if (__Expression__ == NULL)
    {
        return __Body_Fail__(
            __Context__, __E1100_Internal_Context_Error__, __Destination__->__Declaration_Span__);
    }

    if (__Expression__->__Kind__ == __Ast_Expression_Call__)
    {
        __Ast_Lvalue__ *__Function__ = __Expression__->__As__.__Call__.__Function__;
        if (__Function__ != NULL && __Function__->__Kind__ == __Ast_Lvalue_Field__)
        {
            size_t __Index__;

            if (__Expression__->__As__.__Call__.__Argument_Count__ == 0U)
            {
                return 1;
            }
            int __Found_Dependency__ = 0;
            for (__Index__ = 0U; __Index__ < __Expression__->__As__.__Call__.__Argument_Count__;
                 ++__Index__)
            {
                __Semantic_Local__ *__Argument_Source__ = __Body_Composite_Expression_Local__(
                    __Context__, __Expression__->__As__.__Call__.__Arguments__[__Index__]);
                if (!__Body_Local_Has_View_Dependency__(__Argument_Source__))
                {
                    continue;
                }
                if (__Found_Dependency__)
                {
                    return __Body_Fail__(__Context__,
                                         __E1303_Reference_Escape__,
                                         __Expression__->__Header__.__Span__);
                }
                __Move__ = __Argument_Source__->__Safety__.__State__ == __Safety_Value_Moved__;
                if (!__Body_Safety_Transfer_Local_Dependency__(__Context__,
                                                               __Argument_Source__,
                                                               __Destination__,
                                                               __Expression__->__Header__.__Span__,
                                                               __Move__))
                {
                    return 0;
                }
                __Found_Dependency__ = 1;
            }
            return __Found_Dependency__ || !__Memory_Type_Contains_View__(__Context__->__Semantic__,
                                                                          __Destination__->__Type__)
                       ? 1
                       : __Body_Fail__(__Context__,
                                       __E1303_Reference_Escape__,
                                       __Expression__->__Header__.__Span__);
        }
        __Source_Expression__ =
            __Body_Composite_Call_Provenance_Source__(__Context__, __Expression__);
        if (__Source_Expression__ == NULL)
        {
            return __Body_Fail__(
                __Context__, __E1303_Reference_Escape__, __Expression__->__Header__.__Span__);
        }
    }

    __Source__ = __Body_Composite_Expression_Local__(__Context__, __Source_Expression__);
    if (__Source__ == NULL || !__Body_Local_Has_View_Dependency__(__Source__))
    {
        return __Body_Fail__(
            __Context__, __E1303_Reference_Escape__, __Expression__->__Header__.__Span__);
    }
    __Move__ = __Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Source__->__Type__);
    if (__Move__ && __Source__->__Safety__.__State__ != __Safety_Value_Moved__ &&
        !__Body_Safety_Move_Expression__(__Context__, __Source_Expression__, NULL))
    {
        return 0;
    }
    return __Body_Safety_Transfer_Local_Dependency__(
        __Context__, __Source__, __Destination__, __Expression__->__Header__.__Span__, __Move__);
}

int __Body_Safety_Check_Return_Composite_View__(__Semantic_Body_Context__ *__Context__,
                                                __Ast_Expression__ *__Expression__)
{
    __Ast_Type__ *__Return_Type__ = __Context__->__Function__->__Function__->__Output__.__Type__;
    __Semantic_Local__ *__Local__;
    size_t __Origin__;

    if (__Memory_Type_Is_View__(__Return_Type__) ||
        !__Memory_Type_Contains_View__(__Context__->__Semantic__, __Return_Type__))
    {
        return 1;
    }
    __Local__ = __Body_Composite_Expression_Local__(__Context__, __Expression__);
    if (__Local__ == NULL || __Local__->__Safety__.__Lifetime_Region__ != 0U ||
        __Local__->__Safety__.__Origin_Parameter__ == SIZE_MAX)
    {
        return __Body_Report_Lifetime_Escape__(
            __Context__, __Local__, __Expression__->__Header__.__Span__);
    }

    __Origin__ = __Local__->__Safety__.__Origin_Parameter__;
    if (__Context__->__Function__->__Composite_View_Return_Parameter__ == SIZE_MAX)
    {
        __Context__->__Function__->__Composite_View_Return_Parameter__ = __Origin__;
    }
    else if (__Context__->__Function__->__Composite_View_Return_Parameter__ != __Origin__)
    {
        return __Body_Fail__(
            __Context__, __E1303_Reference_Escape__, __Expression__->__Header__.__Span__);
    }
    return 1;
}
