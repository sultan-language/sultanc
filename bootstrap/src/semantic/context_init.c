#include "semantic/context_lifecycle.h"
#include "semantic/context.h"

#include <string.h>

void __Semantic_Context_Init__(__Semantic_Context__ *__Context__, __Program__ *__Program_State__)
{
    memset(__Context__, 0, sizeof(*__Context__));
    __Context__->__Program__ = __Program_State__;
    __Vector_Init__(&__Context__->__Types__, sizeof(__Semantic_Type_Entry__));
    __Vector_Init__(&__Context__->__Functions__, sizeof(__Semantic_Function_Entry__));
    __Vector_Init__(&__Context__->__Type_Owners__, sizeof(__Semantic_Type_Owner_Fact__));
}
