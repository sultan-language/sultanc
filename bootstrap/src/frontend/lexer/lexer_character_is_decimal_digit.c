#include "frontend/lexer/character.h"

int __Lexer_Is_Decimal_Digit__(unsigned char __Byte__)
{
    return __Byte__ >= (unsigned char)'0' && __Byte__ <= (unsigned char)'9';
}
