#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/module_internal.h"
#include "support/text/equality.h"

int __Parser_Parse_Type_Item__(__Parser__ *__Parser_State__, __Vector__ *__Items__, int __Public__)
{
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    __Text_Slice__ __Name__;
    __Ast_Type_Declaration__ *__Declaration__ = NULL;
    __Ast_Module_Item__ *__Item__ = NULL;
    if (!__Parser_Advance__(__Parser_State__))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
    {
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Type_Name__);
    }
    __Name__ = __Parser_State__->__Current__.__Lexeme__;
    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_DEFINITION_SEPARATOR__))
    {
        return 0;
    }
    __Declaration__ = __Parser_Parse_Type_Declaration__(__Parser_State__, __Name__, __Start__);
    if (__Declaration__ == NULL ||
        !__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
    {
        return 0;
    }
    __Item__ = __Parser_New_Module_Item__(
        __Parser_State__,
        __Ast_Module_Item_Type__,
        __Name__,
        __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
    if (__Item__ == NULL)
    {
        return 0;
    }
    __Item__->__Public__ = __Public__;
    __Item__->__As__.__Type__ = __Declaration__;
    return __Parser_Push_Module_Item__(__Parser_State__, __Items__, __Item__);
}
