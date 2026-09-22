#ifndef SULTANC__FRONTEND_PARSER_EXPRESSION_INTERNAL_H__
#define SULTANC__FRONTEND_PARSER_EXPRESSION_INTERNAL_H__

#include "frontend/lexer/token.h"
#include "ast_factory.h"

int __Parser_Binary_Info__(__Token_Kind__ __Kind__,
                           int *__Out_Precedence__,
                           __Ast_Binary_Operation__ *__Out_Operation__);

__Ast_Expression__ *__Parser_Parse_Primary__(__Parser__ *__Parser_State__);

__Ast_Expression__ *__Parser_Parse_Unary__(__Parser__ *__Parser_State__);

__Ast_Expression__ *__Parser_Parse_Expression_Min__(__Parser__ *__Parser_State__,
                                                    int __Minimum_Precedence__);

#endif
