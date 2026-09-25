/* Creates parser-owned AST nodes. */

#include "frontend/ast/storage.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/ast_factory.h"

#include <stdalign.h>
#include <string.h>

/* Creates the parser node. */
static void *
__Parser_New_Node__(__Parser__ *__Parser_State__, size_t __Size__, size_t __Alignment__)
{
    /* References the node. */
    void *__Node__ = __Ast_Allocate__(__Parser_State__->__Ast__, __Size__, __Alignment__);

    if (__Node__ == NULL)
    {
        (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
        return NULL;
    }
    memset(__Node__, 0, __Size__);
    return __Node__;
}

/* Creates the parser literal. */
__Ast_Literal__ *__Parser_New_Literal__(__Parser__ *__Parser_State__,
                                        __Ast_Literal_Kind__ __Kind__,
                                        __Source_Span__ __Span__)
{
    /* References the literal. */
    __Ast_Literal__ *__Literal__ = (__Ast_Literal__ *)__Parser_New_Node__(
        __Parser_State__, sizeof(*__Literal__), alignof(__Ast_Literal__));

    if (__Literal__ != NULL)
    {
        __Literal__->__Header__ = __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
        __Literal__->__Kind__ = __Kind__;
    }
    return __Literal__;
}

/* Creates the parser lvalue. */
__Ast_Lvalue__ *__Parser_New_Lvalue__(__Parser__ *__Parser_State__,
                                      __Ast_Lvalue_Kind__ __Kind__,
                                      __Source_Span__ __Span__)
{
    /* References the lvalue. */
    __Ast_Lvalue__ *__Lvalue__ = (__Ast_Lvalue__ *)__Parser_New_Node__(
        __Parser_State__, sizeof(*__Lvalue__), alignof(__Ast_Lvalue__));

    if (__Lvalue__ != NULL)
    {
        __Lvalue__->__Header__ = __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
        __Lvalue__->__Kind__ = __Kind__;
    }
    return __Lvalue__;
}

/* Creates the parser base lvalue. */
__Ast_Lvalue__ *__Parser_New_Base_Lvalue__(__Parser__ *__Parser_State__,
                                           __Ast_Lvalue_Base_Kind__ __Name_Kind__,
                                           __Text_Slice__ __Identifier__,
                                           __Temporary_Id__ __Temporary__,
                                           __Source_Span__ __Span__)
{
    /* References the lvalue. */
    __Ast_Lvalue__ *__Lvalue__ =
        __Parser_New_Lvalue__(__Parser_State__, __Ast_Lvalue_Base__, __Span__);

    if (__Lvalue__ == NULL)
    {
        return NULL;
    }
    __Lvalue__->__As__.__Base__.__Kind__ = __Name_Kind__;
    if (__Name_Kind__ == __Ast_Lvalue_Base_Identifier__)
    {
        __Lvalue__->__As__.__Base__.__As__.__Identifier__ = __Identifier__;
    }
    else
    {
        __Lvalue__->__As__.__Base__.__As__.__Temporary__ = __Temporary__;
    }
    return __Lvalue__;
}

/* Creates the parser expression. */
__Ast_Expression__ *__Parser_New_Expression__(__Parser__ *__Parser_State__,
                                              __Ast_Expression_Kind__ __Kind__,
                                              __Source_Span__ __Span__)
{
    /* References the expression. */
    __Ast_Expression__ *__Expression__ = (__Ast_Expression__ *)__Parser_New_Node__(
        __Parser_State__, sizeof(*__Expression__), alignof(__Ast_Expression__));

    if (__Expression__ != NULL)
    {
        __Expression__->__Header__ = __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
        __Expression__->__Kind__ = __Kind__;
    }
    return __Expression__;
}

/* Creates the parser statement. */
__Ast_Statement__ *__Parser_New_Statement__(__Parser__ *__Parser_State__,
                                            __Ast_Statement_Kind__ __Kind__,
                                            __Source_Span__ __Span__)
{
    /* References the statement. */
    __Ast_Statement__ *__Statement__ = (__Ast_Statement__ *)__Parser_New_Node__(
        __Parser_State__, sizeof(*__Statement__), alignof(__Ast_Statement__));

    if (__Statement__ != NULL)
    {
        __Statement__->__Header__ = __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
        __Statement__->__Kind__ = __Kind__;
    }
    return __Statement__;
}

/* Creates the parser block. */
__Ast_Block__ *__Parser_New_Block__(__Parser__ *__Parser_State__, __Source_Span__ __Span__)
{
    /* References the block. */
    __Ast_Block__ *__Block__ = (__Ast_Block__ *)__Parser_New_Node__(
        __Parser_State__, sizeof(*__Block__), alignof(__Ast_Block__));

    if (__Block__ != NULL)
    {
        __Block__->__Header__ = __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
    }
    return __Block__;
}

/* Creates the parser type. */
__Ast_Type__ *__Parser_New_Type__(__Parser__ *__Parser_State__, __Ast_Type_Kind__ __Kind__)
{
    /* References the type. */
    __Ast_Type__ *__Type__ = (__Ast_Type__ *)__Parser_New_Node__(
        __Parser_State__, sizeof(*__Type__), alignof(__Ast_Type__));

    if (__Type__ != NULL)
    {
        __Type__->__Kind__ = __Kind__;
    }
    return __Type__;
}

/* Creates the parser type declaration. */
__Ast_Type_Declaration__ *__Parser_New_Type_Declaration__(__Parser__ *__Parser_State__)
{
    return (__Ast_Type_Declaration__ *)__Parser_New_Node__(
        __Parser_State__, sizeof(__Ast_Type_Declaration__), alignof(__Ast_Type_Declaration__));
}

/* Creates the parser module item. */
__Ast_Module_Item__ *__Parser_New_Module_Item__(__Parser__ *__Parser_State__,
                                                __Ast_Module_Item_Kind__ __Kind__,
                                                __Text_Slice__ __Name__,
                                                __Source_Span__ __Span__)
{
    /* References the item. */
    __Ast_Module_Item__ *__Item__ = (__Ast_Module_Item__ *)__Parser_New_Node__(
        __Parser_State__, sizeof(*__Item__), alignof(__Ast_Module_Item__));

    if (__Item__ != NULL)
    {
        __Item__->__Header__ = __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
        __Item__->__Kind__ = __Kind__;
        __Item__->__Name__ = __Name__;
    }
    return __Item__;
}
