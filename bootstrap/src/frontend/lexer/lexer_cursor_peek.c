/* Reads upcoming source bytes without advancing. */

#include "frontend/lexer/cursor.h"

/* Returns the lexer. */
unsigned char __Lexer_Peek__(const __Lexer__ *__Lexer_State__, size_t __Ahead__)
{
    if (!__Lexer_Has__(__Lexer_State__, __Ahead__ + 1U))
    {
        return 0U;
    }
    return (unsigned char)
        __Lexer_State__->__Source__->__Bytes__[__Lexer_State__->__Offset__ + __Ahead__];
}
