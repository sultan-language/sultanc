#include "frontend/parser/cursor.h"
#include "frontend/parser/token_payload.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/module_internal.h"
#include "support/text/equality.h"

int __Parser_Parse_Import__(__Parser__ *__Parser_State__, __Vector__ *__Imports__)
{
    __Text_Slice__ __Import__;
    if (!__Parser_Advance__(__Parser_State__))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ == __Token_LIT_STR__)
    {
        __Text_Slice__ __Literal_Text__;
        if (!__Parser_Current_Text__(__Parser_State__, &__Literal_Text__))
        {
            return 0;
        }
        __Import__ = __Parser_Copy_Literal_Text__(__Parser_State__, __Literal_Text__);
    }
    else if (__Parser_State__->__Current__.__Kind__ == __Token_IDENTIFIER__)
    {
        __Import__ = __Parser_State__->__Current__.__Lexeme__;
    }
    else
    {
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Source_Or_Module__);
    }
    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
    {
        return 0;
    }
    if (__Vector_Push__(__Imports__, &__Import__) == NULL)
    {
        return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
    }
    return 1;
}
