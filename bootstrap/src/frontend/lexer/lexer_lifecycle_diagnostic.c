/* Returns the current lexer diagnostic. */

#include "frontend/lexer/lifecycle.h"
#include "frontend/lexer/lexer.h"

/* Returns the lexer diagnostic. */
const __Diagnostic__ *__Lexer_Diagnostic__(const __Lexer__ *__Lexer_State__)
{
    return __Lexer_State__ == NULL || !__Lexer_State__->__Failed__
               ? NULL
               : &__Lexer_State__->__Diagnostic__;
}
