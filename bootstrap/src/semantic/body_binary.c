/* Infers and validates binary expressions. */

#include "semantic/body_internal.h"
#include "semantic/generic.h"

/* Infers the body binary. */
int __Body_Infer_Binary__(__Semantic_Body_Context__ *__Context__,
                          __Ast_Expression__ *__Expression__,
                          __Ast_Type__ **__Out_Type__)
{
    /* References the left. */
    __Ast_Type__ *__Left__ = NULL;
    /* References the right. */
    __Ast_Type__ *__Right__ = NULL;
    /* Stores the operation. */
    __Ast_Binary_Operation__ __Operation__ = __Expression__->__As__.__Binary__.__Operation__;
    /* Stores the comparison result. */
    int __Compare__ =
        __Operation__ == __Binary_Equal__ || __Operation__ == __Binary_Not_Equal__ ||
        __Operation__ == __Binary_Less_Than__ || __Operation__ == __Binary_Less_Or_Equal__ ||
        __Operation__ == __Binary_Greater_Or_Equal__ || __Operation__ == __Binary_Greater_Than__;

    /* Canonical null comparison: an unbound الفارغ takes the other operand's
     * concrete raw-pointer Type; two nulls have no Type. */
    if (__Operation__ == __Binary_Equal__ || __Operation__ == __Binary_Not_Equal__)
    {
        int __Left_Null__ =
            __Body_Is_Unbound_Null_Name__(__Context__, __Expression__->__As__.__Binary__.__Left__);
        int __Right_Null__ =
            __Body_Is_Unbound_Null_Name__(__Context__, __Expression__->__As__.__Binary__.__Right__);
        if (__Left_Null__ && __Right_Null__)
        {
            return __Body_Fail__(
                __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
        }
        if (__Left_Null__ || __Right_Null__)
        {
            __Ast_Expression__ *__Typed__ = __Left_Null__ ? __Expression__->__As__.__Binary__.__Right__
                                                          : __Expression__->__As__.__Binary__.__Left__;
            __Ast_Expression__ *__Null__ = __Left_Null__ ? __Expression__->__As__.__Binary__.__Left__
                                                         : __Expression__->__As__.__Binary__.__Right__;
            __Ast_Type__ *__Typed_Type__ = NULL;
            __Ast_Type__ *__Null_Type__ = NULL;
            if (!__Body_Infer_Expression__(__Context__, __Typed__, &__Typed_Type__))
            {
                return 0;
            }
            /* Contextual types outlive the body check (the emitter reads
             * them), so persist a program-lifetime copy of the inferred type. */
            __Typed_Type__ = __Semantic_Generic_Substitute_Type__(
                __Context__->__Semantic__, __Typed_Type__, NULL, NULL, 0U, NULL);
            if (__Typed_Type__ == NULL)
            {
                return __Body_Fail__(__Context__, __E1100_Internal_Context_Error__,
                                     __Expression__->__Header__.__Span__);
            }
            __Null__->__Contextual_Type__ = __Typed_Type__;
            if (!__Body_Infer_Expression__(__Context__, __Null__, &__Null_Type__))
            {
                return 0;
            }
            *__Out_Type__ = &__Body_Builtin_Bool_Type__;
            return 1;
        }
    }
    if (!__Body_Infer_Expression__(
            __Context__, __Expression__->__As__.__Binary__.__Left__, &__Left__) ||
        !__Body_Infer_Expression__(
            __Context__, __Expression__->__As__.__Binary__.__Right__, &__Right__))
    {
        return 0;
    }
    if (__Operation__ == __Binary_Logical_And__ || __Operation__ == __Binary_Logical_Or__)
    {
        if (!__Body_Is_Bool_Convertible__(__Context__, __Left__) ||
            !__Body_Is_Bool_Convertible__(__Context__, __Right__))
        {
            return __Body_Fail__(
                __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
        }
        *__Out_Type__ = &__Body_Builtin_Bool_Type__;
        return 1;
    }
    if (!__Type_Compatible__(__Context__->__Semantic__, __Left__, __Right__))
    {
        /* Stores the literal value. */
        int64_t __Literal_Value__ = 0;

        if (__Semantic_Integer_Literal_Expression_Value__(
                __Expression__->__As__.__Binary__.__Right__, &__Literal_Value__) &&
            __Type_Integer_Literal_Fits__(__Context__->__Semantic__, __Literal_Value__, __Left__))
        {
            __Right__ = __Left__;
        }
        else if (__Semantic_Integer_Literal_Expression_Is_Unsigned_Wide__(
                     __Expression__->__As__.__Binary__.__Right__) &&
                 __Type_Unsigned_Wide_Literal_Fits__(__Context__->__Semantic__, __Left__))
        {
            __Right__ = __Left__;
        }
        else if (__Semantic_Integer_Literal_Expression_Is_Unsigned_Wide__(
                     __Expression__->__As__.__Binary__.__Left__) &&
                 __Type_Unsigned_Wide_Literal_Fits__(__Context__->__Semantic__, __Right__))
        {
            __Left__ = __Right__;
        }
        else if (__Semantic_Integer_Literal_Expression_Value__(
                     __Expression__->__As__.__Binary__.__Left__, &__Literal_Value__) &&
                 __Type_Integer_Literal_Fits__(
                     __Context__->__Semantic__, __Literal_Value__, __Right__))
        {
            __Left__ = __Right__;
        }
        else
        {
            return __Body_Fail__(
                __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
        }
    }
    if (__Compare__)
    {
        *__Out_Type__ = &__Body_Builtin_Bool_Type__;
        return 1;
    }
    if (!__Body_Is_Integer_Like__(__Context__, __Left__, NULL))
    {
        return __Body_Fail__(__Context__,
                             __E0602_Unsupported_Binary_Expression__,
                             __Expression__->__Header__.__Span__);
    }
    *__Out_Type__ = __Left__;
    return 1;
}
