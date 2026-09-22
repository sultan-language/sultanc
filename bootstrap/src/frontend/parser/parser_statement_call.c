#include "frontend/ast/storage.h"
#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/statement_internal.h"

#include <stdalign.h>

__Ast_Expression__ *__Parser_Call_After_Lvalue__(__Parser__ *__Parser_State__,
                                                 __Ast_Lvalue__ *__Function__,
                                                 __Source_Position__ __Start__)
{
    __Vector__ __Arguments__;
    __Ast_Expression__ *__Call__ = NULL;
    __Vector_Init__(&__Arguments__, sizeof(__Ast_Expression__ *));
    if (!__Parser_Expect__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
    {
        __Vector_Destroy__(&__Arguments__);
        return NULL;
    }
    while (__Parser_State__->__Current__.__Kind__ != __Token_RIGHT_PARENTHESIS_OPERATOR__)
    {
        __Ast_Expression__ *__Argument__ = __Parser_Parse_Expression__(__Parser_State__);
        if (__Argument__ == NULL || __Vector_Push__(&__Arguments__, &__Argument__) == NULL)
        {
            __Vector_Destroy__(&__Arguments__);
            if (!__Parser_State__->__Failed__)
            {
                (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
            }
            return NULL;
        }
        if (!__Parser_Accept__(__Parser_State__, __Token_COMMA_OPERATOR__))
        {
            break;
        }
    }
    if (!__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
    {
        __Vector_Destroy__(&__Arguments__);
        return NULL;
    }
    __Call__ = (__Ast_Expression__ *)__Ast_Allocate__(
        __Parser_State__->__Ast__, sizeof(*__Call__), alignof(__Ast_Expression__));
    if (__Call__ == NULL)
    {
        __Vector_Destroy__(&__Arguments__);
        (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
        return NULL;
    }
    __Call__->__Header__ = __Ast_New_Header__(
        __Parser_State__->__Ast__,
        __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
    __Call__->__Kind__ = __Ast_Expression_Call__;
    __Call__->__As__.__Call__.__Function__ = __Function__;
    __Call__->__As__.__Call__.__Argument_Count__ = __Arguments__.__Count__;
    __Call__->__As__.__Call__.__Arguments__ = (__Ast_Expression__ **)__Parser_Freeze_Vector__(
        __Parser_State__, &__Arguments__, alignof(__Ast_Expression__ *));
    __Vector_Destroy__(&__Arguments__);
    return __Call__;
}
