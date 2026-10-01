/* Parses aggregate initializer elements. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/initializer.h"

#include <stdalign.h>

/* Parses the parser aggregate elements. */
int __Parser_Parse_Aggregate_Elements__(__Parser__ *__Parser_State__,
                                        __Token_Kind__ __Closing_Token__,
                                        const __Ast_Atom__ *__First_Atom__,
                                        __Ast_Atom__ **__Out_Values__,
                                        size_t *__Out_Count__)
{
    /* Stores the values. */
    __Vector__ __Values__;

    __Vector_Init__(&__Values__, sizeof(__Ast_Atom__));
    if (__First_Atom__ != NULL && __Vector_Push__(&__Values__, __First_Atom__) == NULL)
    {
        __Vector_Destroy__(&__Values__);
        return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
    }

    while (__Parser_State__->__Current__.__Kind__ != __Closing_Token__)
    {
        /* Stores the atom ok. */
        int __Atom_Ok__ = 0;
        /* Stores the atom. */
        __Ast_Atom__ __Atom__ = __Parser_Parse_Atom__(__Parser_State__, &__Atom_Ok__);

        if (!__Atom_Ok__ || __Vector_Push__(&__Values__, &__Atom__) == NULL)
        {
            __Vector_Destroy__(&__Values__);
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

    if (!__Parser_Expect__(__Parser_State__, __Closing_Token__))
    {
        __Vector_Destroy__(&__Values__);
        return 0;
    }

    *__Out_Count__ = __Values__.__Count__;
    *__Out_Values__ = (__Ast_Atom__ *)__Parser_Freeze_Vector__(
        __Parser_State__, &__Values__, alignof(__Ast_Atom__));
    __Vector_Destroy__(&__Values__);
    return !__Parser_State__->__Failed__;
}
