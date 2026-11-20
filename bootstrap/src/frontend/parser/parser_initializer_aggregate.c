/* Parses aggregate initializer elements. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/initializer.h"

#include <stdalign.h>
#include <string.h>

/* Parses the parser aggregate elements. */
int __Parser_Parse_Aggregate_Elements__(__Parser__ *__Parser_State__,
                                        __Token_Kind__ __Closing_Token__,
                                        const __Ast_Atom__ *__First_Atom__,
                                        __Ast_Atom__ **__Out_Values__,
                                        __Ast_Expression__ ***__Out_Expressions__,
                                        size_t *__Out_Count__)
{
    /* Stores the values. */
    __Vector__ __Values__;
    /* Stores the non-atom elements (canonical: any expression). */
    __Vector__ __Expressions__;
    /* Tracks whether an element is not an atom. */
    int __Has_Expressions__ = 0;

    __Vector_Init__(&__Values__, sizeof(__Ast_Atom__));
    __Vector_Init__(&__Expressions__, sizeof(__Ast_Expression__ *));
    *__Out_Expressions__ = NULL;
    if (__First_Atom__ != NULL &&
        (__Vector_Push__(&__Values__, __First_Atom__) == NULL ||
         __Vector_Push__(&__Expressions__, &(__Ast_Expression__ *){NULL}) == NULL))
    {
        __Vector_Destroy__(&__Values__);
        __Vector_Destroy__(&__Expressions__);
        return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
    }

    while (__Parser_State__->__Current__.__Kind__ != __Closing_Token__)
    {
        /* Stores the element. */
        __Ast_Expression__ *__Element__ = __Parser_Parse_Expression__(__Parser_State__);
        /* Stores the atom. */
        __Ast_Atom__ __Atom__;
        /* Stores the non-atom element. */
        __Ast_Expression__ *__Expression__ = NULL;

        memset(&__Atom__, 0, sizeof(__Atom__));
        if (__Element__ != NULL)
        {
            if (__Element__->__Kind__ == __Ast_Expression_Atom__)
            {
                __Atom__ = __Element__->__As__.__Atom__;
            }
            else
            {
                __Expression__ = __Element__;
                __Has_Expressions__ = 1;
            }
        }
        if (__Element__ == NULL || __Vector_Push__(&__Values__, &__Atom__) == NULL ||
            __Vector_Push__(&__Expressions__, &__Expression__) == NULL)
        {
            __Vector_Destroy__(&__Values__);
            __Vector_Destroy__(&__Expressions__);
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
        __Vector_Destroy__(&__Expressions__);
        return 0;
    }

    *__Out_Count__ = __Values__.__Count__;
    *__Out_Values__ = (__Ast_Atom__ *)__Parser_Freeze_Vector__(
        __Parser_State__, &__Values__, alignof(__Ast_Atom__));
    if (__Has_Expressions__)
    {
        *__Out_Expressions__ = (__Ast_Expression__ **)__Parser_Freeze_Vector__(
            __Parser_State__, &__Expressions__, alignof(__Ast_Expression__ *));
    }
    __Vector_Destroy__(&__Values__);
    __Vector_Destroy__(&__Expressions__);
    return !__Parser_State__->__Failed__;
}
