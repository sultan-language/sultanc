/* Declares internal type parsers. */

#ifndef SULTANC__FRONTEND_PARSER_TYPE_INTERNAL_H__
#define SULTANC__FRONTEND_PARSER_TYPE_INTERNAL_H__

#include "frontend/lexer/token.h"
#include "ast_factory.h"

/* Parses the parser wrapped type. */
__Ast_Type__ *__Parser_Parse_Wrapped_Type__(__Parser__ *__Parser_State__,
                                            __Ast_Type_Kind__ __Kind__);

/* Returns the parser type primitive. */
int __Parser_Type_Primitive__(__Token_Kind__ __Token_Value__,
                              __Ast_Type_Kind__ *__Out_Kind__,
                              __Machine_Type__ *__Out_Machine__);

/* Returns the parser type contextual. */
int __Parser_Type_Contextual__(__Text_Slice__ __Name__, __Ast_Type_Kind__ *__Out_Kind__);

/* Returns the parser exterior slot. */
__Ast_Slot__ __Parser_Exterior_Slot__(__Parser__ *__Parser_State__,
                                      __Ast_Type__ *__Type__,
                                      __Source_Span__ __Span__);

#endif
