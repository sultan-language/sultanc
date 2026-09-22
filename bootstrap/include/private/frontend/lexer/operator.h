#ifndef SULTANC__FRONTEND_LEXER_OPERATOR_H__
#define SULTANC__FRONTEND_LEXER_OPERATOR_H__

#include "frontend/lexer/token.h"
#include "cursor.h"

int __Lexer_Scan_Operator__(__Lexer__ *__Lexer_State__,
                            __Token__ *__Out_Token__,
                            __Source_Position__ __Start__,
                            size_t __Start_Offset__);

#endif
