#ifndef SULTANC__FRONTEND_PARSER_INTERNAL_H__
#define SULTANC__FRONTEND_PARSER_INTERNAL_H__

#include "frontend/lexer/token.h"
#include "parser.h"
#include "frontend/lexer/lexer.h"
#include "support/containers/vector.h"

#include <stddef.h>

typedef struct
{
    const __Source_File__ *__Source__;
    /* borrowed; outlives parse result */
    __Lexer__ __Lexer__;
    __Token__ __Current__;
    __Token__ __Next__;
    __Token__ __Previous__;
    __Ast_Context__ *__Ast__;
    /* borrowed from result */
    __Diagnostic__ *__Diagnostic__;
    /* borrowed from result */
    __Temporary_Id__ __Next_Temporary__;
    int __Failed__;
} __Parser__;

typedef enum
{
    __Initializer_Expression__,
    __Initializer_Vector__,
    __Initializer_Record__,
    __Initializer_Box__
} __Initializer_Kind__;

typedef struct
{
    __Initializer_Kind__ __Kind__;
    __Source_Span__ __Span__;
    union
    {
        __Ast_Expression__ *__Expression__;
        struct
        {
            __Ast_Atom__ *__Values__;
            size_t __Count__;
        } __Aggregate__;
        struct
        {
            __Ast_Record_Input__ *__Fields__;
            size_t __Count__;
        } __Record__;
        __Ast_Atom__ __Box__;
    } __As__;
} __Initializer__;

__Ast_Type__ *__Parser_Parse_Type__(__Parser__ *__Parser_State__);

__Ast_Type_Declaration__ *__Parser_Parse_Type_Declaration__(__Parser__ *__Parser_State__,
                                                            __Text_Slice__ __Name__,
                                                            __Source_Position__ __Start__);

__Ast_Expression__ *__Parser_Parse_Expression__(__Parser__ *__Parser_State__);

__Ast_Atom__ __Parser_Parse_Atom__(__Parser__ *__Parser_State__, int *__Ok__);

__Ast_Lvalue__ *__Parser_Parse_Lvalue__(__Parser__ *__Parser_State__);

__Ast_Block__ *__Parser_Parse_Block__(__Parser__ *__Parser_State__);

__Initializer__ __Parser_Parse_Initializer__(__Parser__ *__Parser_State__, int *__Ok__);

__Ast_Function__ *__Parser_Parse_Function__(__Parser__ *__Parser_State__,
                                            __Text_Slice__ __Name__,
                                            int __Public__,
                                            __Source_Position__ __Start__);

int __Parser_Parse_Module_Items__(__Parser__ *__Parser_State__, __Ast_Module__ *__Out_Module__);

#endif
