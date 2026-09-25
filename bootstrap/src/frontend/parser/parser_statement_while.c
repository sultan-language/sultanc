/* Parses while statements. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/statement_internal.h"

/* Parses the parser while. */
int __Parser_Parse_While__(__Parser__ *__Parser_State__, __Vector__ *__Statements__)
{
    /* Stores the start. */
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    /* References the condition. */
    __Ast_Expression__ *__Condition__ = NULL;
    /* References the body. */
    __Ast_Block__ *__Body__ = NULL;
    /* References the statement. */
    __Ast_Statement__ *__Statement__ = NULL;
    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
    {
        return 0;
    }
    __Condition__ = __Parser_Parse_Expression__(__Parser_State__);
    if (__Condition__ == NULL ||
        !__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
    {
        return 0;
    }
    __Body__ = __Parser_Parse_Block__(__Parser_State__);
    if (__Body__ == NULL)
    {
        return 0;
    }
    __Statement__ =
        __Parser_New_Statement__(__Parser_State__,
                                 __Ast_Statement_While__,
                                 __Parser_Span__(__Start__, __Body__->__Header__.__Span__.__End__));
    if (__Statement__ == NULL)
    {
        return 0;
    }
    __Statement__->__As__.__While__.__Condition__ = __Condition__;
    __Statement__->__As__.__While__.__Body__ = __Body__;
    return __Parser_Push_Statement__(__Parser_State__, __Statements__, __Statement__);
}
