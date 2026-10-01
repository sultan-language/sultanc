/* Defines internal parser state. */

#ifndef SULTANC__FRONTEND_PARSER_INTERNAL_H__
#define SULTANC__FRONTEND_PARSER_INTERNAL_H__

#include "frontend/lexer/token.h"
#include "parser.h"
#include "frontend/lexer/lexer.h"
#include "support/containers/vector.h"

#include <stddef.h>

/* Defines the parser structure. */
typedef struct
{
    /* References the source. */
    const __Source_File__ *__Source__;
    /* Borrowed; outlives the parse result. */
    __Lexer__ __Lexer__;
    /* Stores the current. */
    __Token__ __Current__;
    /* Stores the next. */
    __Token__ __Next__;
    /* Stores the previous. */
    __Token__ __Previous__;
    /* References the AST. */
    __Ast_Context__ *__Ast__;
    /* Borrowed from the parse result. */
    __Diagnostic__ *__Diagnostic__;
    /* borrowed from result */
    __Temporary_Id__ __Next_Temporary__;
    /* Tracks the failed state. */
    int __Failed__;
} __Parser__;

/* Defines the initializer kind values. */
typedef enum
{
    /* Represents the initializer expression value. */
    __Initializer_Expression__,
    /* Represents the initializer vector value. */
    __Initializer_Vector__,
    /* Represents the initializer record value. */
    __Initializer_Record__,
    /* Represents the initializer box value. */
    __Initializer_Box__
} __Initializer_Kind__;

/* Defines the initializer structure. */
typedef struct
{
    /* Stores the kind. */
    __Initializer_Kind__ __Kind__;
    /* Stores the span. */
    __Source_Span__ __Span__;
    /* Stores the active variant payload. */
    union
    {
        /* References the expression. */
        __Ast_Expression__ *__Expression__;
        /* Stores the aggregate. */
        struct
        {
            /* References the values. */
            __Ast_Atom__ *__Values__;
            /* Stores the count. */
            size_t __Count__;
        } __Aggregate__;
        /* Stores the record. */
        struct
        {
            /* References the fields. */
            __Ast_Record_Input__ *__Fields__;
            /* Stores the count. */
            size_t __Count__;
        } __Record__;
        /* Stores the box. */
        __Ast_Atom__ __Box__;
    } __As__;
} __Initializer__;

/* Parses the parser type. */
__Ast_Type__ *__Parser_Parse_Type__(__Parser__ *__Parser_State__);

/* Parses the parser type declaration. */
__Ast_Type_Declaration__ *__Parser_Parse_Type_Declaration__(__Parser__ *__Parser_State__,
                                                            __Text_Slice__ __Name__,
                                                            __Source_Position__ __Start__);

/* Parses the parser expression. */
__Ast_Expression__ *__Parser_Parse_Expression__(__Parser__ *__Parser_State__);

/* Parses the parser atom. */
__Ast_Atom__ __Parser_Parse_Atom__(__Parser__ *__Parser_State__, int *__Ok__);

/* Parses the parser lvalue. */
__Ast_Lvalue__ *__Parser_Parse_Lvalue__(__Parser__ *__Parser_State__);

/* Parses the parser block. */
__Ast_Block__ *__Parser_Parse_Block__(__Parser__ *__Parser_State__);

/* Parses the parser initializer. */
__Initializer__ __Parser_Parse_Initializer__(__Parser__ *__Parser_State__, int *__Ok__);

/* Parses the parser function. */
__Ast_Function__ *__Parser_Parse_Function__(__Parser__ *__Parser_State__,
                                            __Text_Slice__ __Name__,
                                            int __Public__,
                                            __Source_Position__ __Start__);

/* Parses the parser module items. */
int __Parser_Parse_Module_Items__(__Parser__ *__Parser_State__, __Ast_Module__ *__Out_Module__);

#endif
