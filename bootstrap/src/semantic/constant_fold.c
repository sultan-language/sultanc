/* Folds module constants (canonical: compile-time evaluation of a قيمة_عليا
 * initializer, docs/TYPE_SYSTEM.md "Negation and constant arithmetic").
 *
 * The initializer runs with the run-time semantics: an operation that would
 * trap at run time (integer overflow, division or remainder by zero, the
 * signed minimum divided by -1, a shift by the width or more) is rejected.
 * An integer literal has no type of its own until a cast or a typed operand
 * gives it one; the result must fit the declared Type, which may be wider
 * than the expression's (a widening). Stage0 folds what module constants in
 * compiler and std sources use: literals, unary and binary operators, casts
 * between integer types and from ط8 to حرف, other module constants, and
 * vector literals of those. */

#include "semantic/constant_fold.h"
#include "semantic/diagnostic.h"
#include "kernel/name/name.h"
#include "kernel/type/type.h"

#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    __Ast_Constant_Kind__ __Kind__;
    int64_t __Value__;
    int __Wide__;
    int __Typed__;
    unsigned __Bits__;
    int __Signed__;
    int __Negative_Float__;
    unsigned char __Float_Steps__[4];
    size_t __Float_Step_Count__;
    __Text_Slice__ __Text__;
} __Fold_Value__;

static int __Fold_Expression__(__Semantic_Context__ *__Context__,
                               const __Program_Unit__ *__Unit__,
                               const __Ast_Expression__ *__Expression__,
                               __Fold_Value__ *__Out__);

/* Answers whether the integer fits BITS with SIGNED. */
static int __Fold_Fits__(int64_t __Value__, int __Wide__, unsigned __Bits__, int __Signed__)
{
    if (__Bits__ == 0U || __Bits__ > 64U)
    {
        return 0;
    }
    if (__Wide__)
    {
        return !__Signed__ && __Bits__ == 64U;
    }
    if (__Signed__)
    {
        if (__Bits__ == 64U)
        {
            return 1;
        }
        return __Value__ >= -((int64_t)1 << (__Bits__ - 1U)) &&
               __Value__ <= (((int64_t)1 << (__Bits__ - 1U)) - 1);
    }
    if (__Value__ < 0)
    {
        return 0;
    }
    return __Bits__ == 64U || (uint64_t)__Value__ <= ((((uint64_t)1) << __Bits__) - 1U);
}

/* Truncates or extends to BITS (two's complement), as an integer cast does. */
static void __Fold_Truncate__(__Fold_Value__ *__Value__, unsigned __Bits__, int __Signed__)
{
    uint64_t __Raw__ = (uint64_t)__Value__->__Value__;
    if (__Bits__ < 64U)
    {
        __Raw__ &= (((uint64_t)1) << __Bits__) - 1U;
        if (__Signed__ && (__Raw__ & (((uint64_t)1) << (__Bits__ - 1U))) != 0U)
        {
            __Raw__ |= ~((((uint64_t)1) << __Bits__) - 1U);
        }
    }
    __Value__->__Value__ = (int64_t)__Raw__;
    __Value__->__Wide__ = !__Signed__ && __Bits__ == 64U && __Raw__ > (uint64_t)INT64_MAX;
    __Value__->__Typed__ = 1;
    __Value__->__Bits__ = __Bits__;
    __Value__->__Signed__ = __Signed__;
}

/* Resolves an integer Type to its width and signedness. */
static int __Fold_Integer_Type__(__Semantic_Context__ *__Context__,
                                 __Ast_Type__ *__Type__,
                                 unsigned *__Out_Bits__,
                                 int *__Out_Signed__)
{
    __Resolved_Type__ __Resolved__;
    if (!__Type_Resolve__(__Context__, __Type__, &__Resolved__))
    {
        return 0;
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Signed_Integer__ ||
        __Resolved__.__Kind__ == __Resolved_Type_Unsigned_Integer__)
    {
        *__Out_Bits__ = __Resolved__.__Bits__;
        *__Out_Signed__ = __Resolved__.__Kind__ == __Resolved_Type_Signed_Integer__;
        return 1;
    }
    return 0;
}

/* Fails folding at SPAN. */
static int __Fold_Fail__(__Semantic_Context__ *__Context__, const __Ast_Expression__ *__Expression__)
{
    __Source_Span__ __Span__ = {0};
    if (__Expression__ != NULL)
    {
        __Span__ = __Expression__->__Header__.__Span__;
    }
    return __Semantic_Fail__(__Context__, __E0400_Mismatched_Types__, __Span__);
}

/* Gives an untyped operand the other operand's Type. */
static void __Fold_Unify__(__Fold_Value__ *__Left__, __Fold_Value__ *__Right__)
{
    if (__Left__->__Typed__ && !__Right__->__Typed__)
    {
        __Right__->__Typed__ = 1;
        __Right__->__Bits__ = __Left__->__Bits__;
        __Right__->__Signed__ = __Left__->__Signed__;
    }
    else if (__Right__->__Typed__ && !__Left__->__Typed__)
    {
        __Left__->__Typed__ = 1;
        __Left__->__Bits__ = __Right__->__Bits__;
        __Left__->__Signed__ = __Right__->__Signed__;
    }
}

