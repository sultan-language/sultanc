#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/statement_internal.h"

__Ast_Statement__ *__Parser_Parse_If_Core__(__Parser__ *__Parser_State__)
{
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    __Ast_Expression__ *__Condition__ = NULL;
    __Ast_Block__ *__Then__ = NULL;
    __Ast_Block__ *__Else__ = NULL;
    __Ast_Statement__ *__Statement__ = NULL;
    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
    {
        return NULL;
    }
    __Condition__ = __Parser_Parse_Expression__(__Parser_State__);
    if (__Condition__ == NULL ||
        !__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
    {
        return NULL;
    }
    __Then__ = __Parser_Parse_Block__(__Parser_State__);
    if (__Then__ == NULL)
    {
        return NULL;
    }
    if (__Parser_Accept__(__Parser_State__, __Token_ELSE__))
    {
        if (__Parser_State__->__Current__.__Kind__ == __Token_IF__)
        {
            __Ast_Statement__ *__Nested__ = __Parser_Parse_If_Core__(__Parser_State__);
            if (__Nested__ == NULL)
            {
                return NULL;
            }
            __Else__ = __Parser_Block_From_Statement__(__Parser_State__, __Nested__);
        }
        else
        {
            __Else__ = __Parser_Parse_Block__(__Parser_State__);
        }
        if (__Else__ == NULL)
        {
            return NULL;
        }
    }
    __Statement__ = __Parser_New_Statement__(
        __Parser_State__,
        __Ast_Statement_If__,
        __Parser_Span__(__Start__,
                        (__Else__ != NULL ? __Else__->__Header__.__Span__.__End__
                                          : __Then__->__Header__.__Span__.__End__)));
    if (__Statement__ == NULL)
    {
        return NULL;
    }
    __Statement__->__As__.__If__.__Condition__ = __Condition__;
    __Statement__->__As__.__If__.__Then__ = __Then__;
    __Statement__->__As__.__If__.__Else__ = __Else__;
    return __Statement__;
}
