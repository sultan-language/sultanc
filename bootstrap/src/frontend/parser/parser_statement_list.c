/* Builds statement lists. */

#include "frontend/parser/diagnostic.h"
#include "frontend/parser/statement_internal.h"

/* Adds the parser statement. */
int __Parser_Push_Statement__(__Parser__ *__Parser_State__,
                              __Vector__ *__Statements__,
                              __Ast_Statement__ *__Statement__)
{
    if (__Vector_Push__(__Statements__, &__Statement__) != NULL)
    {
        return 1;
    }
    return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
}
