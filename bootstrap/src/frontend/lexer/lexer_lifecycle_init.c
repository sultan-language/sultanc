#include "frontend/lexer/lifecycle.h"
#include "frontend/lexer/lexer.h"

#include <string.h>

void __Lexer_Init__(__Lexer__ *__Lexer_State__, const __Source_File__ *__Source__)
{
    if (__Lexer_State__ == NULL)
    {
        return;
    }
    memset(__Lexer_State__, 0, sizeof(*__Lexer_State__));
    __Lexer_State__->__Source__ = __Source__;
    __Lexer_State__->__Line__ = 1U;
    __Lexer_State__->__Column__ = 0U;
    __Arena_Init__(&__Lexer_State__->__Literal_Arena__, 4096U);
}
