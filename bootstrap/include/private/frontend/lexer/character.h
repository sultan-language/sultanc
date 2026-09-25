/* Declares lexer character classifiers. */

#ifndef SULTANC__FRONTEND_LEXER_CHARACTER_H__
#define SULTANC__FRONTEND_LEXER_CHARACTER_H__

#include <stdint.h>

/* Checks whether the lexer is ID start. */
int __Lexer_Is_Id_Start__(uint32_t __Scalar__);

/* Checks whether the lexer is ID rest. */
int __Lexer_Is_Id_Rest__(uint32_t __Scalar__);

/* Checks whether the lexer is decimal digit. */
int __Lexer_Is_Decimal_Digit__(unsigned char __Byte__);

/* Checks whether the lexer is hex digit. */
int __Lexer_Is_Hex_Digit__(unsigned char __Byte__);

/* Returns the lexer hex value. */
int __Lexer_Hex_Value__(unsigned char __Byte__);

#endif
