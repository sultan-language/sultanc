/* Declares internal module parsers. */

#ifndef SULTANC__FRONTEND_PARSER_MODULE_INTERNAL_H__
#define SULTANC__FRONTEND_PARSER_MODULE_INTERNAL_H__

#include "ast_factory.h"

/* Finds the parser module binding. */
const __Ast_Module_Item__ *__Parser_Module_Find_Binding__(const __Vector__ *__Items__,
                                                          __Text_Slice__ __Name__);

/* Returns the parser module name exists. */
int __Parser_Module_Name_Exists__(const __Vector__ *__Items__, __Text_Slice__ __Name__);

/* Adds the parser module item. */
int __Parser_Push_Module_Item__(__Parser__ *__Parser_State__,
                                __Vector__ *__Items__,
                                __Ast_Module_Item__ *__Item__);

/* Parses the parser import. */
int __Parser_Parse_Import__(__Parser__ *__Parser_State__, __Vector__ *__Imports__);

/* Parses the parser type item. */
int __Parser_Parse_Type_Item__(__Parser__ *__Parser_State__, __Vector__ *__Items__, int __Public__);

/* Parses the parser function item. */
int __Parser_Parse_Function_Item__(__Parser__ *__Parser_State__,
                                   __Vector__ *__Items__,
                                   int __Public__);

/* Parses the parser alias item. */
int __Parser_Parse_Alias_Item__(__Parser__ *__Parser_State__,
                                __Vector__ *__Items__,
                                __Source_Position__ __Start__,
                                int __Public__);

#endif
