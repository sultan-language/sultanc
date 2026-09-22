#include "semantic/body_internal.h"
#include "kernel/type/conversion.h"

int __Body_Infer_Conversion__(__Semantic_Body_Context__ *__Context__,
                              __Ast_Expression__ *__Expression__,
                              __Ast_Type__ **__Out_Type__)
{
    __Ast_Type__ *__Source_Type__ = NULL;
    __Ast_Type__ *__Target_Type__;
    __Type_Conversion_Class__ __Class__;

    if (__Context__ == NULL || __Expression__ == NULL || __Out_Type__ == NULL ||
        __Expression__->__Kind__ != __Ast_Expression_Conversion__)
    {
        return 0;
    }
    __Target_Type__ = __Expression__->__As__.__Conversion__.__Target_Type__;
    if (__Target_Type__ == NULL ||
        !__Body_Infer_Expression__(
            __Context__, __Expression__->__As__.__Conversion__.__Operand__, &__Source_Type__))
    {
        return 0;
    }
    __Class__ =
        __Type_Conversion_Classify__(__Context__->__Semantic__, __Source_Type__, __Target_Type__);

    if (__Class__ == __Type_Conversion_Invalid__)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }

    *__Out_Type__ = __Target_Type__;
    return 1;
}
