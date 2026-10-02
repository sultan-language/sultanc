/* Parses type syntax. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/type_internal.h"

#include <stdalign.h>

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

    if (__Parser_State__->__Current__.__Kind__ == __Token_FUNCTION_TYPE__)
    {
        __Vector__ __Parameters__;
        __Ast_Type__ *__Function_Type__ = NULL;
        __Ast_Type__ *__Output__ = NULL;
        __Vector_Init__(&__Parameters__, sizeof(__Ast_Type__ *));
        if (!__Parser_Advance__(__Parser_State__) ||
            !__Parser_Expect__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
        {
            __Vector_Destroy__(&__Parameters__);
            return NULL;
        }
        while (__Parser_State__->__Current__.__Kind__ != __Token_RIGHT_PARENTHESIS_OPERATOR__)
        {
            __Ast_Type__ *__Parameter__ = __Parser_Parse_Type__(__Parser_State__);
            if (__Parameter__ == NULL || __Vector_Push__(&__Parameters__, &__Parameter__) == NULL)
            {
                __Vector_Destroy__(&__Parameters__);
                if (!__Parser_State__->__Failed__)
                    (void)__Parser_Fail_Internal__(
                        __Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
                return NULL;
            }
            if (!__Parser_Accept__(__Parser_State__, __Token_COMMA_OPERATOR__))
                break;
        }
        if (!__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__) ||
            !__Parser_Expect__(__Parser_State__, __Token_COLON_OPERATOR__))
        {
            __Vector_Destroy__(&__Parameters__);
            return NULL;
        }
        __Output__ = __Parser_Parse_Type__(__Parser_State__);
        if (__Output__ == NULL)
        {
            __Vector_Destroy__(&__Parameters__);
            return NULL;
        }
        __Function_Type__ = __Parser_New_Type__(__Parser_State__, __Ast_Type_Function__);
        if (__Function_Type__ == NULL)
        {
            __Vector_Destroy__(&__Parameters__);
            return NULL;
        }
        __Function_Type__->__As__.__Function__.__Parameter_Count__ = __Parameters__.__Count__;
        __Function_Type__->__As__.__Function__.__Parameters__ =
            (__Ast_Type__ **)__Parser_Freeze_Vector__(
                __Parser_State__, &__Parameters__, alignof(__Ast_Type__ *));
        __Function_Type__->__As__.__Function__.__Output__ = __Output__;
        __Vector_Destroy__(&__Parameters__);
        return __Function_Type__;
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
        if (__Parser_Accept__(__Parser_State__, __Token_LESS_THAN_OPERATOR__))
        {
            __Vector__ __Arguments__;
            __Vector_Init__(&__Arguments__, sizeof(__Ast_Type__ *));
            if (__Parser_State__->__Current__.__Kind__ == __Token_GREATER_THAN_OPERATOR__)
            {
                __Vector_Destroy__(&__Arguments__);
                (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Type__);
                return NULL;
            }
            for (;;)
            {
                __Ast_Type__ *__Argument__ = __Parser_Parse_Type__(__Parser_State__);
                if (__Argument__ == NULL || __Vector_Push__(&__Arguments__, &__Argument__) == NULL)
                {
                    __Vector_Destroy__(&__Arguments__);
                    if (!__Parser_State__->__Failed__)
                        (void)__Parser_Fail_Internal__(
                            __Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
                    return NULL;
                }
                if (!__Parser_Accept__(__Parser_State__, __Token_COMMA_OPERATOR__))
                    break;
            }
            if (!__Parser_Expect__(__Parser_State__, __Token_GREATER_THAN_OPERATOR__))
            {
                __Vector_Destroy__(&__Arguments__);
                return NULL;
            }
            __Type__->__As__.__Named__.__Argument_Count__ = __Arguments__.__Count__;
            __Type__->__As__.__Named__.__Arguments__ =
                (__Ast_Type__ **)__Parser_Freeze_Vector__(
                    __Parser_State__, &__Arguments__, alignof(__Ast_Type__ *));
            __Vector_Destroy__(&__Arguments__);
        }
        return __Type__;
    }

    (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Type__);
    return NULL;
}
