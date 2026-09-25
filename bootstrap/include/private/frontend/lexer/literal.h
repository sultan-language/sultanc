/* Declares literal scanners. */

#ifndef SULTANC__FRONTEND_LEXER_LITERAL_H__
#define SULTANC__FRONTEND_LEXER_LITERAL_H__

#include "frontend/lexer/token.h"
#include "lexer.h"

/* Scans the lexer string. */
int __Lexer_Scan_String__(__Lexer__ *__Lexer_State__,
                          __Token__ *__Out_Token__,
                          __Source_Position__ __Start__,
                          size_t __Start_Offset__,
                          int __Guillemet__);

/* Scans the lexer character. */
int __Lexer_Scan_Character__(__Lexer__ *__Lexer_State__,
                             __Token__ *__Out_Token__,
                             __Source_Position__ __Start__,
                             size_t __Start_Offset__);

/* Scans the lexer number. */
int __Lexer_Scan_Number__(__Lexer__ *__Lexer_State__,
                          __Token__ *__Out_Token__,
                          __Source_Position__ __Start__,
                          size_t __Start_Offset__);

#endif
