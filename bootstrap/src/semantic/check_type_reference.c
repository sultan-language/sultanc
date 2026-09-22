#include "semantic/check_type_reference.h"
#include "kernel/layout/layout.h"

int __Semantic_Check_Type_Reference__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__)
{
    size_t __Size__ = 0U;
    size_t __Alignment__ = 0U;

    return __Layout_Type__(__Context__, __Type__, &__Size__, &__Alignment__);
}
