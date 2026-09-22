#ifndef SULTANC__FRONTEND_PARSER_MODULE_INTERNAL_H__
#define SULTANC__FRONTEND_PARSER_MODULE_INTERNAL_H__

#include "ast_factory.h"

const __Ast_Module_Item__ *__Parser_Module_Find_Binding__(const __Vector__ *__Items__,
                                                          __Text_Slice__ __Name__);

int __Parser_Module_Name_Exists__(const __Vector__ *__Items__, __Text_Slice__ __Name__);

int __Parser_Push_Module_Item__(__Parser__ *__Parser_State__,
                                __Vector__ *__Items__,
                                __Ast_Module_Item__ *__Item__);

int __Parser_Parse_Import__(__Parser__ *__Parser_State__, __Vector__ *__Imports__);

int __Parser_Parse_Type_Item__(__Parser__ *__Parser_State__, __Vector__ *__Items__, int __Public__);

int __Parser_Parse_Function_Item__(__Parser__ *__Parser_State__,
                                   __Vector__ *__Items__,
                                   int __Public__);

int __Parser_Parse_Alias_Item__(__Parser__ *__Parser_State__,
                                __Vector__ *__Items__,
                                __Source_Position__ __Start__,
                                int __Public__);

#endif
