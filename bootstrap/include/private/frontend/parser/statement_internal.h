#ifndef SULTANC__FRONTEND_PARSER_STATEMENT_INTERNAL_H__
#define SULTANC__FRONTEND_PARSER_STATEMENT_INTERNAL_H__

#include "ast_factory.h"

int __Parser_Push_Statement__(__Parser__ *__Parser_State__,
                              __Vector__ *__Statements__,
                              __Ast_Statement__ *__Statement__);

int __Parser_Parse_Return__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

int __Parser_Parse_Variable__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

__Ast_Block__ *__Parser_Block_From_Statement__(__Parser__ *__Parser_State__,
                                               __Ast_Statement__ *__Statement__);

__Ast_Statement__ *__Parser_Parse_If_Core__(__Parser__ *__Parser_State__);

int __Parser_Parse_Match__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

int __Parser_Parse_While__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

__Ast_Expression__ *__Parser_Call_After_Lvalue__(__Parser__ *__Parser_State__,
                                                 __Ast_Lvalue__ *__Function__,
                                                 __Source_Position__ __Start__);

int __Parser_Parse_Lvalue_Statement__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

int __Parser_Parse_One_Statement__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

#endif
