/* Infers and validates unary expressions. */

#include "semantic/body_internal.h"

/* Infers the body unary. */
int __Body_Infer_Unary__(__Semantic_Body_Context__ *__Context__,
                         __Ast_Expression__ *__Expression__,
                         __Ast_Type__ **__Out_Type__)
{
    /* References the operand type. */
    __Ast_Type__ *__Operand_Type__ = NULL;
    /* Stores the operation. */
    __Ast_Unary_Operation__ __Operation__ = __Expression__->__As__.__Unary__.__Operation__;

    if (__Operation__ == __Unary_Address__ || __Operation__ == __Unary_Address_Mutable__)
    {
        /* References the operand. */
        __Ast_Expression__ *__Operand__ = __Expression__->__As__.__Unary__.__Operand__;
        /* References the lvalue type. */
        __Ast_Type__ *__Lvalue_Type__ = NULL;
        /* References the reference type. */
        __Ast_Type__ *__Reference_Type__ = NULL;

        if (__Operand__ == NULL || __Operand__->__Kind__ != __Ast_Expression_Atom__ ||
            __Operand__->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__)
        {
            return __Body_Fail__(
                __Context__, __E0613_Address_Of_Non_Lvalue__, __Expression__->__Header__.__Span__);
        }
        if (!__Body_Infer_Lvalue__(__Context__,
                                   __Operand__->__As__.__Atom__.__As__.__Lvalue__,
                                   &__Lvalue_Type__,
                                   NULL))
        {
            return 0;
        }
        __Reference_Type__ = __Body_Synthetic_Reference_Type__(
            __Context__, __Lvalue_Type__, __Operation__ == __Unary_Address_Mutable__);
        if (__Reference_Type__ == NULL)
        {
            return __Body_Fail__(
                __Context__, __E1100_Internal_Context_Error__, __Expression__->__Header__.__Span__);
        }
        *__Out_Type__ = __Reference_Type__;
        return 1;
    }

    if (!__Body_Infer_Expression__(
            __Context__, __Expression__->__As__.__Unary__.__Operand__, &__Operand_Type__))
    {
        return 0;
    }
    if (__Operation__ == __Unary_Dereference__)
    {
        /* Stores the resolved. */
        __Resolved_Type__ __Resolved__;

        if (!__Type_Resolve__(__Context__->__Semantic__, __Operand_Type__, &__Resolved__))
        {
            return 0;
        }
        if (__Resolved__.__Kind__ != __Resolved_Type_Reference__ &&
            __Resolved__.__Kind__ != __Resolved_Type_Box__)
        {
            return __Body_Fail__(__Context__,
                                 __E0612_Dereference_Non_Pointer__,
                                 __Expression__->__Header__.__Span__);
        }
        *__Out_Type__ = __Resolved__.__Inner__;
        return 1;
    }
    if (__Operation__ == __Unary_Not__)
    {
        if (!__Body_Is_Bool_Convertible__(__Context__, __Operand_Type__))
        {
            return __Body_Fail__(
                __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
        }
        *__Out_Type__ = &__Body_Builtin_Bool_Type__;
        return 1;
    }
    if (!__Body_Is_Integer_Like__(__Context__, __Operand_Type__, NULL))
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    *__Out_Type__ = __Operand_Type__;
    return 1;
}
