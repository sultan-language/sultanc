/* Builds parser slots for temporary values. */

#include "frontend/ast/storage.h"
#include "frontend/parser/type_internal.h"

/* Returns the parser exterior slot. */
__Ast_Slot__ __Parser_Exterior_Slot__(__Parser__ *__Parser_State__,
                                      __Ast_Type__ *__Type__,
                                      __Source_Span__ __Span__)
{
    /* Stores the slot. */
    __Ast_Slot__ __Slot__;
    __Slot__.__Header__ = __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
    __Slot__.__Type__ = __Type__;
    return __Slot__;
}
