#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/initializer.h"

#include <stdalign.h>

int __Parser_Parse_Record_Initializer__(__Parser__ *__Parser_State__,
                                        __Source_Position__ __Start__,
                                        __Initializer__ *__Out_Initializer__)
{
    __Vector__ __Fields__;

    __Vector_Init__(&__Fields__, sizeof(__Ast_Record_Input__));
    while (__Parser_State__->__Current__.__Kind__ != __Token_RIGHT_BRACE_OPERATOR__)
    {
        __Ast_Record_Input__ __Field__;
        int __Atom_Ok__ = 0;

        if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
        {
            __Vector_Destroy__(&__Fields__);
            return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Record_Field__);
        }
        __Field__.__Name__ = __Parser_State__->__Current__.__Lexeme__;
        if (!__Parser_Advance__(__Parser_State__) ||
            !__Parser_Expect__(__Parser_State__, __Token_COLON_OPERATOR__))
        {
            __Vector_Destroy__(&__Fields__);
            return 0;
        }
        __Field__.__Value__ = __Parser_Parse_Atom__(__Parser_State__, &__Atom_Ok__);
        if (!__Atom_Ok__ || __Vector_Push__(&__Fields__, &__Field__) == NULL)
        {
            __Vector_Destroy__(&__Fields__);
            if (!__Parser_State__->__Failed__)
            {
                (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
            }
            return 0;
        }
        if (!__Parser_Accept__(__Parser_State__, __Token_COMMA_OPERATOR__))
        {
            break;
        }
    }

    if (!__Parser_Expect__(__Parser_State__, __Token_RIGHT_BRACE_OPERATOR__))
    {
        __Vector_Destroy__(&__Fields__);
        return 0;
    }

    __Out_Initializer__->__Kind__ = __Initializer_Record__;
    __Out_Initializer__->__As__.__Record__.__Count__ = __Fields__.__Count__;
    __Out_Initializer__->__As__.__Record__.__Fields__ =
        (__Ast_Record_Input__ *)__Parser_Freeze_Vector__(
            __Parser_State__, &__Fields__, alignof(__Ast_Record_Input__));
    __Out_Initializer__->__Span__ =
        __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__);
    __Vector_Destroy__(&__Fields__);
    return !__Parser_State__->__Failed__;
}
