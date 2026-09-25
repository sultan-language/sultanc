/* Initializes parser state and token flow. */

#include "frontend/lexer/lifecycle.h"
#include "frontend/lexer/scanning.h"
#include "frontend/parser/cursor.h"
#include "frontend/parser/lifecycle.h"
#include "frontend/parser/internal.h"

#include <string.h>

/* Reads the parser token. */
static int __Parser_Read_Token__(__Parser__ *__Parser_State__, __Token__ *__Out_Token__)
{
    if (__Lexer_Next__(&__Parser_State__->__Lexer__, __Out_Token__))
    {
        return 1;
    }
    if (__Lexer_Diagnostic__(&__Parser_State__->__Lexer__) != NULL)
    {
        *__Parser_State__->__Diagnostic__ = *__Lexer_Diagnostic__(&__Parser_State__->__Lexer__);
    }
    __Parser_State__->__Failed__ = 1;
    return 0;
}

/* Initializes the parser. */
int __Parser_Init__(__Parser__ *__Parser_State__,
                    const __Source_File__ *__Source__,
                    __Ast_Context__ *__Ast__,
                    __Diagnostic__ *__Diagnostic_State__)
{
    if (__Parser_State__ == NULL || __Source__ == NULL || __Ast__ == NULL ||
        __Diagnostic_State__ == NULL)
    {
        return 0;
    }
    memset(__Parser_State__, 0, sizeof(*__Parser_State__));
    __Parser_State__->__Source__ = __Source__;
    __Parser_State__->__Ast__ = __Ast__;
    __Parser_State__->__Diagnostic__ = __Diagnostic_State__;
    __Parser_State__->__Next_Temporary__ = 1U;
    __Lexer_Init__(&__Parser_State__->__Lexer__, __Source__);
    if (!__Parser_Read_Token__(__Parser_State__, &__Parser_State__->__Current__))
    {
        return 0;
    }
    if (!__Parser_Read_Token__(__Parser_State__, &__Parser_State__->__Next__))
    {
        return 0;
    }
    __Parser_State__->__Previous__ = __Parser_State__->__Current__;
    return 1;
}

/* Advances the parser. */
int __Parser_Advance__(__Parser__ *__Parser_State__)
{
    /* Stores the incoming. */
    __Token__ __Incoming__;
    if (__Parser_State__ == NULL || __Parser_State__->__Failed__)
    {
        return 0;
    }
    __Parser_State__->__Previous__ = __Parser_State__->__Current__;
    __Parser_State__->__Current__ = __Parser_State__->__Next__;
    if (__Parser_State__->__Current__.__Kind__ == __Token_EOF__)
    {
        __Parser_State__->__Next__ = __Parser_State__->__Current__;
        return 1;
    }
    if (!__Parser_Read_Token__(__Parser_State__, &__Incoming__))
    {
        return 0;
    }
    __Parser_State__->__Next__ = __Incoming__;
    return 1;
}
