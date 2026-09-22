#include "frontend/lexer/cursor.h"

__Source_Position__ __Lexer_Position__(const __Lexer__ *__Lexer_State__)
{
    __Source_Position__ __Position__;
    __Position__.__Source__ = __Lexer_State__->__Source__;
    __Position__.__Offset__ = __Lexer_State__->__Offset__;
    __Position__.__Line__ = __Lexer_State__->__Line__;
    __Position__.__Column__ = __Lexer_State__->__Column__;
    return __Position__;
}
