/* Parses type syntax. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/type_internal.h"

/* Parses the parser type. */
__Ast_Type__ *__Parser_Parse_Type__(__Parser__ *__Parser_State__)
{
    /* Stores the kind. */
    __Ast_Type_Kind__ __Kind__ = __Ast_Type_Any__;
    /* Stores the machine. */
    __Machine_Type__ __Machine__ = __Machine_I8__;
    /* References the type. */
    __Ast_Type__ *__Type__ = NULL;

    if (__Parser_State__->__Current__.__Kind__ == __Token_MUTABLE__)
    {
        return __Parser_Parse_Wrapped_Type__(__Parser_State__, __Ast_Type_Mutable__);
    }

    if (__Parser_State__->__Current__.__Kind__ == __Token_BOX_OPERATOR__)
    {
        return __Parser_Parse_Wrapped_Type__(__Parser_State__, __Ast_Type_Box__);
    }

    if (__Parser_State__->__Current__.__Kind__ == __Token_AND_OPERATOR__)
    {
        return __Parser_Parse_Wrapped_Type__(__Parser_State__, __Ast_Type_Reference__);
    }

    if (__Parser_Type_Primitive__(__Parser_State__->__Current__.__Kind__, &__Kind__, &__Machine__))
    {
        __Type__ = __Parser_New_Type__(__Parser_State__, __Kind__);
        if (__Type__ == NULL)
        {
            return NULL;
        }

        if (__Kind__ == __Ast_Type_Machine__)
        {
            __Type__->__As__.__Machine__ = __Machine__;
        }

        if (!__Parser_Advance__(__Parser_State__))
        {
            return NULL;
        }
        return __Type__;
    }

    if (__Parser_State__->__Current__.__Kind__ == __Token_IDENTIFIER__)
    {
        /* Stores the name. */
        __Text_Slice__ __Name__ = __Parser_State__->__Current__.__Lexeme__;
        /* Stores the contextual. */
        __Ast_Type_Kind__ __Contextual__ = __Ast_Type_Named__;

        if (__Parser_Type_Contextual__(__Name__, &__Contextual__))
        {
            if (__Contextual__ == __Ast_Type_Option__ || __Contextual__ == __Ast_Type_Vector__)
            {
                return __Parser_Parse_Wrapped_Type__(__Parser_State__, __Contextual__);
            }
            if (__Contextual__ == __Ast_Type_Result__)
            {
                /* Stores the operation result. */
                __Ast_Type__ *__Result__ = NULL;
                if (!__Parser_Advance__(__Parser_State__))
                    return NULL;
                __Result__ = __Parser_New_Type__(__Parser_State__, __Ast_Type_Result__);
                if (__Result__ == NULL)
                    return NULL;
                __Result__->__As__.__Result__.__Ok__ = __Parser_Parse_Type__(__Parser_State__);
                if (__Result__->__As__.__Result__.__Ok__ == NULL ||
                    !__Parser_Expect__(__Parser_State__, __Token_COMMA_OPERATOR__))
                    return NULL;
                __Result__->__As__.__Result__.__Error__ = __Parser_Parse_Type__(__Parser_State__);
                if (__Result__->__As__.__Result__.__Error__ == NULL)
                    return NULL;
                return __Result__;
            }
        }

        if (!__Parser_Advance__(__Parser_State__))
            return NULL;
        __Type__ = __Parser_New_Type__(__Parser_State__, __Ast_Type_Named__);
        if (__Type__ == NULL)
            return NULL;
        __Type__->__As__.__Named__.__Name__ = __Name__;
        return __Type__;
    }

    (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Type__);
    return NULL;
}
