/* Allocates AST nodes from the AST arena. */

#include "frontend/ast/storage.h"
#include "frontend/ast/context.h"

#include <string.h>

/* Allocates the AST. */
void *__Ast_Allocate__(__Ast_Context__ *__Context__, size_t __Size__, size_t __Alignment__)
{
    /* References the memory. */
    void *__Memory__ = NULL;
    if (__Context__ == NULL)
    {
        return NULL;
    }
    __Memory__ = __Arena_Allocate__(&__Context__->__Arena__, __Size__, __Alignment__);
    if (__Memory__ != NULL)
    {
        memset(__Memory__, 0, __Size__);
    }
    return __Memory__;
}
