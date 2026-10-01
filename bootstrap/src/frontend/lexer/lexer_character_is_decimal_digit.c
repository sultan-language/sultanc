/* Checks decimal digit characters. */

#include "frontend/lexer/character.h"

/* Checks whether the lexer is decimal digit. */
int __Lexer_Is_Decimal_Digit__(unsigned char __Byte__)
{
    return __Byte__ >= (unsigned char)'0' && __Byte__ <= (unsigned char)'9';
}
