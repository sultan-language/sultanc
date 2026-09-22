#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/statement_internal.h"

#include <stdalign.h>

__Ast_Block__ *__Parser_Parse_Block__(__Parser__ *__Parser_State__)
{
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    __Vector__ __Statements__;
    __Ast_Block__ *__Block__ = NULL;

    __Vector_Init__(&__Statements__, sizeof(__Ast_Statement__ *));

    if (!__Parser_Expect__(__Parser_State__, __Token_LEFT_BRACE_OPERATOR__))
    {
        __Vector_Destroy__(&__Statements__);
        return NULL;
    }

    while (__Parser_State__->__Current__.__Kind__ != __Token_RIGHT_BRACE_OPERATOR__)
    {
        if (__Parser_State__->__Current__.__Kind__ == __Token_EOF__ ||
            !__Parser_Parse_One_Statement__(__Parser_State__, &__Statements__))
        {
            __Vector_Destroy__(&__Statements__);
            if (!__Parser_State__->__Failed__)
            {
                (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Unterminated_Block__);
            }
            return NULL;
        }
    }

    if (!__Parser_Expect__(__Parser_State__, __Token_RIGHT_BRACE_OPERATOR__))
    {
        __Vector_Destroy__(&__Statements__);
        return NULL;
    }

    __Block__ = __Parser_New_Block__(
        __Parser_State__,
        __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
    if (__Block__ == NULL)
    {
        __Vector_Destroy__(&__Statements__);
        return NULL;
    }

    __Block__->__Statement_Count__ = __Statements__.__Count__;
    __Block__->__Statements__ = (__Ast_Statement__ **)__Parser_Freeze_Vector__(
        __Parser_State__, &__Statements__, alignof(__Ast_Statement__ *));
    __Vector_Destroy__(&__Statements__);

    return __Block__;
}