/* Answers whether the result is checked as a 64-bit unsigned value. */
static int __Fold_Is_U64__(const __Fold_Value__ *__Value__)
{
    return __Value__->__Typed__ && !__Value__->__Signed__ && __Value__->__Bits__ == 64U;
}

/* Stores an integer result and checks it against the operand Type. */
static int __Fold_Integer_Result__(__Fold_Value__ *__Out__,
                                   const __Fold_Value__ *__Type_Of__,
                                   uint64_t __Raw__,
                                   int __Raw_Is_Unsigned__)
{
    memset(__Out__, 0, sizeof(*__Out__));
    __Out__->__Kind__ = __Ast_Constant_Integer__;
    __Out__->__Typed__ = __Type_Of__->__Typed__;
    __Out__->__Bits__ = __Type_Of__->__Bits__;
    __Out__->__Signed__ = __Type_Of__->__Signed__;
    __Out__->__Value__ = (int64_t)__Raw__;
    __Out__->__Wide__ = __Raw_Is_Unsigned__ && __Raw__ > (uint64_t)INT64_MAX;
    if (!__Out__->__Typed__)
    {
        return !__Out__->__Wide__;
    }
    return __Fold_Fits__(__Out__->__Value__, __Out__->__Wide__, __Out__->__Bits__,
                         __Out__->__Signed__);
}

/* Folds a binary integer operation with trap checks. */
static int __Fold_Integer_Binary__(__Ast_Binary_Operation__ __Operation__,
                                   __Fold_Value__ *__Left__,
                                   __Fold_Value__ *__Right__,
                                   __Fold_Value__ *__Out__)
{
    __Fold_Unify__(__Left__, __Right__);
    if (__Fold_Is_U64__(__Left__) || __Left__->__Wide__ || __Right__->__Wide__)
    {
        /* Unsigned 64-bit arithmetic: both operands are non-negative. */
        uint64_t __A__ = (uint64_t)__Left__->__Value__;
        uint64_t __B__ = (uint64_t)__Right__->__Value__;
        uint64_t __R__ = 0U;
        if ((!__Left__->__Wide__ && __Left__->__Value__ < 0) ||
            (!__Right__->__Wide__ && __Right__->__Value__ < 0))
        {
            return 0;
        }
        switch (__Operation__)
        {
            case __Binary_Add__:
                __R__ = __A__ + __B__;
                if (__R__ < __A__)
                    return 0;
                break;
            case __Binary_Subtract__:
                if (__B__ > __A__)
                    return 0;
                __R__ = __A__ - __B__;
                break;
            case __Binary_Multiply__:
                if (__A__ != 0U && __B__ > UINT64_MAX / __A__)
                    return 0;
                __R__ = __A__ * __B__;
                break;
            case __Binary_Divide__:
                if (__B__ == 0U)
                    return 0;
                __R__ = __A__ / __B__;
                break;
            case __Binary_Modulo__:
                if (__B__ == 0U)
                    return 0;
                __R__ = __A__ % __B__;
                break;
            case __Binary_And__:
                __R__ = __A__ & __B__;
                break;
            case __Binary_Or__:
                __R__ = __A__ | __B__;
                break;
            case __Binary_Xor__:
                __R__ = __A__ ^ __B__;
                break;
            case __Binary_Shift_Left_Logical__:
                if (__B__ >= 64U || (__B__ != 0U && (__A__ >> (64U - __B__)) != 0U))
                    return 0;
                __R__ = __A__ << __B__;
                break;
            case __Binary_Shift_Right_Logical__:
                if (__B__ >= 64U)
                    return 0;
                __R__ = __A__ >> __B__;
                break;
            default:
                return 0;
        }
        if (!__Left__->__Typed__)
        {
            __Left__->__Typed__ = 1;
            __Left__->__Bits__ = 64U;
            __Left__->__Signed__ = 0;
        }
        return __Fold_Integer_Result__(__Out__, __Left__, __R__, 1);
    }
    {
        int64_t __A__ = __Left__->__Value__;
        int64_t __B__ = __Right__->__Value__;
        int64_t __R__ = 0;
        unsigned __Width__ = __Left__->__Typed__ ? __Left__->__Bits__ : 64U;
        switch (__Operation__)
        {
            case __Binary_Add__:
                if ((__B__ > 0 && __A__ > INT64_MAX - __B__) ||
                    (__B__ < 0 && __A__ < INT64_MIN - __B__))
                    return 0;
                __R__ = __A__ + __B__;
                break;
            case __Binary_Subtract__:
                if ((__B__ < 0 && __A__ > INT64_MAX + __B__) ||
                    (__B__ > 0 && __A__ < INT64_MIN + __B__))
                    return 0;
                __R__ = __A__ - __B__;
                break;
            case __Binary_Multiply__:
                if (__A__ != 0 && __B__ != 0)
                {
                    if (__A__ == -1 && __B__ == INT64_MIN)
                        return 0;
                    if (__B__ == -1 && __A__ == INT64_MIN)
                        return 0;
                    if (__A__ > 0 ? (__B__ > 0 ? __A__ > INT64_MAX / __B__
                                                : __B__ < INT64_MIN / __A__)
                                  : (__B__ > 0 ? __A__ < INT64_MIN / __B__
                                                : __A__ < INT64_MAX / __B__))
                        return 0;
                }
                __R__ = __A__ * __B__;
                break;
            case __Binary_Divide__:
                if (__B__ == 0 || (__A__ == INT64_MIN && __B__ == -1))
                    return 0;
                __R__ = __A__ / __B__;
                break;
            case __Binary_Modulo__:
                if (__B__ == 0 || (__A__ == INT64_MIN && __B__ == -1))
                    return 0;
                __R__ = __A__ % __B__;
                break;
            case __Binary_And__:
                __R__ = __A__ & __B__;
                break;
            case __Binary_Or__:
                __R__ = __A__ | __B__;
                break;
            case __Binary_Xor__:
                __R__ = __A__ ^ __B__;
                break;
            case __Binary_Shift_Left_Logical__:
                if (__B__ < 0 || (uint64_t)__B__ >= __Width__)
                    return 0;
                if (__A__ < 0)
                    return 0;
                if (__B__ != 0 && (((uint64_t)__A__) >> (63U - (unsigned)__B__)) != 0U)
                    return 0;
                __R__ = (int64_t)(((uint64_t)__A__) << __B__);
                break;
            case __Binary_Shift_Right_Logical__:
            {
                uint64_t __Raw__ = (uint64_t)__A__;
                if (__B__ < 0 || (uint64_t)__B__ >= __Width__)
                    return 0;
                if (__Width__ < 64U)
                    __Raw__ &= (((uint64_t)1) << __Width__) - 1U;
                __R__ = (int64_t)(__Raw__ >> __B__);
                break;
            }
            default:
                return 0;
        }
        return __Fold_Integer_Result__(__Out__, __Left__, (uint64_t)__R__, 0);
    }
}

