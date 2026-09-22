#include "semantic/body_internal.h"
#include "kernel/type/tagged.h"
#include "support/text/equality.h"

/*
 * Checks an option/result constructor against an expected contextual type.
 * These constructors are intentionally context-typed so `option.none()` does
 * not need redundant source-level type arguments.
 */
int __Body_Try_Check_Builtin_Tagged_Construct__(__Semantic_Body_Context__ *__Context__,
                                                __Ast_Type__ *__Expected__,
                                                __Ast_Expression__ *__Expression__,
                                                int *__Matched__)
{
    __Ast_Lvalue__ *__Function__;
    __Ast_Lvalue__ *__Parent__;
    __Text_Slice__ __Type_Name__;
    __Text_Slice__ __Constructor_Name__;
    __Type_Tagged_Constructor__ __Constructor__;

    *__Matched__ = 0;
    if (!__Type_Is_Builtin_Tagged__(__Expected__) || __Expression__ == NULL ||
        __Expression__->__Kind__ != __Ast_Expression_Call__)
    {
        return 1;
    }

    __Function__ = __Expression__->__As__.__Call__.__Function__;
    if (__Function__ == NULL || __Function__->__Kind__ != __Ast_Lvalue_Field__)
    {
        return 1;
    }
    __Parent__ = __Function__->__As__.__Field__.__Parent__;
    if (__Parent__ == NULL || __Parent__->__Kind__ != __Ast_Lvalue_Base__ ||
        __Parent__->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__)
    {
        return 1;
    }

    __Type_Name__ = __Parent__->__As__.__Base__.__As__.__Identifier__;
    __Constructor_Name__ = __Function__->__As__.__Field__.__Field__;
    if (!__Type_Tagged_Name_Matches__(__Expected__, __Type_Name__) ||
        !__Type_Tagged_Find_Constructor__(__Expected__, __Constructor_Name__, &__Constructor__))
    {
        return 1;
    }

    *__Matched__ = 1;
    if (__Expression__->__As__.__Call__.__Argument_Count__ != __Constructor__.__Payload_Count__)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }

    if (__Constructor__.__Payload_Count__ == 1U)
    {
        __Ast_Expression__ *__Payload__ = __Expression__->__As__.__Call__.__Arguments__[0];
        if (!__Body_Check_Expression_Compatible__(__Context__,
                                                  __Constructor__.__Payload_Type__,
                                                  __Payload__,
                                                  __Payload__->__Header__.__Span__) ||
            (__Safety_Type_Is_Move_Only__(__Context__->__Semantic__,
                                          __Constructor__.__Payload_Type__) &&
             !__Body_Safety_Move_Expression__(__Context__, __Payload__, NULL)))
        {
            return 0;
        }
    }

    __Expression__->__Contextual_Type__ = __Type_Unwrap_Mutable__(__Expected__);
    return 1;
}
