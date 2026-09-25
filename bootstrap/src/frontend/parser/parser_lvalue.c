/* Parses assignable value paths. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/expression_internal.h"
#include "support/text/equality.h"

/* Parses the parser lvalue. */
__Ast_Lvalue__ *__Parser_Parse_Lvalue__(__Parser__ *__Parser_State__)
{
    /* References the lvalue. */
    __Ast_Lvalue__ *__Lvalue__ = NULL;
    /* Stores the start. */
    __Source_Position__ __Start__;
    if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
    {
        (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Identifier_Lvalue__);
        return NULL;
    }
    __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    __Lvalue__ = __Parser_New_Lvalue__(
        __Parser_State__, __Ast_Lvalue_Base__, __Parser_State__->__Current__.__Span__);
    if (__Lvalue__ == NULL)
    {
        return NULL;
    }
    __Lvalue__->__As__.__Base__.__Kind__ = __Ast_Lvalue_Base_Identifier__;
    __Lvalue__->__As__.__Base__.__As__.__Identifier__ =
        __Parser_State__->__Current__.__Kind__ == __Token_IDENTIFIER__
            ? __Parser_State__->__Current__.__Lexeme__
            : __Parser_State__->__Current__.__Lexeme__;
    if (!__Parser_Advance__(__Parser_State__))
    {
        return NULL;
    }

    for (;;)
    {
        if (__Parser_Accept__(__Parser_State__, __Token_DOT_OPERATOR__))
        {
            if (__Parser_Accept__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
            {
                /* Tracks whether the operation succeeded. */
                int __Ok__ = 0;
                /* Tracks the index. */
                __Ast_Atom__ __Index__ = __Parser_Parse_Atom__(__Parser_State__, &__Ok__);
                /* References the extended. */
                __Ast_Lvalue__ *__Extended__ = NULL;
                if (!__Ok__ ||
                    !__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
                {
                    return NULL;
                }
                __Extended__ = __Parser_New_Lvalue__(
                    __Parser_State__,
                    __Ast_Lvalue_Index__,
                    __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
                if (__Extended__ == NULL)
                {
                    return NULL;
                }
                __Extended__->__As__.__Index__.__Parent__ = __Lvalue__;
                __Extended__->__As__.__Index__.__Index__ = __Index__;
                __Lvalue__ = __Extended__;
                continue;
            }
            if (!__Parser_Token_Is_Contextual_Name__(&__Parser_State__->__Current__))
            {
                (void)__Parser_Fail__(__Parser_State__,
                                      __Diag_Word_Syntax_Expected_Field_After_Dot__);
                return NULL;
            }
            {
                /* Stores the field. */
                __Text_Slice__ __Field__;
                /* References the extended. */
                __Ast_Lvalue__ *__Extended__ = NULL;
                if (!__Parser_Take_Contextual_Name__(__Parser_State__, &__Field__))
                {
                    return NULL;
                }
                __Extended__ = __Parser_New_Lvalue__(
                    __Parser_State__,
                    __Ast_Lvalue_Field__,
                    __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
                if (__Extended__ == NULL)
                {
                    return NULL;
                }
                __Extended__->__As__.__Field__.__Parent__ = __Lvalue__;
                __Extended__->__As__.__Field__.__Field__ = __Field__;
                __Lvalue__ = __Extended__;
            }
            continue;
        }
        if (__Parser_Accept__(__Parser_State__, __Token_LEFT_BRACKET_OPERATOR__))
        {
            /* Tracks whether the operation succeeded. */
            int __Ok__ = 0;
            /* Tracks the index. */
            __Ast_Atom__ __Index__ = __Parser_Parse_Atom__(__Parser_State__, &__Ok__);
            /* References the extended. */
            __Ast_Lvalue__ *__Extended__ = NULL;
            if (!__Ok__ || !__Parser_Expect__(__Parser_State__, __Token_RIGHT_BRACKET_OPERATOR__))
            {
                return NULL;
            }
            __Extended__ = __Parser_New_Lvalue__(
                __Parser_State__,
                __Ast_Lvalue_Index__,
                __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
            if (__Extended__ == NULL)
            {
                return NULL;
            }
            __Extended__->__As__.__Index__.__Parent__ = __Lvalue__;
            __Extended__->__As__.__Index__.__Index__ = __Index__;
            __Lvalue__ = __Extended__;
            continue;
        }
        break;
    }
    return __Lvalue__;
}
