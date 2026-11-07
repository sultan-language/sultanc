/* Reads integer values from literal expressions. */

#include "semantic/literal.h"

#include <stdint.h>

/* Returns the semantic integer literal expression value. */
int __Semantic_Integer_Literal_Expression_Value__(const __Ast_Expression__ *__Expression__,
                                                  int64_t *__Out_Value__)
{
    /* Stores the value. */
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
    if (__Expression__->__Kind__ == __Ast_Expression_Binary__ &&
        (__Expression__->__As__.__Binary__.__Operation__ == __Binary_Add__ ||
         __Expression__->__As__.__Binary__.__Operation__ == __Binary_Subtract__ ||
         __Expression__->__As__.__Binary__.__Operation__ == __Binary_Multiply__ ||
         __Expression__->__As__.__Binary__.__Operation__ == __Binary_Shift_Left_Logical__ ||
         __Expression__->__As__.__Binary__.__Operation__ == __Binary_Shift_Right_Logical__ ||
         __Expression__->__As__.__Binary__.__Operation__ == __Binary_Or__ ||
         __Expression__->__As__.__Binary__.__Operation__ == __Binary_And__ ||
         __Expression__->__As__.__Binary__.__Operation__ == __Binary_Xor__))
    {
        /* Arithmetic of literals is a literal too (`(-127 - 1)` fits ص8),
         * as long as it does not overflow 64 bits. */
        int64_t __Left__ = 0;
        int64_t __Right__ = 0;
        if (!__Semantic_Integer_Literal_Expression_Value__(__Expression__->__As__.__Binary__.__Left__,
                                                           &__Left__) ||
            !__Semantic_Integer_Literal_Expression_Value__(__Expression__->__As__.__Binary__.__Right__,
                                                           &__Right__))
        {
            return 0;
        }
        switch (__Expression__->__As__.__Binary__.__Operation__)
        {
            case __Binary_Add__:
                if ((__Right__ > 0 && __Left__ > INT64_MAX - __Right__) ||
                    (__Right__ < 0 && __Left__ < INT64_MIN - __Right__))
                    return 0;
                *__Out_Value__ = __Left__ + __Right__;
                return 1;
            case __Binary_Subtract__:
                if ((__Right__ < 0 && __Left__ > INT64_MAX + __Right__) ||
                    (__Right__ > 0 && __Left__ < INT64_MIN + __Right__))
                    return 0;
                *__Out_Value__ = __Left__ - __Right__;
                return 1;
            case __Binary_Shift_Left_Logical__:
                if (__Left__ < 0 || __Right__ < 0 || __Right__ > 62 ||
                    (__Left__ >> (62 - __Right__)) != 0)
                    return 0;
                *__Out_Value__ = __Left__ << __Right__;
                return 1;
            case __Binary_Shift_Right_Logical__:
                if (__Left__ < 0 || __Right__ < 0 || __Right__ > 63)
                    return 0;
                *__Out_Value__ = __Left__ >> __Right__;
                return 1;
            case __Binary_Or__:
                if (__Left__ < 0 || __Right__ < 0)
                    return 0;
                *__Out_Value__ = __Left__ | __Right__;
                return 1;
            case __Binary_And__:
                if (__Left__ < 0 || __Right__ < 0)
                    return 0;
                *__Out_Value__ = __Left__ & __Right__;
                return 1;
            case __Binary_Xor__:
                if (__Left__ < 0 || __Right__ < 0)
                    return 0;
                *__Out_Value__ = __Left__ ^ __Right__;
                return 1;
            default:
                if (__Left__ != 0 && __Right__ != 0 &&
                    (__Left__ > 3037000499 || __Left__ < -3037000499 || __Right__ > 3037000499 ||
                     __Right__ < -3037000499))
                    return 0;
                *__Out_Value__ = __Left__ * __Right__;
                return 1;
        }
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
