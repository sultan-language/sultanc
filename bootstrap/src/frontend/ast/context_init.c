/* Initializes AST context storage. */

#include "frontend/ast/context_lifecycle.h"
#include "frontend/ast/context.h"

/* Initializes the AST context. */
void __Ast_Context_Init__(__Ast_Context__ *__Context__)
{
    if (__Context__ == NULL)
    {
        return;
    }
    __Arena_Init__(&__Context__->__Arena__, 16384U);
}
