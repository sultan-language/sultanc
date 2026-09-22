#include "semantic/body_internal.h"

int __Body_Infer_Binary__(__Semantic_Body_Context__ *__Context__,
                          __Ast_Expression__ *__Expression__,
                          __Ast_Type__ **__Out_Type__)
{
    __Ast_Type__ *__Left__ = NULL;
    __Ast_Type__ *__Right__ = NULL;
    __Ast_Binary_Operation__ __Operation__ = __Expression__->__As__.__Binary__.__Operation__;
    int __Compare__ =
        __Operation__ == __Binary_Equal__ || __Operation__ == __Binary_Not_Equal__ ||
        __Operation__ == __Binary_Less_Than__ || __Operation__ == __Binary_Less_Or_Equal__ ||
        __Operation__ == __Binary_Greater_Or_Equal__ || __Operation__ == __Binary_Greater_Than__;

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
        int64_t __Literal_Value__ = 0;

        if (__Semantic_Integer_Literal_Expression_Value__(
                __Expression__->__As__.__Binary__.__Right__, &__Literal_Value__) &&
            __Type_Integer_Literal_Fits__(__Context__->__Semantic__, __Literal_Value__, __Left__))
        {
            __Right__ = __Left__;
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
