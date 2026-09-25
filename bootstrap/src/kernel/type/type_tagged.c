/* Queries tagged and enum type constructors. */

#include "kernel/type/tagged.h"
#include "core/source.h"
#include "kernel/type/type_contextual.generated.h"

/* Maps option/result constructors to canonical tags and payloads. */
static int
__Type_Tagged_Fill__(__Ast_Type__ *__Type__, size_t __Tag__, __Type_Tagged_Constructor__ *__Out__)
{
    /* References the base. */
    __Ast_Type__ *__Base__ = __Type_Unwrap_Mutable__(__Type__);

    if (__Base__ == NULL || __Out__ == NULL)
    {
        return 0;
    }

    __Out__->__Tag__ = __Tag__;
    __Out__->__Payload_Count__ = 0U;
    __Out__->__Payload_Type__ = NULL;

    if (__Base__->__Kind__ == __Ast_Type_Option__)
    {
        if (__Tag__ > 1U)
        {
            return 0;
        }
        if (__Tag__ == 1U)
        {
            __Out__->__Payload_Count__ = 1U;
            __Out__->__Payload_Type__ = __Base__->__As__.__Inner__;
        }
        return 1;
    }

    if (__Base__->__Kind__ == __Ast_Type_Result__)
    {
        if (__Tag__ > 1U)
        {
            return 0;
        }
        __Out__->__Payload_Count__ = 1U;
        __Out__->__Payload_Type__ = __Tag__ == 0U ? __Base__->__As__.__Result__.__Ok__
                                                  : __Base__->__As__.__Result__.__Error__;
        return 1;
    }

    return 0;
}

/* Checks whether the type is builtin tagged. */
int __Type_Is_Builtin_Tagged__(__Ast_Type__ *__Type__)
{
    /* References the base. */
    __Ast_Type__ *__Base__ = __Type_Unwrap_Mutable__(__Type__);
    return __Base__ != NULL &&
           (__Base__->__Kind__ == __Ast_Type_Option__ || __Base__->__Kind__ == __Ast_Type_Result__);
}

/* Returns the type tagged constructor count. */
size_t __Type_Tagged_Constructor_Count__(__Ast_Type__ *__Type__)
{
    return __Type_Is_Builtin_Tagged__(__Type__) ? 2U : 0U;
}

/* Returns the type tagged constructor at. */
int __Type_Tagged_Constructor_At__(__Ast_Type__ *__Type__,
                                   size_t __Tag__,
                                   __Type_Tagged_Constructor__ *__Out_Constructor__)
{
    return __Type_Tagged_Fill__(__Type__, __Tag__, __Out_Constructor__);
}

/* Finds the type tagged constructor. */
int __Type_Tagged_Find_Constructor__(__Ast_Type__ *__Type__,
                                     __Text_Slice__ __Name__,
                                     __Type_Tagged_Constructor__ *__Out_Constructor__)
{
    /* References the base. */
    __Ast_Type__ *__Base__ = __Type_Unwrap_Mutable__(__Type__);

    if (__Base__ == NULL)
    {
        return 0;
    }

    if (__Base__->__Kind__ == __Ast_Type_Option__)
    {
        if (__Text_Equals_Cstr__(__Name__, "none") || __Text_Equals_Cstr__(__Name__, "None") ||
            __Text_Equals_Cstr__(__Name__, "لا_شيء"))
        {
            return __Type_Tagged_Fill__(__Type__, 0U, __Out_Constructor__);
        }
        if (__Text_Equals_Cstr__(__Name__, "some") || __Text_Equals_Cstr__(__Name__, "Some") ||
            __Text_Equals_Cstr__(__Name__, "بعض"))
        {
            return __Type_Tagged_Fill__(__Type__, 1U, __Out_Constructor__);
        }
    }
    else if (__Base__->__Kind__ == __Ast_Type_Result__)
    {
        if (__Text_Equals_Cstr__(__Name__, "ok") || __Text_Equals_Cstr__(__Name__, "Ok") ||
            __Text_Equals_Cstr__(__Name__, "نجح"))
        {
            return __Type_Tagged_Fill__(__Type__, 0U, __Out_Constructor__);
        }
        if (__Text_Equals_Cstr__(__Name__, "err") || __Text_Equals_Cstr__(__Name__, "Err") ||
            __Text_Equals_Cstr__(__Name__, "خطأ"))
        {
            return __Type_Tagged_Fill__(__Type__, 1U, __Out_Constructor__);
        }
    }

    return 0;
}

/* Checks the type tagged name. */
int __Type_Tagged_Name_Matches__(__Ast_Type__ *__Type__, __Text_Slice__ __Name__)
{
    /* References the base. */
    __Ast_Type__ *__Base__ = __Type_Unwrap_Mutable__(__Type__);
    if (__Base__ == NULL)
    {
        return 0;
    }
    if (__Base__->__Kind__ == __Ast_Type_Option__)
    {
        return __Type_Contextual_Name_Is_OPTION__(__Name__);
    }
    if (__Base__->__Kind__ == __Ast_Type_Result__)
    {
        return __Type_Contextual_Name_Is_RESULT__(__Name__);
    }
    return 0;
}
