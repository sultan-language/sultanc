#ifndef SULTANC__FRONTEND_LEXER_CHARACTER_H__
#define SULTANC__FRONTEND_LEXER_CHARACTER_H__

#include <stdint.h>

int __Lexer_Is_Id_Start__(uint32_t __Scalar__);

int __Lexer_Is_Id_Rest__(uint32_t __Scalar__);

int __Lexer_Is_Decimal_Digit__(unsigned char __Byte__);

int __Lexer_Is_Hex_Digit__(unsigned char __Byte__);

int __Lexer_Hex_Value__(unsigned char __Byte__);

#endif
