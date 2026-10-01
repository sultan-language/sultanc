/* Checks remaining lexer input length. */

#include "frontend/lexer/cursor.h"

/* Checks whether the lexer has the requested value. */
int __Lexer_Has__(const __Lexer__ *__Lexer_State__, size_t __Count__)
{
    return __Lexer_State__->__Offset__ + __Count__ <= __Lexer_State__->__Source__->__Length__;
}
