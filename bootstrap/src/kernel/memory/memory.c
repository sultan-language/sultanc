/* Classifies ownership, views, and vector memory policy. */

#include "kernel/memory/memory.h"

#include <stdint.h>

/* Defines the memory vector minimum capacity macro. */
#define __Memory_Vector_Minimum_Capacity__ 4U

/* Removes surface mutability wrappers before classifying memory semantics. */
static const __Ast_Type__ *__Memory_Unwrap_Mutable__(const __Ast_Type__ *__Type__)
{
    while (__Type__ != NULL && __Type__->__Kind__ == __Ast_Type_Mutable__)
    {
        __Type__ = __Type__->__As__.__Inner__;
    }
    return __Type__;
}

/* Checks whether the memory type is owned recursive. */
static int __Memory_Type_Is_Owned_Recursive__(__Semantic_Context__ *__Context__,
                                              __Ast_Type__ *__Type__,
                                              unsigned __Depth__)
{
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;
    /* Tracks the index. */
    size_t __Index__;

    if (__Depth__ > 64U)
    {
        return 1;
    }
    __Type__ = __Type_Unwrap_Mutable__(__Type__);
    if (__Type__ == NULL)
    {
        return 0;
    }

    if (__Type__->__Kind__ == __Ast_Type_Box__ || __Type__->__Kind__ == __Ast_Type_Vector__ ||
        __Type__->__Kind__ == __Ast_Type_String__)
    {
        return 1;
    }
    if (__Type__->__Kind__ == __Ast_Type_Option__)
    {
        return __Memory_Type_Is_Owned_Recursive__(
            __Context__, __Type__->__As__.__Inner__, __Depth__ + 1U);
    }
    if (__Type__->__Kind__ == __Ast_Type_Result__)
    {
        return __Memory_Type_Is_Owned_Recursive__(
                   __Context__, __Type__->__As__.__Result__.__Ok__, __Depth__ + 1U) ||
               __Memory_Type_Is_Owned_Recursive__(
                   __Context__, __Type__->__As__.__Result__.__Error__, __Depth__ + 1U);
    }
    if (__Type__->__Kind__ != __Ast_Type_Named__ || __Context__ == NULL ||
        !__Type_Resolve__(__Context__, __Type__, &__Resolved__))
    {
        return 0;
    }

    /* Aliases resolve to their underlying kind before this point. */
    if (__Resolved__.__Kind__ == __Resolved_Type_String__ ||
        __Resolved__.__Kind__ == __Resolved_Type_Box__ ||
        __Resolved__.__Kind__ == __Resolved_Type_Vector__)
    {
        return 1;
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Struct__ && __Resolved__.__Named__ != NULL)
    {
        /* References the declaration. */
        __Ast_Type_Declaration__ *__Declaration__ = __Resolved__.__Named__->__Declaration__;
        for (__Index__ = 0U; __Index__ < __Declaration__->__As__.__Struct__.__Count__; ++__Index__)
        {
            if (__Memory_Type_Is_Owned_Recursive__(
                    __Context__,
                    __Declaration__->__As__.__Struct__.__Fields__[__Index__].__Slot__.__Type__,
                    __Depth__ + 1U))
            {
                return 1;
            }
        }
    }
    else if (__Resolved__.__Kind__ == __Resolved_Type_Enum__ && __Resolved__.__Named__ != NULL)
    {
        /* References the declaration. */
        __Ast_Type_Declaration__ *__Declaration__ = __Resolved__.__Named__->__Declaration__;
        /* Tracks the constructor index. */
        size_t __Constructor_Index__;
        for (__Constructor_Index__ = 0U;
             __Constructor_Index__ < __Declaration__->__As__.__Enum__.__Count__;
             ++__Constructor_Index__)
        {
            /* References the constructor. */
            __Ast_Enum_Constructor__ *__Constructor__ =
                &__Declaration__->__As__.__Enum__.__Constructors__[__Constructor_Index__];
            for (__Index__ = 0U; __Index__ < __Constructor__->__Payload_Count__; ++__Index__)
            {
                if (__Memory_Type_Is_Owned_Recursive__(
                        __Context__,
                        __Constructor__->__Payload_Slots__[__Index__].__Type__,
                        __Depth__ + 1U))
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Checks whether the memory type is owned. */
int __Memory_Type_Is_Owned__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__)
{
    return __Memory_Type_Is_Owned_Recursive__(__Context__, __Type__, 0U);
}

/* Checks whether the memory type is view. */
int __Memory_Type_Is_View__(const __Ast_Type__ *__Type__)
{
    __Type__ = __Memory_Unwrap_Mutable__(__Type__);
    return __Type__ != NULL && __Type__->__Kind__ == __Ast_Type_Reference__;
}

/* Returns the memory type contains view recursive. */
static int __Memory_Type_Contains_View_Recursive__(__Semantic_Context__ *__Context__,
                                                   __Ast_Type__ *__Type__,
                                                   unsigned __Depth__)
{
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;
    /* Tracks the index. */
    size_t __Index__;

    if (__Depth__ > 64U)
    {
        /* Recursive types containing an unresolved view are conservatively view-dependent. */
        return 1;
    }
    __Type__ = __Type_Unwrap_Mutable__(__Type__);
    if (__Type__ == NULL)
    {
        return 0;
    }
    if (__Type__->__Kind__ == __Ast_Type_Reference__)
    {
        return 1;
    }
    if (__Type__->__Kind__ == __Ast_Type_Option__)
    {
        return __Memory_Type_Contains_View_Recursive__(
            __Context__, __Type__->__As__.__Inner__, __Depth__ + 1U);
    }
    if (__Type__->__Kind__ == __Ast_Type_Result__)
    {
        return __Memory_Type_Contains_View_Recursive__(
                   __Context__, __Type__->__As__.__Result__.__Ok__, __Depth__ + 1U) ||
               __Memory_Type_Contains_View_Recursive__(
                   __Context__, __Type__->__As__.__Result__.__Error__, __Depth__ + 1U);
    }
    if (__Type__->__Kind__ != __Ast_Type_Named__ || __Context__ == NULL ||
        !__Type_Resolve__(__Context__, __Type__, &__Resolved__))
    {
        return 0;
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Reference__)
    {
        return 1;
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Struct__ && __Resolved__.__Named__ != NULL)
    {
        /* References the declaration. */
        __Ast_Type_Declaration__ *__Declaration__ = __Resolved__.__Named__->__Declaration__;
        for (__Index__ = 0U; __Index__ < __Declaration__->__As__.__Struct__.__Count__; ++__Index__)
        {
            if (__Memory_Type_Contains_View_Recursive__(
                    __Context__,
                    __Declaration__->__As__.__Struct__.__Fields__[__Index__].__Slot__.__Type__,
                    __Depth__ + 1U))
            {
                return 1;
            }
        }
    }
    else if (__Resolved__.__Kind__ == __Resolved_Type_Enum__ && __Resolved__.__Named__ != NULL)
    {
        /* References the declaration. */
        __Ast_Type_Declaration__ *__Declaration__ = __Resolved__.__Named__->__Declaration__;
        /* Tracks the constructor index. */
        size_t __Constructor_Index__;
        for (__Constructor_Index__ = 0U;
             __Constructor_Index__ < __Declaration__->__As__.__Enum__.__Count__;
             ++__Constructor_Index__)
        {
            /* References the constructor. */
            __Ast_Enum_Constructor__ *__Constructor__ =
                &__Declaration__->__As__.__Enum__.__Constructors__[__Constructor_Index__];
            for (__Index__ = 0U; __Index__ < __Constructor__->__Payload_Count__; ++__Index__)
            {
                if (__Memory_Type_Contains_View_Recursive__(
                        __Context__,
                        __Constructor__->__Payload_Slots__[__Index__].__Type__,
                        __Depth__ + 1U))
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Returns the memory type contains view. */
int __Memory_Type_Contains_View__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__)
{
    return __Memory_Type_Contains_View_Recursive__(__Context__, __Type__, 0U);
}

/* Returns the memory vector growth policy view. */
const __Memory_Vector_Growth_Policy__ *__Memory_Vector_Growth_Policy_View__(void)
{
    /* Stores the policy. */
    static const __Memory_Vector_Growth_Policy__ __Policy__ = {__Memory_Vector_Minimum_Capacity__,
                                                               2U};
    return &__Policy__;
}

/* Returns the memory element bytes. */
int __Memory_Element_Bytes__(size_t __Element_Size__,
                             size_t __Element_Count__,
                             size_t *__Out_Bytes__)
{
    if (__Out_Bytes__ == NULL)
        return 0;
    if (__Element_Size__ != 0U && __Element_Count__ > SIZE_MAX / __Element_Size__)
        return 0;
    *__Out_Bytes__ = __Element_Size__ * __Element_Count__;
    return 1;
}

/* Returns the memory vector initial capacity. */
size_t __Memory_Vector_Initial_Capacity__(size_t __Length__)
{
    /* Stores the capacity. */
    size_t __Capacity__;
    if (__Length__ == 0U)
        return 0U;
    __Capacity__ = __Memory_Vector_Minimum_Capacity__;
    while (__Capacity__ < __Length__)
    {
        if (__Capacity__ > SIZE_MAX / 2U)
            return __Length__;
        __Capacity__ *= 2U;
    }
    return __Capacity__;
}

/* Grows the memory vector capacity. */
int __Memory_Vector_Grow_Capacity__(size_t __Current_Capacity__,
                                    size_t __Required_Capacity__,
                                    size_t *__Out_Capacity__)
{
    /* Stores the capacity. */
    size_t __Capacity__;
    if (__Out_Capacity__ == NULL)
        return 0;
    if (__Required_Capacity__ <= __Current_Capacity__)
    {
        *__Out_Capacity__ = __Current_Capacity__;
        return 1;
    }
    __Capacity__ =
        __Current_Capacity__ == 0U ? __Memory_Vector_Minimum_Capacity__ : __Current_Capacity__;
    while (__Capacity__ < __Required_Capacity__)
    {
        if (__Capacity__ > SIZE_MAX / 2U)
        {
            __Capacity__ = __Required_Capacity__;
            break;
        }
        __Capacity__ *= 2U;
    }
    if (__Capacity__ < __Required_Capacity__)
        return 0;
    *__Out_Capacity__ = __Capacity__;
    return 1;
}
