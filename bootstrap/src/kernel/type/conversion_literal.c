/* Classifies safe literal and type conversions. */

#include "kernel/type/conversion.h"

#include <stdint.h>

/* Returns the signed integer fits. */
static int __Signed_Integer_Fits__(int64_t __Value__, unsigned __Bits__)
{
    /* Stores the minimum. */
    int64_t __Minimum__;
    /* Stores the maximum. */
    int64_t __Maximum__;
    if (__Bits__ == 0U || __Bits__ > 64U)
    {
        return 0;
    }
    if (__Bits__ == 64U)
    {
        return 1;
    }
    __Maximum__ = ((int64_t)1 << (__Bits__ - 1U)) - 1;
    __Minimum__ = -((int64_t)1 << (__Bits__ - 1U));
    return __Value__ >= __Minimum__ && __Value__ <= __Maximum__;
}

/* Returns the unsigned integer fits. */
static int __Unsigned_Integer_Fits__(int64_t __Value__, unsigned __Bits__)
{
    /* Stores the maximum. */
    uint64_t __Maximum__;
    if (__Bits__ == 0U || __Bits__ > 64U || __Value__ < 0)
    {
        return 0;
    }
    if (__Bits__ == 64U)
    {
        return 1;
    }
    __Maximum__ = ((uint64_t)1 << __Bits__) - 1U;
    return (uint64_t)__Value__ <= __Maximum__;
}

/* Returns the type integer literal fits. */
int __Type_Integer_Literal_Fits__(__Semantic_Context__ *__Context__,
                                  int64_t __Value__,
                                  __Ast_Type__ *__Target_Type__)
{
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;
    if (__Context__ == NULL || __Target_Type__ == NULL ||
        !__Type_Resolve__(__Context__, __Target_Type__, &__Resolved__))
    {
        return 0;
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Signed_Integer__)
    {
        return __Signed_Integer_Fits__(__Value__, __Resolved__.__Bits__);
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Unsigned_Integer__)
    {
        return __Unsigned_Integer_Fits__(__Value__, __Resolved__.__Bits__);
    }
    return 0;
}

/* Classifies the type conversion. */
__Type_Conversion_Class__ __Type_Conversion_Classify__(__Semantic_Context__ *__Context__,
                                                       __Ast_Type__ *__Source__,
                                                       __Ast_Type__ *__Target__)
{
    /* Stores the source type. */
    __Resolved_Type__ __Source_Type__;
    /* Stores the target type. */
    __Resolved_Type__ __Target_Type__;

    if (__Context__ == NULL || __Source__ == NULL || __Target__ == NULL)
    {
        return __Type_Conversion_Invalid__;
    }
    if (__Type_Compatible__(__Context__, __Source__, __Target__))
    {
        return __Type_Conversion_Implicit_Safe__;
    }
    if (!__Type_Resolve__(__Context__, __Source__, &__Source_Type__) ||
        !__Type_Resolve__(__Context__, __Target__, &__Target_Type__))
    {
        return __Type_Conversion_Invalid__;
    }

    if (__Source_Type__.__Kind__ == __Resolved_Type_Reference__ &&
        __Target_Type__.__Kind__ == __Resolved_Type_Reference__)
    {
        if (!__Type_Compatible__(__Context__, __Source_Type__.__Inner__, __Target_Type__.__Inner__))
        {
            return __Type_Conversion_Invalid__;
        }
        if (__Source_Type__.__Reference_Mutable__ && !__Target_Type__.__Reference_Mutable__)
        {
            return __Type_Conversion_Implicit_Safe__;
        }
        if (!__Source_Type__.__Reference_Mutable__ && __Target_Type__.__Reference_Mutable__)
        {
            return __Type_Conversion_Invalid__;
        }
    }

    if (__Source_Type__.__Kind__ == __Resolved_Type_Signed_Integer__ &&
        __Target_Type__.__Kind__ == __Resolved_Type_Signed_Integer__)
    {
        return __Target_Type__.__Bits__ >= __Source_Type__.__Bits__
                   ? __Type_Conversion_Implicit_Safe__
                   : __Type_Conversion_Explicit_Safe__;
    }
    if (__Source_Type__.__Kind__ == __Resolved_Type_Unsigned_Integer__ &&
        __Target_Type__.__Kind__ == __Resolved_Type_Unsigned_Integer__)
    {
        return __Target_Type__.__Bits__ >= __Source_Type__.__Bits__
                   ? __Type_Conversion_Implicit_Safe__
                   : __Type_Conversion_Explicit_Safe__;
    }
    if (__Source_Type__.__Kind__ == __Resolved_Type_Unsigned_Integer__ &&
        __Target_Type__.__Kind__ == __Resolved_Type_Signed_Integer__)
    {
        return __Target_Type__.__Bits__ > __Source_Type__.__Bits__
                   ? __Type_Conversion_Implicit_Safe__
                   : __Type_Conversion_Explicit_Safe__;
    }
    if (__Source_Type__.__Kind__ == __Resolved_Type_Signed_Integer__ &&
        __Target_Type__.__Kind__ == __Resolved_Type_Unsigned_Integer__)
    {
        return __Type_Conversion_Explicit_Safe__;
    }
    if (__Source_Type__.__Kind__ == __Resolved_Type_Character__ &&
        __Target_Type__.__Kind__ == __Resolved_Type_Unsigned_Integer__)
    {
        return __Target_Type__.__Bits__ >= 32U ? __Type_Conversion_Implicit_Safe__
                                               : __Type_Conversion_Explicit_Safe__;
    }

    return __Type_Conversion_Invalid__;
}

/* Checks whether the type conversion is safe. */
int __Type_Conversion_Is_Safe__(__Semantic_Context__ *__Context__,
                                __Ast_Type__ *__Source__,
                                __Ast_Type__ *__Target__)
{
    return __Type_Conversion_Classify__(__Context__, __Source__, __Target__) ==
           __Type_Conversion_Implicit_Safe__;
}
