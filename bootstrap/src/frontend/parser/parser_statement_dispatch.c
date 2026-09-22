#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/statement_internal.h"

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
        case __Token_IF__:
        {
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
            return __Parser_Parse_Lvalue_Statement__(__Parser_State__, __Statements__);
        default:
            return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Statement__);
    }
}
