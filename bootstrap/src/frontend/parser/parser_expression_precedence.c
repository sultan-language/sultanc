/* Parses binary expressions by precedence. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/span.h"
#include "frontend/parser/expression_internal.h"

/* Parses the parser expression min. */
__Ast_Expression__ *__Parser_Parse_Expression_Min__(__Parser__ *__Parser_State__,
                                                    int __Minimum_Precedence__)
{
    /* References the left. */
    __Ast_Expression__ *__Left__ = __Parser_Parse_Unary__(__Parser_State__);
    if (__Left__ == NULL)
    {
        return NULL;
    }
    for (;;)
    {
        /* Stores the precedence. */
        int __Precedence__ = 0;
        /* Stores the operation. */
        __Ast_Binary_Operation__ __Operation__ = __Binary_Add__;
        /* References the right. */
        __Ast_Expression__ *__Right__ = NULL;
        /* References the combined. */
        __Ast_Expression__ *__Combined__ = NULL;
        if (!__Parser_Binary_Info__(
                __Parser_State__->__Current__.__Kind__, &__Precedence__, &__Operation__) ||
            __Precedence__ < __Minimum_Precedence__)
        {
            break;
        }
        if (!__Parser_Advance__(__Parser_State__))
        {
            return NULL;
        }
        __Right__ = __Parser_Parse_Expression_Min__(__Parser_State__, __Precedence__ + 1);
        if (__Right__ == NULL)
        {
            return NULL;
        }
        __Combined__ =
            __Parser_New_Expression__(__Parser_State__,
                                      __Ast_Expression_Binary__,
                                      __Parser_Span__(__Left__->__Header__.__Span__.__Start__,
                                                      __Right__->__Header__.__Span__.__End__));
        if (__Combined__ == NULL)
        {
            return NULL;
        }
        __Combined__->__As__.__Binary__.__Operation__ = __Operation__;
        __Combined__->__As__.__Binary__.__Left__ = __Left__;
        __Combined__->__As__.__Binary__.__Right__ = __Right__;
        __Left__ = __Combined__;
    }
    return __Left__;
}
