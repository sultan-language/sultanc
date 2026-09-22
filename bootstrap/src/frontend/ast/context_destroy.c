#include "frontend/ast/context_lifecycle.h"
#include "frontend/ast/context.h"

void __Ast_Context_Destroy__(__Ast_Context__ *__Context__)
{
    if (__Context__ == NULL)
    {
        return;
    }
    __Arena_Destroy__(&__Context__->__Arena__);
}
