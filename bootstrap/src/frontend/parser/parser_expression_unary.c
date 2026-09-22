#include "frontend/parser/cursor.h"
#include "frontend/parser/span.h"
#include "frontend/parser/type_internal.h"
#include "frontend/parser/expression_internal.h"

static __Ast_Expression__ *__Parser_Parse_Explicit_Conversions__(__Parser__ *__Parser_State__,
                                                                 __Ast_Expression__ *__Expression__)
{
    while (__Expression__ != NULL && __Parser_State__->__Current__.__Kind__ == __Token_AS__)
    {
        __Ast_Type__ *__Target_Type__;
        __Ast_Expression__ *__Conversion__;
        __Source_Position__ __Start__ = __Expression__->__Header__.__Span__.__Start__;

        if (!__Parser_Advance__(__Parser_State__))
        {
            return NULL;
        }
        __Target_Type__ = __Parser_Parse_Type__(__Parser_State__);
        if (__Target_Type__ == NULL)
        {
            return NULL;
        }
        __Conversion__ = __Parser_New_Expression__(
            __Parser_State__,
            __Ast_Expression_Conversion__,
            __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
        if (__Conversion__ == NULL)
        {
            return NULL;
        }
        __Conversion__->__As__.__Conversion__.__Operand__ = __Expression__;
        __Conversion__->__As__.__Conversion__.__Target_Type__ = __Target_Type__;
        __Expression__ = __Conversion__;
    }
    return __Expression__;
}

__Ast_Expression__ *__Parser_Parse_Unary__(__Parser__ *__Parser_State__)
{
    __Ast_Unary_Operation__ __Operation__ = __Unary_Not__;
    int __Is_Unary__ = 1;
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;

    switch (__Parser_State__->__Current__.__Kind__)
    {
        case __Token_NOT_OPERATOR__:
            __Operation__ = __Unary_Not__;
            break;
        case __Token_TILDE_OPERATOR__:
            __Operation__ = __Unary_Bitwise_Not__;
            break;
        case __Token_MINUS_OPERATOR__:
            __Operation__ = __Unary_Negate__;
            break;
        case __Token_AND_OPERATOR__:
            __Operation__ = __Unary_Address__;
            break;
        case __Token_STAR_OPERATOR__:
            __Operation__ = __Unary_Dereference__;
            break;
        default:
            __Is_Unary__ = 0;
            break;
    }

    if (!__Is_Unary__)
    {
        return __Parser_Parse_Explicit_Conversions__(__Parser_State__,
                                                     __Parser_Parse_Primary__(__Parser_State__));
    }
    if (!__Parser_Advance__(__Parser_State__))
    {
        return NULL;
    }
    if (__Operation__ == __Unary_Address__ &&
        __Parser_State__->__Current__.__Kind__ == __Token_MUTABLE__)
    {
        __Operation__ = __Unary_Address_Mutable__;
        if (!__Parser_Advance__(__Parser_State__))
        {
            return NULL;
        }
    }
    {
        __Ast_Expression__ *__Operand__ = __Parser_Parse_Unary__(__Parser_State__);
        __Ast_Expression__ *__Expression__ = NULL;
        if (__Operand__ == NULL)
        {
            return NULL;
        }
        __Expression__ = __Parser_New_Expression__(
            __Parser_State__,
            __Ast_Expression_Unary__,
            __Parser_Span__(__Start__, __Operand__->__Header__.__Span__.__End__));
        if (__Expression__ == NULL)
        {
            return NULL;
        }
        __Expression__->__As__.__Unary__.__Operation__ = __Operation__;
        __Expression__->__As__.__Unary__.__Operand__ = __Operand__;
        return __Parser_Parse_Explicit_Conversions__(__Parser_State__, __Expression__);
    }
}
