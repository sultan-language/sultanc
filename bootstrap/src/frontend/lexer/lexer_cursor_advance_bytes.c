#include "frontend/lexer/cursor.h"
#include "support/text/utf8.h"

void __Lexer_Advance_Bytes__(__Lexer__ *__Lexer_State__, size_t __Count__)
{
    size_t __Index__ = 0U;
    for (__Index__ = 0U; __Index__ < __Count__ && __Lexer_Has__(__Lexer_State__, 1U); ++__Index__)
    {
        unsigned char __Byte__ = __Lexer_Peek__(__Lexer_State__, 0U);
        ++__Lexer_State__->__Offset__;
        if (__Byte__ == (unsigned char)'\n')
        {
            ++__Lexer_State__->__Line__;
            __Lexer_State__->__Column__ = 0U;
        }
        else if (!__Utf8_Is_Continuation_Byte__(__Byte__))
        {
            /* Count UTF-8 scalar starts, not continuation bytes. */
            ++__Lexer_State__->__Column__;
        }
    }
}
