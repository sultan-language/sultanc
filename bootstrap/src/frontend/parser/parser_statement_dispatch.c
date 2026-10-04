/* Dispatches statement parsing. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/statement_internal.h"
#include "frontend/parser/span.h"

/* Parses the parser one statement. */
int __Parser_Parse_One_Statement__(__Parser__ *__Parser_State__, __Vector__ *__Statements__)
{
    switch (__Parser_State__->__Current__.__Kind__)
    {
        case __Token_RETURN__:
            return __Parser_Parse_Return__(__Parser_State__, __Statements__);
        case __Token_LET__:
            return __Parser_Parse_Variable__(__Parser_State__, __Statements__);
        case __Token_WHILE__:
            return __Parser_Parse_While__(__Parser_State__, __Statements__);
        case __Token_MATCH__:
            return __Parser_Parse_Match__(__Parser_State__, __Statements__);
        case __Token_UNSAFE__:
        {
            __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
            __Ast_Block__ *__Body__ = NULL;
            __Ast_Statement__ *__Statement__ = NULL;
            if (!__Parser_Advance__(__Parser_State__))
                return 0;
            __Body__ = __Parser_Parse_Block__(__Parser_State__);
            if (__Body__ == NULL)
                return 0;
            __Statement__ = __Parser_New_Statement__(
                __Parser_State__, __Ast_Statement_Unsafe__,
                __Parser_Span__(__Start__, __Body__->__Header__.__Span__.__End__));
            if (__Statement__ == NULL)
                return 0;
            __Statement__->__As__.__Unsafe__ = __Body__;
            (void)__Parser_Accept__(__Parser_State__, __Token_SEMICOLON_OPERATOR__);
            return __Parser_Push_Statement__(__Parser_State__, __Statements__, __Statement__);
        }
        case __Token_IF__:
        {
            /* References the conditional statement. */
            __Ast_Statement__ *__If__ = __Parser_Parse_If_Core__(__Parser_State__);
            if (__If__ == NULL)
            {
                return 0;
            }
            /* v27 sources use both `if (...) {...};` and block-style `if (...) {...}`. */
            (void)__Parser_Accept__(__Parser_State__, __Token_SEMICOLON_OPERATOR__);
            return __Parser_Push_Statement__(__Parser_State__, __Statements__, __If__);
        }
        case __Token_IDENTIFIER__:
        case __Token_STAR_OPERATOR__:
        case __Token_LEFT_PARENTHESIS_OPERATOR__:
            return __Parser_Parse_Lvalue_Statement__(__Parser_State__, __Statements__);
        default:
            return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Statement__);
    }
}
