#include "semantic/body_internal.h"

int __Body_Try_Infer_Enum_Construct__(__Semantic_Body_Context__ *__Context__,
                                      __Ast_Expression__ *__Expression__,
                                      __Ast_Type__ **__Out_Type__,
                                      int *__Matched__)
{
    __Semantic_Type_Entry__ *__Type__ = NULL;
    __Ast_Enum_Constructor__ *__Constructor__ = NULL;
    size_t __Constructor_Index__ = 0U;
    size_t __Index__;

    *__Matched__ = 0;
    if (__Expression__ == NULL || __Expression__->__Kind__ != __Ast_Expression_Call__ ||
        !__Name_Resolve_Enum_Constructor_Lvalue__(__Context__->__Semantic__,
                                                  __Expression__->__As__.__Call__.__Function__,
                                                  &__Type__,
                                                  &__Constructor_Index__,
                                                  &__Constructor__))
    {
        return 1;
    }
    (void)__Constructor_Index__;
    *__Matched__ = 1;

    if (__Constructor__->__Payload_Count__ != __Expression__->__As__.__Call__.__Argument_Count__)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }

    /* A call statement is inferred once to establish its temporary result type
     * and again when that temporary assignment is checked. Constructor payload
     * ownership effects therefore belong to the constructor expression itself
     * and must be applied only once, just like ordinary/builtin call effects. */
    if (!__Expression__->__Semantic_Effects_Applied__)
    {
        for (__Index__ = 0U; __Index__ < __Constructor__->__Payload_Count__; ++__Index__)
        {
            __Ast_Type__ *__Payload_Type__ = __Constructor__->__Payload_Slots__[__Index__].__Type__;
            __Ast_Expression__ *__Payload__ =
                __Expression__->__As__.__Call__.__Arguments__[__Index__];
            if (!__Body_Check_Expression_Compatible__(
                    __Context__, __Payload_Type__, __Payload__, __Payload__->__Header__.__Span__) ||
                (__Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Payload_Type__) &&
                 !__Body_Safety_Move_Expression__(__Context__, __Payload__, NULL)))
            {
                return 0;
            }
        }
        __Expression__->__Semantic_Effects_Applied__ = 1;
    }

    *__Out_Type__ = __Body_Synthetic_Named_Type__(__Context__, __Type__->__Name__);
    if (*__Out_Type__ == NULL)
    {
        return __Body_Fail__(
            __Context__, __E1100_Internal_Context_Error__, __Expression__->__Header__.__Span__);
    }
    return 1;
}
