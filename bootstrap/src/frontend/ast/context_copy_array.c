#include "frontend/ast/storage.h"
#include "frontend/ast/context.h"

#include <stdint.h>
#include <string.h>

void *__Ast_Copy_Array__(__Ast_Context__ *__Context__,
                         const void *__Data__,
                         size_t __Count__,
                         size_t __Element_Size__,
                         size_t __Alignment__)
{
    void *__Copy__ = NULL;
    if (__Count__ == 0U)
    {
        return NULL;
    }
    if (__Element_Size__ != 0U && __Count__ > SIZE_MAX / __Element_Size__)
    {
        return NULL;
    }
    __Copy__ = __Ast_Allocate__(__Context__, __Count__ * __Element_Size__, __Alignment__);
    if (__Copy__ == NULL)
    {
        return NULL;
    }
    memcpy(__Copy__, __Data__, __Count__ * __Element_Size__);
    return __Copy__;
}
