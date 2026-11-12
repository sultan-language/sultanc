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
/* Parses `لكل (دع …; condition; place := value) { … }` (canonical حلل_لكل). */
int __Parser_Parse_For__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

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

/* Defines the propagation statement form values (`EXPR؟`, docs/en/reference/errors.md). */
typedef enum
{
    /* Represents the assign form (`let x: T = EXPR؟;`, `x := EXPR؟;`). */
    __Parser_Propagation_Assign__,
    /* Represents the return form (`return EXPR؟;`). */
    __Parser_Propagation_Return__,
    /* Represents the expression statement form (`EXPR؟;`). */
    __Parser_Propagation_Ignore__
} __Parser_Propagation_Form__;

/* Pushes the expansion of one propagation statement. */
int __Parser_Push_Propagation__(__Parser__ *__Parser_State__,
                                __Vector__ *__Statements__,
                                __Ast_Expression__ *__Value__,
                                __Parser_Propagation_Form__ __Form__,
                                __Ast_Lvalue__ *__Destination__,
                                __Source_Span__ __Mark__,
                                __Source_Span__ __Span__);

/* Parses the parser one statement. */
int __Parser_Parse_One_Statement__(__Parser__ *__Parser_State__, __Vector__ *__Statements__);

#endif
