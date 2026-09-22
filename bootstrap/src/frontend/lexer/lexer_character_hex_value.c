#include "frontend/lexer/character.h"

int __Lexer_Hex_Value__(unsigned char __Byte__)
{
    if (__Byte__ >= (unsigned char)'0' && __Byte__ <= (unsigned char)'9')
    {
        return (int)(__Byte__ - (unsigned char)'0');
    }
    if (__Byte__ >= (unsigned char)'a' && __Byte__ <= (unsigned char)'f')
    {
        return 10 + (int)(__Byte__ - (unsigned char)'a');
    }
    if (__Byte__ >= (unsigned char)'A' && __Byte__ <= (unsigned char)'F')
    {
        return 10 + (int)(__Byte__ - (unsigned char)'A');
    }
    return -1;
}