/* Folds a comparison of two integers, characters or booleans. */
static int __Fold_Compare__(__Ast_Binary_Operation__ __Operation__,
                            __Fold_Value__ *__Left__,
                            __Fold_Value__ *__Right__,
                            int *__Out__)
{
    int __Order__;
    if (__Left__->__Kind__ != __Right__->__Kind__ ||
        (__Left__->__Kind__ != __Ast_Constant_Integer__ &&
         __Left__->__Kind__ != __Ast_Constant_Character__ &&
         __Left__->__Kind__ != __Ast_Constant_Boolean__))
    {
        return 0;
    }
    if (__Left__->__Wide__ || __Right__->__Wide__)
    {
        uint64_t __A__ = (uint64_t)__Left__->__Value__;
        uint64_t __B__ = (uint64_t)__Right__->__Value__;
        if ((!__Left__->__Wide__ && __Left__->__Value__ < 0) ||
            (!__Right__->__Wide__ && __Right__->__Value__ < 0))
            return 0;
        __Order__ = __A__ < __B__ ? -1 : (__A__ > __B__ ? 1 : 0);
    }
    else
    {
        __Order__ = __Left__->__Value__ < __Right__->__Value__
                        ? -1
                        : (__Left__->__Value__ > __Right__->__Value__ ? 1 : 0);
    }
    switch (__Operation__)
    {
        case __Binary_Equal__:
            *__Out__ = __Order__ == 0;
            return 1;
        case __Binary_Not_Equal__:
            *__Out__ = __Order__ != 0;
            return 1;
        case __Binary_Less_Than__:
            *__Out__ = __Order__ < 0;
            return 1;
        case __Binary_Less_Or_Equal__:
            *__Out__ = __Order__ <= 0;
            return 1;
        case __Binary_Greater_Than__:
            *__Out__ = __Order__ > 0;
            return 1;
        case __Binary_Greater_Or_Equal__:
            *__Out__ = __Order__ >= 0;
            return 1;
        default:
            return 0;
    }
}

/* Converts a folded constant of a global back to a fold value of TYPE. */
static int __Fold_From_Constant__(__Semantic_Context__ *__Context__,
                                  const __Ast_Constant__ *__Constant__,
                                  __Ast_Type__ *__Type__,
                                  __Fold_Value__ *__Out__)
{
    memset(__Out__, 0, sizeof(*__Out__));
    __Out__->__Kind__ = __Constant__->__Kind__;
    __Out__->__Value__ = __Constant__->__Integer__;
    __Out__->__Wide__ = __Constant__->__Wide__;
    __Out__->__Text__ = __Constant__->__Text__;
    __Out__->__Negative_Float__ = __Constant__->__Negative_Float__;
    if (__Constant__->__Kind__ == __Ast_Constant_Vector__)
    {
        return 0;
    }
    if (__Constant__->__Kind__ == __Ast_Constant_Integer__)
    {
        if (!__Fold_Integer_Type__(__Context__, __Type__, &__Out__->__Bits__, &__Out__->__Signed__))
        {
            return 0;
        }
        __Out__->__Typed__ = 1;
    }
    return 1;
}

