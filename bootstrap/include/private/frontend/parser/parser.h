#ifndef SULTANC__FRONTEND_PARSER_PARSER_H__
#define SULTANC__FRONTEND_PARSER_PARSER_H__

#include "core/diagnostic.h"
#include "core/source.h"
#include "frontend/ast/context.h"
#include "frontend/ast/module.h"

typedef struct
{
    __Ast_Context__ __Ast__;
    __Ast_Module__ __Module__;
    __Diagnostic__ __Diagnostic__;
    int __Failed__;
} __Parse_Result__;

int __Parser_Parse_Module__(const __Source_File__ *__Source__, __Parse_Result__ *__Out_Result__);

void __Parse_Result_Destroy__(__Parse_Result__ *__Result__);

#endif
