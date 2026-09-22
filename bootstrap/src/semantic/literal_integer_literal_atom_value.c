#include "semantic/literal.h"

#include <stdint.h>

int __Semantic_Integer_Literal_Atom_Value__(const __Ast_Atom__ *__Atom__, int64_t *__Out_Value__)
{
    if (__Atom__ == NULL || __Out_Value__ == NULL || __Atom__->__Kind__ != __Ast_Atom_Literal__ ||
        __Atom__->__As__.__Literal__ == NULL ||
        __Atom__->__As__.__Literal__->__Kind__ != __Ast_Literal_Integer__)
    {
        return 0;
    }
    *__Out_Value__ = __Atom__->__As__.__Literal__->__As__.__Integer__.__Value__;
    return 1;
}