/* Folds one module constant named NAME, as seen from UNIT. */
static int __Fold_Global_Name__(__Semantic_Context__ *__Context__,
                                const __Program_Unit__ *__Unit__,
                                const __Ast_Expression__ *__Expression__,
                                __Text_Slice__ __Name__,
                                __Fold_Value__ *__Out__)
{
    __Semantic_Global_Entry__ *__Global__ = NULL;
    if (__Name_Resolve_Global__(__Context__, __Unit__, __Name__, &__Global__) !=
            __Name_Lookup_Found__ ||
        __Global__ == NULL || __Global__->__Declaration__ == NULL ||
        __Global__->__Declaration__->__External__ ||
        __Global__->__Declaration__->__Type__ == NULL ||
        __Global__->__Declaration__->__Type__->__Kind__ == __Ast_Type_Mutable__)
    {
        return __Fold_Fail__(__Context__, __Expression__);
    }
    if (!__Semantic_Fold_Global__(__Context__, __Global__))
    {
        return 0;
    }
    if (!__Fold_From_Constant__(__Context__, __Global__->__Declaration__->__Constant__,
                                __Global__->__Declaration__->__Type__, __Out__))
    {
        return __Fold_Fail__(__Context__, __Expression__);
    }
    return 1;
}

static int __Fold_Expression__(__Semantic_Context__ *__Context__,
                               const __Program_Unit__ *__Unit__,
                               const __Ast_Expression__ *__Expression__,
                               __Fold_Value__ *__Out__)
{
    memset(__Out__, 0, sizeof(*__Out__));
    if (__Expression__ == NULL)
    {
        return __Fold_Fail__(__Context__, __Expression__);
    }
    switch (__Expression__->__Kind__)
    {
        case __Ast_Expression_Atom__:
        {
            const __Ast_Atom__ *__Atom__ = &__Expression__->__As__.__Atom__;
            if (__Atom__->__Kind__ == __Ast_Atom_Literal__ && __Atom__->__As__.__Literal__ != NULL)
            {
                const __Ast_Literal__ *__Literal__ = __Atom__->__As__.__Literal__;
                switch (__Literal__->__Kind__)
                {
                    case __Ast_Literal_Boolean__:
                        __Out__->__Kind__ = __Ast_Constant_Boolean__;
                        __Out__->__Value__ = __Literal__->__As__.__Boolean__ != 0;
                        return 1;
                    case __Ast_Literal_Integer__:
                        __Out__->__Kind__ = __Ast_Constant_Integer__;
                        __Out__->__Value__ = __Literal__->__As__.__Integer__.__Value__;
                        __Out__->__Wide__ = __Literal__->__As__.__Integer__.__Unsigned_Wide__;
                        return 1;
                    case __Ast_Literal_Character__:
                        __Out__->__Kind__ = __Ast_Constant_Character__;
                        __Out__->__Value__ = (int64_t)__Literal__->__As__.__Character__;
                        return 1;
                    case __Ast_Literal_String__:
                        __Out__->__Kind__ = __Ast_Constant_Text__;
                        __Out__->__Text__ = __Literal__->__As__.__String__;
                        return 1;
                    case __Ast_Literal_Float__:
                        __Out__->__Kind__ = __Ast_Constant_Float__;
                        __Out__->__Text__ = __Literal__->__As__.__String__;
                        return 1;
                    default:
                        return __Fold_Fail__(__Context__, __Expression__);
                }
            }
            if (__Atom__->__Kind__ == __Ast_Atom_Lvalue__ && __Atom__->__As__.__Lvalue__ != NULL &&
                __Atom__->__As__.__Lvalue__->__Kind__ == __Ast_Lvalue_Base__ &&
                __Atom__->__As__.__Lvalue__->__As__.__Base__.__Kind__ ==
                    __Ast_Lvalue_Base_Identifier__)
            {
                return __Fold_Global_Name__(
                    __Context__, __Unit__, __Expression__,
                    __Atom__->__As__.__Lvalue__->__As__.__Base__.__As__.__Identifier__, __Out__);
            }
            return __Fold_Fail__(__Context__, __Expression__);
        }
        case __Ast_Expression_Unary__:
        {
            __Fold_Value__ __Operand__;
            if (!__Fold_Expression__(__Context__, __Unit__, __Expression__->__As__.__Unary__.__Operand__,
                                     &__Operand__))
            {
                return 0;
            }
            switch (__Expression__->__As__.__Unary__.__Operation__)
            {
                case __Unary_Not__:
                    if (__Operand__.__Kind__ != __Ast_Constant_Boolean__)
                        return __Fold_Fail__(__Context__, __Expression__);
                    *__Out__ = __Operand__;
                    __Out__->__Value__ = !__Operand__.__Value__;
                    return 1;
                case __Unary_Negate__:
                    if (__Operand__.__Kind__ == __Ast_Constant_Float__)
                    {
                        *__Out__ = __Operand__;
                        __Out__->__Negative_Float__ = !__Operand__.__Negative_Float__;
                        return 1;
                    }
                    if (__Operand__.__Kind__ != __Ast_Constant_Integer__ || __Operand__.__Wide__ ||
                        __Operand__.__Value__ == INT64_MIN ||
                        (__Operand__.__Typed__ && !__Operand__.__Signed__ && __Operand__.__Value__ != 0))
                        return __Fold_Fail__(__Context__, __Expression__);
                    *__Out__ = __Operand__;
                    __Out__->__Value__ = -__Operand__.__Value__;
                    if (__Out__->__Typed__ &&
                        !__Fold_Fits__(__Out__->__Value__, 0, __Out__->__Bits__, __Out__->__Signed__))
                        return __Fold_Fail__(__Context__, __Expression__);
                    return 1;
                case __Unary_Bitwise_Not__:
                    if (__Operand__.__Kind__ != __Ast_Constant_Integer__)
                        return __Fold_Fail__(__Context__, __Expression__);
                    *__Out__ = __Operand__;
                    __Out__->__Value__ = ~__Operand__.__Value__;
                    if (__Out__->__Typed__)
                        __Fold_Truncate__(__Out__, __Out__->__Bits__, __Out__->__Signed__);
                    return 1;
                default:
                    return __Fold_Fail__(__Context__, __Expression__);
            }
        }
        case __Ast_Expression_Binary__:
        {
            __Fold_Value__ __Left__;
            __Fold_Value__ __Right__;
            __Ast_Binary_Operation__ __Operation__ = __Expression__->__As__.__Binary__.__Operation__;
            if (!__Fold_Expression__(__Context__, __Unit__, __Expression__->__As__.__Binary__.__Left__,
                                     &__Left__))
            {
                return 0;
            }
            if (__Operation__ == __Binary_Logical_And__ || __Operation__ == __Binary_Logical_Or__)
            {
                if (__Left__.__Kind__ != __Ast_Constant_Boolean__)
                    return __Fold_Fail__(__Context__, __Expression__);
                if (!__Fold_Expression__(__Context__, __Unit__,
                                         __Expression__->__As__.__Binary__.__Right__, &__Right__))
                    return 0;
                if (__Right__.__Kind__ != __Ast_Constant_Boolean__)
                    return __Fold_Fail__(__Context__, __Expression__);
                __Out__->__Kind__ = __Ast_Constant_Boolean__;
                __Out__->__Value__ = __Operation__ == __Binary_Logical_And__
                                         ? (__Left__.__Value__ && __Right__.__Value__)
                                         : (__Left__.__Value__ || __Right__.__Value__);
                return 1;
            }
            if (!__Fold_Expression__(__Context__, __Unit__, __Expression__->__As__.__Binary__.__Right__,
                                     &__Right__))
            {
                return 0;
            }
            switch (__Operation__)
            {
                case __Binary_Equal__:
                case __Binary_Not_Equal__:
                case __Binary_Less_Than__:
                case __Binary_Less_Or_Equal__:
                case __Binary_Greater_Than__:
                case __Binary_Greater_Or_Equal__:
                {
                    int __Truth__ = 0;
                    if (!__Fold_Compare__(__Operation__, &__Left__, &__Right__, &__Truth__))
                        return __Fold_Fail__(__Context__, __Expression__);
                    __Out__->__Kind__ = __Ast_Constant_Boolean__;
                    __Out__->__Value__ = __Truth__;
                    return 1;
                }
                default:
                    break;
            }
            if (__Left__.__Kind__ == __Ast_Constant_Boolean__ &&
                __Right__.__Kind__ == __Ast_Constant_Boolean__ &&
                (__Operation__ == __Binary_And__ || __Operation__ == __Binary_Or__ ||
                 __Operation__ == __Binary_Xor__))
            {
                __Out__->__Kind__ = __Ast_Constant_Boolean__;
                __Out__->__Value__ = __Operation__ == __Binary_And__
                                         ? (__Left__.__Value__ & __Right__.__Value__)
                                         : (__Operation__ == __Binary_Or__
                                                ? (__Left__.__Value__ | __Right__.__Value__)
                                                : (__Left__.__Value__ ^ __Right__.__Value__));
                return 1;
            }
            if (__Left__.__Kind__ != __Ast_Constant_Integer__ ||
                __Right__.__Kind__ != __Ast_Constant_Integer__ ||
                !__Fold_Integer_Binary__(__Operation__, &__Left__, &__Right__, __Out__))
            {
                return __Fold_Fail__(__Context__, __Expression__);
            }
            return 1;
        }
        case __Ast_Expression_Conversion__:
        {
            __Fold_Value__ __Operand__;
            __Resolved_Type__ __Target__;
            if (!__Fold_Expression__(__Context__, __Unit__,
                                     __Expression__->__As__.__Conversion__.__Operand__, &__Operand__) ||
                !__Type_Resolve__(__Context__, __Expression__->__As__.__Conversion__.__Target_Type__,
                                  &__Target__))
            {
                return 0;
            }
            if (__Operand__.__Kind__ == __Ast_Constant_Float__ &&
                (__Target__.__Kind__ == __Resolved_Type_Signed_Integer__ ||
                 __Target__.__Kind__ == __Resolved_Type_Unsigned_Integer__))
            {
                /* Truncated toward zero and saturated; NaN gives 0 (Rust's `as`).
                 * The decimal text is parsed by the host's correctly rounding
                 * strtod at the operand's width. */
                char __Digits__[512];
                double __Real__;
                int __Signed_Target__ = __Target__.__Kind__ == __Resolved_Type_Signed_Integer__;
                double __Power__ = 1.0;
                double __Low__;
                double __High__;
                unsigned __Step__;
                for (__Step__ = 0U; __Step__ + (__Signed_Target__ ? 1U : 0U) < __Target__.__Bits__; ++__Step__)
                    __Power__ *= 2.0;
                __Low__ = __Signed_Target__ ? -__Power__ : 0.0;
                __High__ = __Power__;
                if (__Operand__.__Text__.__Length__ + 1U > sizeof(__Digits__))
                    return __Fold_Fail__(__Context__, __Expression__);
                memcpy(__Digits__, __Operand__.__Text__.__Data__, __Operand__.__Text__.__Length__);
                __Digits__[__Operand__.__Text__.__Length__] = '\0';
                __Real__ = strtod(__Digits__, NULL);
                if (__Operand__.__Float_Step_Count__ != 0U &&
                    __Operand__.__Float_Steps__[__Operand__.__Float_Step_Count__ - 1U] == 32U)
                    __Real__ = (double)(float)__Real__;
                if (__Operand__.__Negative_Float__)
                    __Real__ = -__Real__;
                memset(__Out__, 0, sizeof(*__Out__));
                __Out__->__Kind__ = __Ast_Constant_Integer__;
                if (__Real__ != __Real__)
                    __Out__->__Value__ = 0;
                else if (__Real__ <= __Low__)
                    __Out__->__Value__ = __Signed_Target__ ? (int64_t)__Low__ : 0;
                else if (__Real__ >= __High__)
                {
                    if (!__Signed_Target__ && __Target__.__Bits__ == 64U)
                    {
                        __Out__->__Value__ = -1;
                        __Out__->__Wide__ = 1;
                    }
                    else
                        __Out__->__Value__ = __Signed_Target__ ? (int64_t)(__High__ - 1.0)
                                                               : (int64_t)(__High__ - 1.0);
                    if (__Signed_Target__ && __Target__.__Bits__ == 64U)
                        __Out__->__Value__ = INT64_MAX;
                }
                else if (!__Signed_Target__ && __Real__ >= 9223372036854775808.0)
                {
                    __Out__->__Value__ = (int64_t)(uint64_t)__Real__;
                    __Out__->__Wide__ = 1;
                }
                else
                    __Out__->__Value__ = (int64_t)__Real__;
                __Out__->__Typed__ = 1;
                __Out__->__Bits__ = __Target__.__Bits__;
                __Out__->__Signed__ = __Signed_Target__;
                return 1;
            }
            if (__Target__.__Kind__ == __Resolved_Type_Float__ &&
                (__Operand__.__Kind__ == __Ast_Constant_Float__ ||
                 __Operand__.__Kind__ == __Ast_Constant_Integer__))
            {
                /* float -> float records a conversion step; integer -> float
                 * is the integer's decimal text parsed at the target width
                 * (round to nearest even). */
                *__Out__ = __Operand__;
                if (__Operand__.__Kind__ == __Ast_Constant_Integer__)
                {
                    char *__Text__ = (char *)__Arena_Allocate__(&__Context__->__Generic_Arena__, 24U, 1U);
                    if (__Text__ == NULL)
                        return __Fold_Fail__(__Context__, __Expression__);
                    if (__Operand__.__Wide__)
                        (void)snprintf(__Text__, 24U, "%llu", (unsigned long long)(uint64_t)__Operand__.__Value__);
                    else
                        (void)snprintf(__Text__, 24U, "%lld",
                                       (long long)(__Operand__.__Value__ < 0 ? -__Operand__.__Value__
                                                                             : __Operand__.__Value__));
                    memset(__Out__, 0, sizeof(*__Out__));
                    __Out__->__Kind__ = __Ast_Constant_Float__;
                    __Out__->__Negative_Float__ = !__Operand__.__Wide__ && __Operand__.__Value__ < 0;
                    __Out__->__Text__.__Data__ = __Text__;
                    __Out__->__Text__.__Length__ = strlen(__Text__);
                    __Out__->__Float_Steps__[0] = (unsigned char)__Target__.__Bits__;
                    __Out__->__Float_Step_Count__ = 1U;
                    return 1;
                }
                if (__Out__->__Float_Step_Count__ == 0U)
                {
                    /* An untyped float literal is an f64. */
                    __Out__->__Float_Steps__[0] = 64U;
                    __Out__->__Float_Step_Count__ = 1U;
                }
                if (__Out__->__Float_Steps__[__Out__->__Float_Step_Count__ - 1U] != __Target__.__Bits__)
                {
                    if (__Out__->__Float_Step_Count__ >= 4U)
                        return __Fold_Fail__(__Context__, __Expression__);
                    __Out__->__Float_Steps__[__Out__->__Float_Step_Count__++] =
                        (unsigned char)__Target__.__Bits__;
                }
                return 1;
            }
            if (__Target__.__Kind__ == __Resolved_Type_Signed_Integer__ ||
                __Target__.__Kind__ == __Resolved_Type_Unsigned_Integer__)
            {
                if (__Operand__.__Kind__ != __Ast_Constant_Integer__ &&
                    __Operand__.__Kind__ != __Ast_Constant_Character__)
                    return __Fold_Fail__(__Context__, __Expression__);
                *__Out__ = __Operand__;
                __Out__->__Kind__ = __Ast_Constant_Integer__;
                __Fold_Truncate__(__Out__, __Target__.__Bits__,
                                  __Target__.__Kind__ == __Resolved_Type_Signed_Integer__);
                return 1;
            }
            if (__Target__.__Kind__ == __Resolved_Type_Character__)
            {
                if (__Operand__.__Kind__ == __Ast_Constant_Character__)
                {
                    *__Out__ = __Operand__;
                    return 1;
                }
                if (__Operand__.__Kind__ != __Ast_Constant_Integer__ || __Operand__.__Wide__ ||
                    __Operand__.__Value__ < 0 || __Operand__.__Value__ > 255)
                    return __Fold_Fail__(__Context__, __Expression__);
                *__Out__ = __Operand__;
                __Out__->__Kind__ = __Ast_Constant_Character__;
                __Out__->__Typed__ = 0;
                return 1;
            }
            if (__Target__.__Kind__ == __Resolved_Type_Boolean__ &&
                __Operand__.__Kind__ == __Ast_Constant_Boolean__)
            {
                *__Out__ = __Operand__;
                return 1;
            }
            return __Fold_Fail__(__Context__, __Expression__);
        }
        default:
            return __Fold_Fail__(__Context__, __Expression__);
    }
}

