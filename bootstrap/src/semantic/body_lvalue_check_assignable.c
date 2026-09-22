#include "semantic/body_internal.h"

int __Body_Check_Assignable__(__Semantic_Body_Context__ *__Context__,
                              __Ast_Lvalue__ *__Lvalue__,
                              __Ast_Type__ **__Out_Type__,
                              __Semantic_Local__ **__Out_Local__)
{
    __Semantic_Local__ *__Local__ = NULL;
    if (!__Body_Infer_Lvalue_For_Write__(__Context__, __Lvalue__, __Out_Type__, &__Local__))
    {
        return 0;
    }
    if (__Lvalue__->__Kind__ == __Ast_Lvalue_Base__)
    {
        if (__Local__ == NULL)
        {
            return 0;
        }
        if (!__Local__->__Mutable__ &&
            __Local__->__Safety__.__State__ == __Safety_Value_Initialized__)
        {
            return __Body_Fail__(
                __Context__, __E0703_Write_To_Immutable__, __Lvalue__->__Header__.__Span__);
        }
        if (!__Safety_Fact_Can_Write__(&__Local__->__Safety__))
        {
            return __Body_Fail__(
                __Context__, __E1302_Borrow_Conflict__, __Lvalue__->__Header__.__Span__);
        }
    }
    if (__Out_Local__ != NULL)
    {
        *__Out_Local__ = __Local__;
    }
    return 1;
}
