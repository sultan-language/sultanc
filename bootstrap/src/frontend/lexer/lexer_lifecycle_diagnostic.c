#include "frontend/lexer/lifecycle.h"
#include "frontend/lexer/lexer.h"

const __Diagnostic__ *__Lexer_Diagnostic__(const __Lexer__ *__Lexer_State__)
{
    return __Lexer_State__ == NULL || !__Lexer_State__->__Failed__
               ? NULL
               : &__Lexer_State__->__Diagnostic__;
}