/* Builds the constant of TYPE from a fold value. */
static int __Fold_To_Constant__(__Semantic_Context__ *__Context__,
                                const __Ast_Expression__ *__Expression__,
                                __Fold_Value__ *__Value__,
                                __Ast_Type__ *__Type__,
                                __Ast_Constant__ *__Out__)
{
    __Resolved_Type__ __Resolved__;
    memset(__Out__, 0, sizeof(*__Out__));
    if (!__Type_Resolve__(__Context__, __Type__, &__Resolved__))
    {
        return 0;
    }
    switch (__Resolved__.__Kind__)
    {
        case __Resolved_Type_Signed_Integer__:
        case __Resolved_Type_Unsigned_Integer__:
            if (__Value__->__Kind__ != __Ast_Constant_Integer__ ||
                !__Fold_Fits__(__Value__->__Value__, __Value__->__Wide__, __Resolved__.__Bits__,
                               __Resolved__.__Kind__ == __Resolved_Type_Signed_Integer__))
                return __Fold_Fail__(__Context__, __Expression__);
            break;
        case __Resolved_Type_Boolean__:
            if (__Value__->__Kind__ != __Ast_Constant_Boolean__)
                return __Fold_Fail__(__Context__, __Expression__);
            break;
        case __Resolved_Type_Character__:
            if (__Value__->__Kind__ != __Ast_Constant_Character__)
                return __Fold_Fail__(__Context__, __Expression__);
            break;
        case __Resolved_Type_String__:
            if (__Value__->__Kind__ != __Ast_Constant_Text__)
                return __Fold_Fail__(__Context__, __Expression__);
            break;
        case __Resolved_Type_Float__:
            if (__Value__->__Kind__ != __Ast_Constant_Float__)
                return __Fold_Fail__(__Context__, __Expression__);
            break;
        default:
            return __Fold_Fail__(__Context__, __Expression__);
    }
    __Out__->__Kind__ = __Value__->__Kind__;
    __Out__->__Integer__ = __Value__->__Value__;
    __Out__->__Wide__ = __Value__->__Wide__;
    __Out__->__Text__ = __Value__->__Text__;
    __Out__->__Negative_Float__ = __Value__->__Negative_Float__;
    memcpy(__Out__->__Float_Steps__, __Value__->__Float_Steps__, sizeof(__Out__->__Float_Steps__));
    __Out__->__Float_Step_Count__ = __Value__->__Float_Step_Count__;
    if (__Out__->__Kind__ == __Ast_Constant_Float__ && __Out__->__Float_Step_Count__ != 0U &&
        __Out__->__Float_Steps__[__Out__->__Float_Step_Count__ - 1U] != __Resolved__.__Bits__)
    {
        /* A float constant of another width than the declared one. */
        return __Fold_Fail__(__Context__, __Expression__);
    }
    return 1;
}

