#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/initializer.h"

__Initializer__ __Parser_Parse_Initializer__(__Parser__ *__Parser_State__, int *__Ok__)
{
    __Initializer__ __Initializer_Value__;
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;

    *__Ok__ = 0;
    __Initializer_Value__.__Kind__ = __Initializer_Expression__;
    __Initializer_Value__.__Span__ = __Parser_State__->__Current__.__Span__;
    __Initializer_Value__.__As__.__Expression__ = NULL;

    if (__Parser_Accept__(__Parser_State__, __Token_LEFT_BRACKET_OPERATOR__))
    {
        __Initializer_Value__.__Kind__ = __Initializer_Vector__;
        if (!__Parser_Parse_Aggregate_Elements__(
                __Parser_State__,
                __Token_RIGHT_BRACKET_OPERATOR__,
                NULL,
                &__Initializer_Value__.__As__.__Aggregate__.__Values__,
                &__Initializer_Value__.__As__.__Aggregate__.__Count__))
        {
            return __Initializer_Value__;
        }
        __Initializer_Value__.__Span__ =
            __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__);
        *__Ok__ = 1;
        return __Initializer_Value__;
    }

    if (__Parser_Accept__(__Parser_State__, __Token_LEFT_BRACE_OPERATOR__))
    {
        if (!__Parser_Parse_Record_Initializer__(
                __Parser_State__, __Start__, &__Initializer_Value__))
        {
            return __Initializer_Value__;
        }
        *__Ok__ = 1;
        return __Initializer_Value__;
    }

    if (__Parser_Accept__(__Parser_State__, __Token_BOX_OPERATOR__))
    {
        int __Atom_Ok__ = 0;

        __Initializer_Value__.__Kind__ = __Initializer_Box__;
        __Initializer_Value__.__As__.__Box__ =
            __Parser_Parse_Atom__(__Parser_State__, &__Atom_Ok__);
        if (!__Atom_Ok__)
        {
            return __Initializer_Value__;
        }
        __Initializer_Value__.__Span__ =
            __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__);
        *__Ok__ = 1;
        return __Initializer_Value__;
    }

    if (__Parser_Accept__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
    {
        __Ast_Expression__ *__Expression__ = __Parser_Parse_Expression__(__Parser_State__);

        if (__Expression__ == NULL ||
            !__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
        {
            return __Initializer_Value__;
        }
        __Initializer_Value__.__As__.__Expression__ = __Expression__;
        __Initializer_Value__.__Span__ =
            __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__);
        *__Ok__ = 1;
        return __Initializer_Value__;
    }

    __Initializer_Value__.__As__.__Expression__ = __Parser_Parse_Expression__(__Parser_State__);
    if (__Initializer_Value__.__As__.__Expression__ == NULL)
    {
        return __Initializer_Value__;
    }
    __Initializer_Value__.__Kind__ = __Initializer_Expression__;
    __Initializer_Value__.__Span__ =
        __Initializer_Value__.__As__.__Expression__->__Header__.__Span__;
    *__Ok__ = 1;
    return __Initializer_Value__;
}
