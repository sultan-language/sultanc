/* Checks ordinary function calls. */

#include "semantic/body_internal.h"
#include "kernel/name/name.h"
#include "kernel/type/tagged.h"

/* Checks the body call arguments. */
static int __Body_Check_Call_Arguments__(__Semantic_Body_Context__ *__Context__,
                                         __Ast_Type__ *const *__Parameters__,
                                         size_t __Parameter_Count__,
                                         __Ast_Expression__ *const *__Arguments__,
                                         size_t __Argument_Count__,
                                         int __Apply_Safety_Effects__)
{
    /* Tracks the index. */
    size_t __Index__;

    if (__Parameter_Count__ != __Argument_Count__)
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, (__Source_Span__){0});
    }

    for (__Index__ = 0U; __Index__ < __Argument_Count__; ++__Index__)
    {
        if (!__Body_Check_Expression_Compatible__(__Context__,
                                                  __Parameters__[__Index__],
                                                  __Arguments__[__Index__],
                                                  __Arguments__[__Index__]->__Header__.__Span__))
        {
            return 0;
        }
        if (__Apply_Safety_Effects__)
        {
            if (!__Body_Safety_Check_Ephemeral_Borrow__(__Context__, __Arguments__[__Index__]))
            {
                return 0;
            }
            if (__Safety_Type_Is_Move_Only__(__Context__->__Semantic__,
                                             __Parameters__[__Index__]) &&
                !__Safety_Type_Is_Reference__(__Parameters__[__Index__], NULL) &&
                !__Body_Safety_Move_Expression__(__Context__, __Arguments__[__Index__], NULL))
            {
                return 0;
            }
        }
    }
    return 1;
}

/* Infers the body call. */
int __Body_Infer_Call__(__Semantic_Body_Context__ *__Context__,
                        __Ast_Expression__ *__Expression__,
                        __Ast_Type__ **__Out_Type__)
{
    /* References the function lvalue. */
    __Ast_Lvalue__ *__Function_Lvalue__ = __Expression__->__As__.__Call__.__Function__;
    /* References the callee. */
    __Semantic_Function_Entry__ *__Callee__ = NULL;
    /* References the local. */
    __Semantic_Local__ *__Local__ = NULL;

    {
        /* Tracks the builtin matched state. */
        int __Builtin_Matched__ = 0;
        if (!__Body_Try_Infer_Builtin_Call__(
                __Context__, __Expression__, __Out_Type__, &__Builtin_Matched__))
        {
            return 0;
        }
        if (__Builtin_Matched__)
        {
            return 1;
        }
    }

    if (__Expression__->__Contextual_Type__ != NULL &&
        __Type_Is_Builtin_Tagged__(__Expression__->__Contextual_Type__))
    {
        *__Out_Type__ = __Expression__->__Contextual_Type__;
        return 1;
    }

    if (__Function_Lvalue__ == NULL)
    {
        return __Body_Fail__(
            __Context__, __E0300_Unknown_Name__, __Expression__->__Header__.__Span__);
    }

    {
        /* Tracks the matched state. */
        int __Matched__ = 0;
        if (!__Body_Try_Infer_Enum_Construct__(
                __Context__, __Expression__, __Out_Type__, &__Matched__))
        {
            return 0;
        }
        if (__Matched__)
        {
            return 1;
        }
    }

    if (__Function_Lvalue__->__Kind__ == __Ast_Lvalue_Base__ &&
        __Function_Lvalue__->__As__.__Base__.__Kind__ == __Ast_Lvalue_Base_Identifier__)
    {
        __Local__ = __Body_Find_Local__(__Context__, __Function_Lvalue__);
        if (__Local__ == NULL)
        {
            /* Stores the lookup. */
            __Name_Lookup_Status__ __Lookup__ = __Name_Resolve_Function__(
                __Context__->__Semantic__,
                __Context__->__Function__->__Unit__,
                __Function_Lvalue__->__As__.__Base__.__As__.__Identifier__,
                &__Callee__);
            if (__Lookup__ == __Name_Lookup_Private__ || __Lookup__ == __Name_Lookup_Ambiguous__)
            {
                return __Body_Fail__(__Context__,
                                     __Name_Lookup_Error_Id__(__Lookup__),
                                     __Expression__->__Header__.__Span__);
            }
        }
    }

    if (__Callee__ != NULL)
    {
        /* References the parameters. */
        __Ast_Type__ **__Parameters__ = NULL;

        /* Tracks the index. */
        size_t __Index__;
        /* Tracks whether the operation succeeded. */
        int __Ok__;

        if (__Callee__->__Function__->__Parameter_Count__ != 0U)
        {
            __Parameters__ = (__Ast_Type__ **)__Arena_Allocate__(
                &__Context__->__Synthetic_Types__,
                __Callee__->__Function__->__Parameter_Count__ * sizeof(*__Parameters__),
                _Alignof(__Ast_Type__ *));
            if (__Parameters__ == NULL)
            {
                return __Body_Fail__(__Context__,
                                     __E1100_Internal_Context_Error__,
                                     __Expression__->__Header__.__Span__);
            }
            for (__Index__ = 0U; __Index__ < __Callee__->__Function__->__Parameter_Count__;
                 ++__Index__)
            {
                __Parameters__[__Index__] =
                    __Callee__->__Function__->__Parameters__[__Index__].__Slot__.__Type__;
            }
        }
        __Ok__ = __Body_Check_Call_Arguments__(__Context__,
                                               __Parameters__,
                                               __Callee__->__Function__->__Parameter_Count__,
                                               __Expression__->__As__.__Call__.__Arguments__,
                                               __Expression__->__As__.__Call__.__Argument_Count__,
                                               !__Expression__->__Semantic_Effects_Applied__);
        if (!__Ok__)
        {
            return 0;
        }
        *__Out_Type__ = __Callee__->__Function__->__Output__.__Type__;
        __Expression__->__Semantic_Effects_Applied__ = 1;
        return 1;
    }

    return __Body_Fail__(
        __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
}
