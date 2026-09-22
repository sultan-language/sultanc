#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/expression_internal.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/token_payload.h"

__Ast_Atom__ __Parser_Parse_Atom__(__Parser__ *__Parser_State__, int *__Ok__)
{
    __Ast_Atom__ __Atom__;
    __Ast_Literal__ *__Literal__ = NULL;
    *__Ok__ = 0;
    __Atom__.__Kind__ = __Ast_Atom_Literal__;
    __Atom__.__As__.__Literal__ = NULL;
    switch (__Parser_State__->__Current__.__Kind__)
    {
        case __Token_LIT_INT__:
        {
            __Integer_Literal__ __Integer__;
            if (!__Parser_Current_Integer__(__Parser_State__, &__Integer__))
            {
                return __Atom__;
            }
            __Literal__ = __Parser_New_Literal__(
                __Parser_State__, __Ast_Literal_Integer__, __Parser_State__->__Current__.__Span__);
            if (__Literal__ == NULL)
            {
                return __Atom__;
            }
            __Literal__->__As__.__Integer__.__Value__ = __Integer__.__Value__;
            __Literal__->__As__.__Integer__.__Spelling__ = __Integer__.__Spelling__;
            break;
        }
        case __Token_LIT_CHAR__:
        {
            uint32_t __Character__;
            if (!__Parser_Current_Character__(__Parser_State__, &__Character__))
            {
                return __Atom__;
            }
            __Literal__ = __Parser_New_Literal__(__Parser_State__,
                                                 __Ast_Literal_Character__,
                                                 __Parser_State__->__Current__.__Span__);
            if (__Literal__ == NULL)
            {
                return __Atom__;
            }
            __Literal__->__As__.__Character__ = __Character__;
            break;
        }
        case __Token_LIT_BOOL__:
        {
            int __Boolean__;
            if (!__Parser_Current_Boolean__(__Parser_State__, &__Boolean__))
            {
                return __Atom__;
            }
            __Literal__ = __Parser_New_Literal__(
                __Parser_State__, __Ast_Literal_Boolean__, __Parser_State__->__Current__.__Span__);
            if (__Literal__ == NULL)
            {
                return __Atom__;
            }
            __Literal__->__As__.__Boolean__ = __Boolean__;
            break;
        }
        case __Token_LIT_STR__:
        {
            __Text_Slice__ __Text__;
            if (!__Parser_Current_Text__(__Parser_State__, &__Text__))
            {
                return __Atom__;
            }
            __Literal__ = __Parser_New_Literal__(
                __Parser_State__, __Ast_Literal_String__, __Parser_State__->__Current__.__Span__);
            if (__Literal__ == NULL)
            {
                return __Atom__;
            }
            __Literal__->__As__.__String__ =
                __Parser_Copy_Literal_Text__(__Parser_State__, __Text__);
            if (__Literal__->__As__.__String__.__Data__ == NULL && __Text__.__Length__ != 0U)
            {
                return __Atom__;
            }
            break;
        }
        case __Token_LIT_UNDEF__:
            __Literal__ = __Parser_New_Literal__(__Parser_State__,
                                                 __Ast_Literal_Undefined__,
                                                 __Parser_State__->__Current__.__Span__);
            if (__Literal__ == NULL)
            {
                return __Atom__;
            }
            break;
        case __Token_IDENTIFIER__:
            __Atom__.__Kind__ = __Ast_Atom_Lvalue__;
            __Atom__.__As__.__Lvalue__ = __Parser_Parse_Lvalue__(__Parser_State__);
            if (__Atom__.__As__.__Lvalue__ == NULL)
            {
                return __Atom__;
            }
            *__Ok__ = 1;
            return __Atom__;
        default:
            (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Atom__);
            return __Atom__;
    }
    __Atom__.__Kind__ = __Ast_Atom_Literal__;
    __Atom__.__As__.__Literal__ = __Literal__;
    if (!__Parser_Advance__(__Parser_State__))
    {
        return __Atom__;
    }
    *__Ok__ = 1;
    return __Atom__;
}
