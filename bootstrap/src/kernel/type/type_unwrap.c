#include "kernel/type/type.h"

__Ast_Type__ *__Type_Unwrap_Mutable__(__Ast_Type__ *__Type__)
{
    while (__Type__ != NULL && __Type__->__Kind__ == __Ast_Type_Mutable__)
    {
        __Type__ = __Type__->__As__.__Inner__;
    }
    return __Type__;
}
