/* Checks hexadecimal digit characters. */

#include "frontend/lexer/character.h"

/* Checks whether the lexer is hex digit. */
int __Lexer_Is_Hex_Digit__(unsigned char __Byte__)
{
    return __Lexer_Is_Decimal_Digit__(__Byte__) ||
           (__Byte__ >= (unsigned char)'a' && __Byte__ <= (unsigned char)'f') ||
           (__Byte__ >= (unsigned char)'A' && __Byte__ <= (unsigned char)'F');
}
