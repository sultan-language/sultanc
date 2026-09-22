#include "semantic/literal.h"

#include <stdint.h>

int __Semantic_Integer_Literal_Expression_Value__(const __Ast_Expression__ *__Expression__,
                                                  int64_t *__Out_Value__)
{
    int64_t __Value__ = 0;
    if (__Expression__ == NULL || __Out_Value__ == NULL)
    {
        return 0;
    }
    if (__Expression__->__Kind__ == __Ast_Expression_Atom__)
    {
        return __Semantic_Integer_Literal_Atom_Value__(&__Expression__->__As__.__Atom__,
                                                       __Out_Value__);
    }
    if (__Expression__->__Kind__ != __Ast_Expression_Unary__ ||
        __Expression__->__As__.__Unary__.__Operation__ != __Unary_Negate__ ||
        !__Semantic_Integer_Literal_Expression_Value__(__Expression__->__As__.__Unary__.__Operand__,
                                                       &__Value__))
    {
        return 0;
    }
    if (__Value__ == INT64_MIN)
    {
        return 0;
    }
    *__Out_Value__ = -__Value__;
    return 1;
}
