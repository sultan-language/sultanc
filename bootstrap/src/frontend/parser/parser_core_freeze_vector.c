#include "frontend/ast/storage.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/internal.h"

void *__Parser_Freeze_Vector__(__Parser__ *__Parser_State__,
                               const __Vector__ *__Vector_State__,
                               size_t __Alignment__)
{
    if (__Parser_State__ == NULL || __Vector_State__ == NULL || __Vector_State__->__Count__ == 0U)
    {
        return NULL;
    }
    return __Ast_Copy_Array__(__Parser_State__->__Ast__,
                              __Vector_State__->__Data__,
                              __Vector_State__->__Count__,
                              __Vector_State__->__Element_Size__,
                              __Alignment__);
}
