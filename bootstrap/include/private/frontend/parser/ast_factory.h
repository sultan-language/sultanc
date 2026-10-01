/* Declares parser AST factory helpers. */

#ifndef SULTANC__FRONTEND_PARSER_AST_FACTORY_H__
#define SULTANC__FRONTEND_PARSER_AST_FACTORY_H__

#include "internal.h"

/* Creates the parser literal. */
__Ast_Literal__ *__Parser_New_Literal__(__Parser__ *__Parser_State__,
                                        __Ast_Literal_Kind__ __Kind__,
                                        __Source_Span__ __Span__);

/* Creates the parser lvalue. */
__Ast_Lvalue__ *__Parser_New_Lvalue__(__Parser__ *__Parser_State__,
                                      __Ast_Lvalue_Kind__ __Kind__,
                                      __Source_Span__ __Span__);

/* Creates the parser base lvalue. */
__Ast_Lvalue__ *__Parser_New_Base_Lvalue__(__Parser__ *__Parser_State__,
                                           __Ast_Lvalue_Base_Kind__ __Name_Kind__,
                                           __Text_Slice__ __Identifier__,
                                           __Temporary_Id__ __Temporary__,
                                           __Source_Span__ __Span__);

/* Creates the parser expression. */
__Ast_Expression__ *__Parser_New_Expression__(__Parser__ *__Parser_State__,
                                              __Ast_Expression_Kind__ __Kind__,
                                              __Source_Span__ __Span__);

/* Creates the parser statement. */
__Ast_Statement__ *__Parser_New_Statement__(__Parser__ *__Parser_State__,
                                            __Ast_Statement_Kind__ __Kind__,
                                            __Source_Span__ __Span__);

/* Creates the parser block. */
__Ast_Block__ *__Parser_New_Block__(__Parser__ *__Parser_State__, __Source_Span__ __Span__);

/* Creates the parser type. */
__Ast_Type__ *__Parser_New_Type__(__Parser__ *__Parser_State__, __Ast_Type_Kind__ __Kind__);

/* Creates the parser type declaration. */
__Ast_Type_Declaration__ *__Parser_New_Type_Declaration__(__Parser__ *__Parser_State__);

/* Creates the parser module item. */
__Ast_Module_Item__ *__Parser_New_Module_Item__(__Parser__ *__Parser_State__,
                                                __Ast_Module_Item_Kind__ __Kind__,
                                                __Text_Slice__ __Name__,
                                                __Source_Span__ __Span__);

#endif
