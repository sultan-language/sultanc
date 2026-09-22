#ifndef SULTANC__FRONTEND_PARSER_CURSOR_H__
#define SULTANC__FRONTEND_PARSER_CURSOR_H__

#include "frontend/lexer/token.h"
#include "internal.h"

int __Parser_Advance__(__Parser__ *__Parser_State__);

int __Parser_Accept__(__Parser__ *__Parser_State__, __Token_Kind__ __Kind__);

/* Contextual-name slots preserve the token's exact source lexeme. */
int __Parser_Token_Is_Contextual_Name__(const __Token__ *__Token_Value__);

int __Parser_Take_Contextual_Name__(__Parser__ *__Parser_State__, __Text_Slice__ *__Out_Name__);

#endif
