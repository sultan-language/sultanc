/* Parses module function items. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/module_internal.h"
#include "support/text/equality.h"

/* Parses the parser function item. */
int __Parser_Parse_Function_Item__(__Parser__ *__Parser_State__,
                                   __Vector__ *__Items__,
                                   int __Public__)
{
    /* Stores the start. */
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* References the function. */
    __Ast_Function__ *__Function__ = NULL;
    /* References the item. */
    __Ast_Module_Item__ *__Item__ = NULL;
    if (!__Parser_Expect__(__Parser_State__, __Token_FUNCTION_DEF__))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
    {
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Function_Name__);
    }
    __Name__ = __Parser_State__->__Current__.__Lexeme__;
    if (!__Parser_Advance__(__Parser_State__))
    {
        return 0;
    }
    __Function__ = __Parser_Parse_Function__(__Parser_State__, __Name__, __Public__, __Start__);
    if (__Function__ == NULL)
    {
        return 0;
    }
    __Item__ = __Parser_New_Module_Item__(__Parser_State__,
                                          __Ast_Module_Item_Function__,
                                          __Name__,
                                          __Function__->__Header__.__Span__);
    if (__Item__ == NULL)
    {
        return 0;
    }
    __Item__->__Public__ = __Public__;
    __Item__->__As__.__Function__ = __Function__;
    return __Parser_Push_Module_Item__(__Parser_State__, __Items__, __Item__);
}
