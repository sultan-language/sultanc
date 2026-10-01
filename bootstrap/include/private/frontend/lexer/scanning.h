/* Declares token scanning entry points. */

#ifndef SULTANC__FRONTEND_LEXER_SCANNING_H__
#define SULTANC__FRONTEND_LEXER_SCANNING_H__

#include "frontend/lexer/token.h"
#include "lexer.h"

/* Returns the lexer. */
int __Lexer_Next__(__Lexer__ *__Lexer__, __Token__ *__Out_Token__);

#endif
