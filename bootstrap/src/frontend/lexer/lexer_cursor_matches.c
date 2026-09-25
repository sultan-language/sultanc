/* Checks upcoming source bytes. */

#include "frontend/lexer/cursor.h"

#include <string.h>

/* Checks the lexer. */
int __Lexer_Matches__(const __Lexer__ *__Lexer_State__, const char *__Text__)
{
    /* Stores the length. */
    size_t __Length__ = strlen(__Text__);
    return __Lexer_Has__(__Lexer_State__, __Length__) &&
           memcmp(__Lexer_State__->__Source__->__Bytes__ + __Lexer_State__->__Offset__,
                  __Text__,
                  __Length__) == 0;
}
