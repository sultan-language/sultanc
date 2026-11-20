/* Defines the public bootstrap parser interface. */

#ifndef SULTANC__FRONTEND_PARSER_PARSER_H__
#define SULTANC__FRONTEND_PARSER_PARSER_H__

#include "core/diagnostic.h"
#include "core/source.h"
#include "frontend/ast/context.h"
#include "frontend/ast/module.h"
#include "target/bootstrap_target.h"

/* Defines the parse result structure. */
typedef struct
{
    /* Stores the AST. */
    __Ast_Context__ __Ast__;
    /* Stores the module. */
    __Ast_Module__ __Module__;
    /* Stores the diagnostic. */
    __Diagnostic__ __Diagnostic__;
    /* Tracks the failed state. */
    int __Failed__;
} __Parse_Result__;

/* Parses one module for the selected compilation target. Only the active arm of
 * each top-level target condition contributes imports and declarations. */
int __Parser_Parse_Module__(const __Source_File__ *__Source__,
                            const __Bootstrap_Target__ *__Target__,
                            __Parse_Result__ *__Out_Result__);

/* Parses the result destroy. */
void __Parse_Result_Destroy__(__Parse_Result__ *__Result__);

#endif
