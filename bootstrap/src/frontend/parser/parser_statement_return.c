/* Parses return statements. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/statement_internal.h"

/* Parses the parser return. */
int __Parser_Parse_Return__(__Parser__ *__Parser_State__, __Vector__ *__Statements__)
{
    /* Stores the start. */
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    /* References the expression. */
    __Ast_Expression__ *__Expression__ = NULL;
    /* References the statement. */
    __Ast_Statement__ *__Statement__ = NULL;
    if (!__Parser_Advance__(__Parser_State__))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_SEMICOLON_OPERATOR__)
    {
        __Expression__ = __Parser_Parse_Expression__(__Parser_State__);
        if (__Expression__ == NULL)
        {
            return 0;
        }
    }
    if (!__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
    {
        return 0;
    }
    __Statement__ = __Parser_New_Statement__(
        __Parser_State__,
        __Ast_Statement_Return__,
        __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
    if (__Statement__ == NULL)
    {
        return 0;
    }
    __Statement__->__As__.__Return__ = __Expression__;
    return __Parser_Push_Statement__(__Parser_State__, __Statements__, __Statement__);
}
