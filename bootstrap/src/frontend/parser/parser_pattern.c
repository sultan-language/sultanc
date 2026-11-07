/* Parses match patterns. */

#include "frontend/ast/storage.h"
#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/pattern.h"
#include "frontend/parser/span.h"
#include "frontend/parser/storage.h"
#include "support/text/equality.h"
#include "frontend/parser/ast_factory.h"
#include "frontend/parser/internal.h"

#include <string.h>

#include <stdalign.h>

/* Creates the parser pattern. */
static __Ast_Pattern__ *__Parser_New_Pattern__(__Parser__ *__Parser_State__,
                                               __Ast_Pattern_Kind__ __Kind__,
                                               __Source_Position__ __Start__)
{
    /* References the pattern. */
    __Ast_Pattern__ *__Pattern__ = (__Ast_Pattern__ *)__Ast_Allocate__(
        __Parser_State__->__Ast__, sizeof(*__Pattern__), alignof(__Ast_Pattern__));
    if (__Pattern__ == NULL)
    {
        (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
        return NULL;
    }

    __Pattern__->__Kind__ = __Kind__;
    __Pattern__->__Header__ = __Ast_New_Header__(
        __Parser_State__->__Ast__,
        __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
    return __Pattern__;
}

/* Parses the parser enum pattern. */
static __Ast_Pattern__ *__Parser_Parse_Enum_Pattern__(__Parser__ *__Parser_State__,
                                                      __Text_Slice__ __Type_Name__,
                                                      __Source_Position__ __Start__)
{
    /* Stores the constructor name. */
    __Text_Slice__ __Constructor_Name__;
    /* Stores the payloads. */
    __Vector__ __Payloads__;
    /* References the pattern. */
    __Ast_Pattern__ *__Pattern__ = NULL;

    if (!__Parser_Expect__(__Parser_State__, __Token_DOT_OPERATOR__))
    {
        return NULL;
    }
    if (!__Parser_Token_Is_Contextual_Name__(&__Parser_State__->__Current__))
    {
        return NULL;
    }

    if (!__Parser_Take_Contextual_Name__(__Parser_State__, &__Constructor_Name__))
    {
        return NULL;
    }

    __Vector_Init__(&__Payloads__, sizeof(__Ast_Pattern__ *));
    if (__Parser_Accept__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
    {
        if (__Parser_State__->__Current__.__Kind__ != __Token_RIGHT_PARENTHESIS_OPERATOR__)
        {
            for (;;)
            {
                /* References the payload. */
                __Ast_Pattern__ *__Payload__ = __Parser_Parse_Pattern__(__Parser_State__);
                if (__Payload__ == NULL || __Vector_Push__(&__Payloads__, &__Payload__) == NULL)
                {
                    __Vector_Destroy__(&__Payloads__);
                    return NULL;
                }
                if (!__Parser_Accept__(__Parser_State__, __Token_COMMA_OPERATOR__))
                {
                    break;
                }
            }
        }
        if (!__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
        {
            __Vector_Destroy__(&__Payloads__);
            return NULL;
        }
    }

    __Pattern__ = __Parser_New_Pattern__(__Parser_State__, __Ast_Pattern_Enum__, __Start__);
    if (__Pattern__ == NULL)
    {
        __Vector_Destroy__(&__Payloads__);
        return NULL;
    }
    __Pattern__->__As__.__Enum__.__Type_Name__ = __Type_Name__;
    __Pattern__->__As__.__Enum__.__Constructor_Name__ = __Constructor_Name__;
    __Pattern__->__As__.__Enum__.__Payload_Count__ = __Payloads__.__Count__;
    __Pattern__->__As__.__Enum__.__Payloads__ = (__Ast_Pattern__ **)__Parser_Freeze_Vector__(
        __Parser_State__, &__Payloads__, alignof(__Ast_Pattern__ *));
    __Vector_Destroy__(&__Payloads__);
    return __Pattern__;
}

/* Parses the parser struct pattern. */
static __Ast_Pattern__ *__Parser_Parse_Struct_Pattern__(__Parser__ *__Parser_State__,
                                                        __Source_Position__ __Start__)
{
    /* Stores the fields. */
    __Vector__ __Fields__;
    /* References the pattern. */
    __Ast_Pattern__ *__Pattern__ = NULL;

    __Vector_Init__(&__Fields__, sizeof(__Ast_Struct_Pattern_Field__));
    if (!__Parser_Advance__(__Parser_State__))
    {
        __Vector_Destroy__(&__Fields__);
        return NULL;
    }

    while (__Parser_State__->__Current__.__Kind__ != __Token_RIGHT_BRACE_OPERATOR__)
    {
        /* Stores the field. */
        __Ast_Struct_Pattern_Field__ __Field__;
        if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
        {
            __Vector_Destroy__(&__Fields__);
            (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Pattern_Field__);
            return NULL;
        }
        __Field__.__Name__ = __Parser_State__->__Current__.__Lexeme__;
        if (!__Parser_Advance__(__Parser_State__) ||
            !__Parser_Expect__(__Parser_State__, __Token_COLON_OPERATOR__))
        {
            __Vector_Destroy__(&__Fields__);
            return NULL;
        }
        __Field__.__Pattern__ = __Parser_Parse_Pattern__(__Parser_State__);
        if (__Field__.__Pattern__ == NULL || __Vector_Push__(&__Fields__, &__Field__) == NULL)
        {
            __Vector_Destroy__(&__Fields__);
            return NULL;
        }
        if (!__Parser_Accept__(__Parser_State__, __Token_COMMA_OPERATOR__))
        {
            break;
        }
    }

    if (!__Parser_Expect__(__Parser_State__, __Token_RIGHT_BRACE_OPERATOR__))
    {
        __Vector_Destroy__(&__Fields__);
        return NULL;
    }

    __Pattern__ = __Parser_New_Pattern__(__Parser_State__, __Ast_Pattern_Struct__, __Start__);
    if (__Pattern__ == NULL)
    {
        __Vector_Destroy__(&__Fields__);
        return NULL;
    }
    __Pattern__->__As__.__Struct__.__Field_Count__ = __Fields__.__Count__;
    __Pattern__->__As__.__Struct__.__Fields__ =
        (__Ast_Struct_Pattern_Field__ *)__Parser_Freeze_Vector__(
            __Parser_State__, &__Fields__, alignof(__Ast_Struct_Pattern_Field__));
    __Vector_Destroy__(&__Fields__);
    return __Pattern__;
}

/* Parses the parser pattern. */
/* Parses a literal pattern value: an integer (possibly negated), a
 * character, a text or a boolean literal. */
static __Ast_Expression__ *__Parser_Parse_Literal_Pattern_Value__(__Parser__ *__Parser_State__)
{
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    int __Negative__ = __Parser_Accept__(__Parser_State__, __Token_MINUS_OPERATOR__);
    int __Ok__ = 0;
    __Ast_Atom__ __Atom__ = __Parser_Parse_Atom__(__Parser_State__, &__Ok__);
    __Ast_Expression__ *__Value__;
    if (!__Ok__ || __Atom__.__Kind__ != __Ast_Atom_Literal__)
    {
        if (!__Parser_State__->__Failed__)
            (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Pattern__);
        return NULL;
    }
    __Value__ = __Parser_New_Expression__(
        __Parser_State__, __Ast_Expression_Atom__,
        __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
    if (__Value__ == NULL)
        return NULL;
    __Value__->__As__.__Atom__ = __Atom__;
    if (__Negative__)
    {
        __Ast_Expression__ *__Negated__ = __Parser_New_Expression__(
            __Parser_State__, __Ast_Expression_Unary__,
            __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
        if (__Negated__ == NULL)
            return NULL;
        __Negated__->__As__.__Unary__.__Operation__ = __Unary_Negate__;
        __Negated__->__As__.__Unary__.__Operand__ = __Value__;
        __Value__ = __Negated__;
    }
    return __Value__;
}

/* Compares a lexeme with a C string. */
static int __Pattern_Word_Equals__(__Text_Slice__ __Text__, const char *__Word__)
{
    size_t __Length__ = strlen(__Word__);
    return __Text__.__Length__ == __Length__ && memcmp(__Text__.__Data__, __Word__, __Length__) == 0;
}

/* Answers whether the current token spells NAME_A or NAME_B as an identifier. */
static int __Parser_Current_Word__(__Parser__ *__Parser_State__, const char *__Name_A__, const char *__Name_B__)
{
    return __Parser_State__->__Current__.__Kind__ == __Token_IDENTIFIER__ &&
           (__Pattern_Word_Equals__(__Parser_State__->__Current__.__Lexeme__, __Name_A__) ||
            __Pattern_Word_Equals__(__Parser_State__->__Current__.__Lexeme__, __Name_B__));
}

static __Ast_Pattern__ *__Parser_Parse_Single_Pattern__(__Parser__ *__Parser_State__);

/* Parses a pattern with its alternatives (`p | q | …`). */
__Ast_Pattern__ *__Parser_Parse_Pattern__(__Parser__ *__Parser_State__)
{
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    __Ast_Pattern__ *__First__ = __Parser_Parse_Single_Pattern__(__Parser_State__);
    __Vector__ __Items__;
    __Ast_Pattern__ *__Pattern__;
    if (__First__ == NULL || __Parser_State__->__Current__.__Kind__ != __Token_OR_OPERATOR__)
    {
        return __First__;
    }
    __Vector_Init__(&__Items__, sizeof(__Ast_Pattern__ *));
    if (__Vector_Push__(&__Items__, &__First__) == NULL)
    {
        __Vector_Destroy__(&__Items__);
        (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
        return NULL;
    }
    while (__Parser_Accept__(__Parser_State__, __Token_OR_OPERATOR__))
    {
        __Ast_Pattern__ *__Next__ = __Parser_Parse_Single_Pattern__(__Parser_State__);
        if (__Next__ == NULL || __Vector_Push__(&__Items__, &__Next__) == NULL)
        {
            __Vector_Destroy__(&__Items__);
            return NULL;
        }
    }
    __Pattern__ = __Parser_New_Pattern__(__Parser_State__, __Ast_Pattern_Alternatives__, __Start__);
    if (__Pattern__ == NULL)
    {
        __Vector_Destroy__(&__Items__);
        return NULL;
    }
    __Pattern__->__As__.__Alternatives__.__Count__ = __Items__.__Count__;
    __Pattern__->__As__.__Alternatives__.__Items__ = (__Ast_Pattern__ **)__Parser_Freeze_Vector__(
        __Parser_State__, &__Items__, alignof(__Ast_Pattern__ *));
    __Vector_Destroy__(&__Items__);
    return __Pattern__;
}

/* Parses one pattern. */
static __Ast_Pattern__ *__Parser_Parse_Single_Pattern__(__Parser__ *__Parser_State__)
{
    /* Stores the start. */
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    /* References the pattern. */
    __Ast_Pattern__ *__Pattern__ = NULL;

    if (__Parser_State__->__Current__.__Kind__ == __Token_UNDERSCORE_OPERATOR__)
    {
        if (!__Parser_Advance__(__Parser_State__))
        {
            return NULL;
        }
        return __Parser_New_Pattern__(__Parser_State__, __Ast_Pattern_Wildcard__, __Start__);
    }

    if (__Parser_State__->__Current__.__Kind__ == __Token_LEFT_BRACE_OPERATOR__)
    {
        return __Parser_Parse_Struct_Pattern__(__Parser_State__, __Start__);
    }

    if (__Parser_State__->__Current__.__Kind__ == __Token_LIT_INT__ ||
        __Parser_State__->__Current__.__Kind__ == __Token_LIT_CHAR__ ||
        __Parser_State__->__Current__.__Kind__ == __Token_LIT_STR__ ||
        __Parser_State__->__Current__.__Kind__ == __Token_LIT_BOOL__ ||
        __Parser_State__->__Current__.__Kind__ == __Token_MINUS_OPERATOR__)
    {
        __Ast_Expression__ *__Value__ = __Parser_Parse_Literal_Pattern_Value__(__Parser_State__);
        if (__Value__ == NULL)
        {
            return NULL;
        }
        __Pattern__ = __Parser_New_Pattern__(__Parser_State__, __Ast_Pattern_Literal__, __Start__);
        if (__Pattern__ != NULL)
        {
            __Pattern__->__As__.__Literal__ = __Value__;
        }
        return __Pattern__;
    }

    if (__Parser_Current_Word__(__Parser_State__, "مدى", "range") &&
        (__Parser_State__->__Next__.__Kind__ == __Token_LIT_INT__ ||
         __Parser_State__->__Next__.__Kind__ == __Token_LIT_CHAR__ ||
         __Parser_State__->__Next__.__Kind__ == __Token_MINUS_OPERATOR__))
    {
        __Ast_Expression__ *__Low__;
        __Ast_Expression__ *__High__;
        if (!__Parser_Advance__(__Parser_State__) ||
            (__Low__ = __Parser_Parse_Literal_Pattern_Value__(__Parser_State__)) == NULL)
        {
            return NULL;
        }
        if (!__Parser_Current_Word__(__Parser_State__, "إلى", "to"))
        {
            (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Pattern__);
            return NULL;
        }
        if (!__Parser_Advance__(__Parser_State__) ||
            (__High__ = __Parser_Parse_Literal_Pattern_Value__(__Parser_State__)) == NULL)
        {
            return NULL;
        }
        __Pattern__ = __Parser_New_Pattern__(__Parser_State__, __Ast_Pattern_Range__, __Start__);
        if (__Pattern__ != NULL)
        {
            __Pattern__->__As__.__Range__.__Low__ = __Low__;
            __Pattern__->__As__.__Range__.__High__ = __High__;
        }
        return __Pattern__;
    }

    if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
    {
        (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Pattern__);
        return NULL;
    }

    {
        /* Stores the name. */
        __Text_Slice__ __Name__ = __Parser_State__->__Current__.__Kind__ == __Token_IDENTIFIER__
                                      ? __Parser_State__->__Current__.__Lexeme__
                                      : __Parser_State__->__Current__.__Lexeme__;
        if (!__Parser_Advance__(__Parser_State__))
        {
            return NULL;
        }
        if (__Parser_State__->__Current__.__Kind__ == __Token_DOT_OPERATOR__)
        {
            return __Parser_Parse_Enum_Pattern__(__Parser_State__, __Name__, __Start__);
        }

        __Pattern__ = __Parser_New_Pattern__(__Parser_State__, __Ast_Pattern_Binding__, __Start__);
        if (__Pattern__ != NULL)
        {
            __Pattern__->__As__.__Binding__ = __Name__;
        }
        return __Pattern__;
    }
}
