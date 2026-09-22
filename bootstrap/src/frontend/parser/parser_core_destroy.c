#include "frontend/lexer/lifecycle.h"
#include "frontend/parser/lifecycle.h"
#include "frontend/parser/internal.h"

void __Parser_Destroy__(__Parser__ *__Parser_State__)
{
    if (__Parser_State__ == NULL)
    {
        return;
    }
    __Lexer_Destroy__(&__Parser_State__->__Lexer__);
    __Parser_State__->__Source__ = NULL;
    __Parser_State__->__Ast__ = NULL;
    __Parser_State__->__Diagnostic__ = NULL;
}
