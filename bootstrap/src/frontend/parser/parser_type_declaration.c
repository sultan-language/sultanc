/* Parses type declarations. */

#include "frontend/parser/cursor.h"
#include "frontend/identifier_identity.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/type_internal.h"

#include <stdalign.h>

/* Returns the parser enum constructor name exists. */
static int __Parser_Enum_Constructor_Name_Exists__(const __Vector__ *__Constructors__,
                                                   __Text_Slice__ __Name__)
{
    /* Tracks the index. */
    size_t __Index__ = 0U;

    for (__Index__ = 0U; __Index__ < __Constructors__->__Count__; ++__Index__)
    {
        /* References the existing. */
        const __Ast_Enum_Constructor__ *__Existing__ =
            (const __Ast_Enum_Constructor__ *)__Vector_At_Const__(__Constructors__, __Index__);
        if (__Existing__ != NULL &&
            __Identifier_Identity_Equals__(__Existing__->__Name__, __Name__))
        {
            return 1;
        }
    }
    return 0;
}

/* Parses the parser type declaration. */
__Ast_Type_Declaration__ *__Parser_Parse_Type_Declaration__(__Parser__ *__Parser_State__,
                                                            __Text_Slice__ __Name__,
                                                            __Source_Position__ __Start__)
{
    /* References the declaration. */
    __Ast_Type_Declaration__ *__Declaration__ = __Parser_New_Type_Declaration__(__Parser_State__);
    if (__Declaration__ == NULL)
    {
        return NULL;
    }
    __Declaration__->__Name__ = __Name__;

    if (__Parser_Accept__(__Parser_State__, __Token_LEFT_BRACE_OPERATOR__))
    {
        /* Stores the fields. */
        __Vector__ __Fields__;
        __Vector_Init__(&__Fields__, sizeof(__Ast_Struct_Field__));
        __Declaration__->__Kind__ = __Ast_Type_Decl_Struct__;
        while (__Parser_State__->__Current__.__Kind__ != __Token_RIGHT_BRACE_OPERATOR__)
        {
            /* Stores the field. */
            __Ast_Struct_Field__ __Field__;
            /* Stores the field start. */
            __Source_Position__ __Field_Start__;
            /* References the field type. */
            __Ast_Type__ *__Field_Type__ = NULL;
            if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
            {
                __Vector_Destroy__(&__Fields__);
                (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Struct_Field__);
                return NULL;
            }
            __Field_Start__ = __Parser_State__->__Current__.__Span__.__Start__;
            __Field__.__Name__ = __Parser_State__->__Current__.__Lexeme__;
            if (!__Parser_Advance__(__Parser_State__) ||
                !__Parser_Expect__(__Parser_State__, __Token_COLON_OPERATOR__))
            {
                __Vector_Destroy__(&__Fields__);
                return NULL;
            }
            __Field_Type__ = __Parser_Parse_Type__(__Parser_State__);
            if (__Field_Type__ == NULL)
            {
                __Vector_Destroy__(&__Fields__);
                return NULL;
            }
            __Field__.__Slot__ = __Parser_Exterior_Slot__(
                __Parser_State__,
                __Field_Type__,
                __Parser_Span__(__Field_Start__, __Parser_State__->__Previous__.__Span__.__End__));
            if (__Vector_Push__(&__Fields__, &__Field__) == NULL)
            {
                __Vector_Destroy__(&__Fields__);
                (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
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
        __Declaration__->__As__.__Struct__.__Count__ = __Fields__.__Count__;
        __Declaration__->__As__.__Struct__.__Fields__ =
            (__Ast_Struct_Field__ *)__Parser_Freeze_Vector__(
                __Parser_State__, &__Fields__, alignof(__Ast_Struct_Field__));
        __Vector_Destroy__(&__Fields__);
        return __Declaration__;
    }

    if (__Parser_Accept__(__Parser_State__, __Token_OR_OPERATOR__))
    {
        /* Stores the constructors. */
        __Vector__ __Constructors__;
        __Vector_Init__(&__Constructors__, sizeof(__Ast_Enum_Constructor__));
        __Declaration__->__Kind__ = __Ast_Type_Decl_Enum__;
        for (;;)
        {
            /* Stores the constructor. */
            __Ast_Enum_Constructor__ __Constructor__;
            /* Stores the payload. */
            __Vector__ __Payload__;
            /* Stores the ctor start. */
            __Source_Position__ __Ctor_Start__;
            /* Stores the ctor name span. */
            __Source_Span__ __Ctor_Name_Span__;
            __Vector_Init__(&__Payload__, sizeof(__Ast_Slot__));
            if (!__Parser_Token_Is_Contextual_Name__(&__Parser_State__->__Current__))
            {
                __Vector_Destroy__(&__Payload__);
                __Vector_Destroy__(&__Constructors__);
                (void)__Parser_Fail__(__Parser_State__,
                                      __Diag_Word_Syntax_Expected_Enum_Constructor__);
                return NULL;
            }
            __Ctor_Start__ = __Parser_State__->__Current__.__Span__.__Start__;
            __Ctor_Name_Span__ = __Parser_State__->__Current__.__Span__;
            if (!__Parser_Take_Contextual_Name__(__Parser_State__, &__Constructor__.__Name__))
            {
                __Vector_Destroy__(&__Payload__);
                __Vector_Destroy__(&__Constructors__);
                return NULL;
            }
            if (__Parser_Enum_Constructor_Name_Exists__(&__Constructors__,
                                                        __Constructor__.__Name__))
            {
                __Vector_Destroy__(&__Payload__);
                __Vector_Destroy__(&__Constructors__);
                __Diagnostic_Begin__(__Parser_State__->__Diagnostic__,
                                     __E0205_Duplicate_Declaration__,
                                     __Ctor_Name_Span__);
                __Parser_State__->__Failed__ = 1;
                return NULL;
            }
            if (__Parser_Accept__(__Parser_State__, __Token_OF__))
            {
                /* Stores the parenthesized. */
                int __Parenthesized__ =
                    __Parser_Accept__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__);
                for (;;)
                {
                    /* Stores the payload start. */
                    __Source_Position__ __Payload_Start__ =
                        __Parser_State__->__Current__.__Span__.__Start__;
                    /* References the payload type. */
                    __Ast_Type__ *__Payload_Type__ = __Parser_Parse_Type__(__Parser_State__);
                    /* Stores the payload slot. */
                    __Ast_Slot__ __Payload_Slot__;
                    if (__Payload_Type__ == NULL)
                    {
                        __Vector_Destroy__(&__Payload__);
                        __Vector_Destroy__(&__Constructors__);
                        return NULL;
                    }
                    __Payload_Slot__ = __Parser_Exterior_Slot__(
                        __Parser_State__,
                        __Payload_Type__,
                        __Parser_Span__(__Payload_Start__,
                                        __Parser_State__->__Previous__.__Span__.__End__));
                    if (__Vector_Push__(&__Payload__, &__Payload_Slot__) == NULL)
                    {
                        __Vector_Destroy__(&__Payload__);
                        __Vector_Destroy__(&__Constructors__);
                        (void)__Parser_Fail_Internal__(__Parser_State__,
                                                       __Diag_Word_Syntax_Internal_Oom__);
                        return NULL;
                    }
                    if (!__Parenthesized__ ||
                        !__Parser_Accept__(__Parser_State__, __Token_COMMA_OPERATOR__))
                    {
                        break;
                    }
                }
                if (__Parenthesized__ &&
                    !__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
                {
                    __Vector_Destroy__(&__Payload__);
                    __Vector_Destroy__(&__Constructors__);
                    return NULL;
                }
            }
            __Constructor__.__Payload_Count__ = __Payload__.__Count__;
            __Constructor__.__Payload_Slots__ = (__Ast_Slot__ *)__Parser_Freeze_Vector__(
                __Parser_State__, &__Payload__, alignof(__Ast_Slot__));
            __Vector_Destroy__(&__Payload__);
            (void)__Ctor_Start__;
            if (__Vector_Push__(&__Constructors__, &__Constructor__) == NULL)
            {
                __Vector_Destroy__(&__Constructors__);
                (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
                return NULL;
            }
            if (!__Parser_Accept__(__Parser_State__, __Token_OR_OPERATOR__))
            {
                break;
            }
        }
        __Declaration__->__As__.__Enum__.__Count__ = __Constructors__.__Count__;
        __Declaration__->__As__.__Enum__.__Constructors__ =
            (__Ast_Enum_Constructor__ *)__Parser_Freeze_Vector__(
                __Parser_State__, &__Constructors__, alignof(__Ast_Enum_Constructor__));
        __Vector_Destroy__(&__Constructors__);
        return __Declaration__;
    }

    (void)__Start__;
    (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Type__);
    return NULL;
}
