#ifndef SULTANC__FRONTEND_LEXER_LITERAL_H__
#define SULTANC__FRONTEND_LEXER_LITERAL_H__

#include "frontend/lexer/token.h"
#include "lexer.h"

int __Lexer_Scan_String__(__Lexer__ *__Lexer_State__,
                          __Token__ *__Out_Token__,
                          __Source_Position__ __Start__,
                          size_t __Start_Offset__,
                          int __Guillemet__);

int __Lexer_Scan_Character__(__Lexer__ *__Lexer_State__,
                             __Token__ *__Out_Token__,
                             __Source_Position__ __Start__,
                             size_t __Start_Offset__);

int __Lexer_Scan_Number__(__Lexer__ *__Lexer_State__,
                          __Token__ *__Out_Token__,
                          __Source_Position__ __Start__,
                          size_t __Start_Offset__);

#endif
