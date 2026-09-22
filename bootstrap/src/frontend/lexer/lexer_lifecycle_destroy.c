#include "frontend/lexer/lifecycle.h"
#include "frontend/lexer/lexer.h"

void __Lexer_Destroy__(__Lexer__ *__Lexer_State__)
{
    if (__Lexer_State__ == NULL)
    {
        return;
    }
    __Arena_Destroy__(&__Lexer_State__->__Literal_Arena__);
    __Lexer_State__->__Source__ = NULL;
}
