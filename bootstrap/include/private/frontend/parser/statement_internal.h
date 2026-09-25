/* Declares internal statement parsers. */

#ifndef SULTANC__FRONTEND_PARSER_STATEMENT_INTERNAL_H__
#define SULTANC__FRONTEND_PARSER_STATEMENT_INTERNAL_H__

#include "ast_factory.h"

/* Adds the parser statement. */
int __Parser_Push_Statement__(__Parser__ *__Parser_State__,
                              __Vector__ *__Statements__,
                              __Ast_Statement__ *__Statement__);

/* Parses the parser return. */
int __Parser_Parse_Return__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

/* Parses the parser variable. */
int __Parser_Parse_Variable__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

/* Maps the statement to the parser block. */
__Ast_Block__ *__Parser_Block_From_Statement__(__Parser__ *__Parser_State__,
                                               __Ast_Statement__ *__Statement__);

/* Parses the parser if core. */
__Ast_Statement__ *__Parser_Parse_If_Core__(__Parser__ *__Parser_State__);

/* Parses the parser match. */
int __Parser_Parse_Match__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

/* Parses the parser while. */
int __Parser_Parse_While__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

/* Returns the parser call after lvalue. */
__Ast_Expression__ *__Parser_Call_After_Lvalue__(__Parser__ *__Parser_State__,
                                                 __Ast_Lvalue__ *__Function__,
                                                 __Source_Position__ __Start__);

/* Parses the parser lvalue statement. */
int __Parser_Parse_Lvalue_Statement__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

/* Parses the parser one statement. */
int __Parser_Parse_One_Statement__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

#endif
