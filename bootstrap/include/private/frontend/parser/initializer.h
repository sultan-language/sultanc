#ifndef SULTANC__FRONTEND_PARSER_INITIALIZER_H__
#define SULTANC__FRONTEND_PARSER_INITIALIZER_H__

#include "frontend/lexer/token.h"
#include "internal.h"

int __Parser_Parse_Aggregate_Elements__(__Parser__ *__Parser_State__,
                                        __Token_Kind__ __Closing_Token__,
                                        const __Ast_Atom__ *__First_Atom__,
                                        __Ast_Atom__ **__Out_Values__,
                                        size_t *__Out_Count__);

int __Parser_Parse_Record_Initializer__(__Parser__ *__Parser_State__,
                                        __Source_Position__ __Start__,
                                        __Initializer__ *__Out_Initializer__);

#endif