int __Semantic_Fold_Global__(__Semantic_Context__ *__Context__, __Semantic_Global_Entry__ *__Entry__)
{
    __Ast_Global__ *__Global__;
    const __Program_Unit__ *__Saved_Unit__;
    __Ast_Type__ *__Type__;
    __Ast_Constant__ *__Constant__;
    int __Ok__ = 0;

    if (__Context__ == NULL || __Entry__ == NULL || (__Global__ = __Entry__->__Declaration__) == NULL)
    {
        return 0;
    }
    if (__Global__->__External__ || __Global__->__Fold_State__ == 2)
    {
        return 1;
    }
    if (__Global__->__Fold_State__ == 1)
    {
        /* A constant that depends on itself. */
        return __Semantic_Fail__(__Context__, __E0400_Mismatched_Types__,
                                 __Global__->__Header__.__Span__);
    }
    __Global__->__Fold_State__ = 1;
    __Saved_Unit__ = __Context__->__Active_Unit__;
    __Context__->__Active_Unit__ = __Entry__->__Unit__;
    __Type__ = __Type_Unwrap_Mutable__(__Global__->__Type__);
    __Constant__ = (__Ast_Constant__ *)__Arena_Allocate__(
        &__Context__->__Generic_Arena__, sizeof(*__Constant__), alignof(__Ast_Constant__));
    if (__Constant__ == NULL || __Type__ == NULL)
    {
        __Context__->__Active_Unit__ = __Saved_Unit__;
        return __Semantic_Fail__(__Context__, __E1100_Internal_Context_Error__,
                                 __Global__->__Header__.__Span__);
    }
    memset(__Constant__, 0, sizeof(*__Constant__));
    if (__Global__->__Vector_Form__)
    {
        __Resolved_Type__ __Resolved__;
        size_t __Index__;
        if (!__Type_Resolve__(__Context__, __Type__, &__Resolved__) ||
            __Resolved__.__Kind__ != __Resolved_Type_Vector__ || __Resolved__.__Inner__ == NULL)
        {
            __Context__->__Active_Unit__ = __Saved_Unit__;
            return __Semantic_Fail__(__Context__, __E0400_Mismatched_Types__,
                                     __Global__->__Header__.__Span__);
        }
        __Constant__->__Kind__ = __Ast_Constant_Vector__;
        __Constant__->__Element_Count__ = __Global__->__Element_Count__;
        if (__Global__->__Element_Count__ != 0U)
        {
            __Constant__->__Elements__ = (__Ast_Constant__ *)__Arena_Allocate__(
                &__Context__->__Generic_Arena__,
                __Global__->__Element_Count__ * sizeof(__Ast_Constant__), alignof(__Ast_Constant__));
            if (__Constant__->__Elements__ == NULL)
            {
                __Context__->__Active_Unit__ = __Saved_Unit__;
                return __Semantic_Fail__(__Context__, __E1100_Internal_Context_Error__,
                                         __Global__->__Header__.__Span__);
            }
        }
        __Ok__ = 1;
        for (__Index__ = 0U; __Ok__ && __Index__ < __Global__->__Element_Count__; ++__Index__)
        {
            __Fold_Value__ __Element__;
            __Ok__ = __Fold_Expression__(__Context__, __Entry__->__Unit__,
                                         __Global__->__Elements__[__Index__], &__Element__) &&
                     __Fold_To_Constant__(__Context__, __Global__->__Elements__[__Index__], &__Element__,
                                          __Resolved__.__Inner__,
                                          &__Constant__->__Elements__[__Index__]);
        }
    }
    else
    {
        __Fold_Value__ __Value__;
        __Ok__ = __Fold_Expression__(__Context__, __Entry__->__Unit__, __Global__->__Initializer__,
                                     &__Value__) &&
                 __Fold_To_Constant__(__Context__, __Global__->__Initializer__, &__Value__, __Type__,
                                      __Constant__);
    }
    __Context__->__Active_Unit__ = __Saved_Unit__;
    if (!__Ok__)
    {
        return 0;
    }
    __Global__->__Constant__ = __Constant__;
    __Global__->__Fold_State__ = 2;
    return 1;
}
