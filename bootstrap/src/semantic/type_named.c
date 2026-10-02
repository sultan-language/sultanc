/* Resolves named types within semantic context. */

#include "kernel/name/name.h"
#include "semantic/diagnostic.h"
#include "semantic/generic.h"
#include "semantic/type_named.h"

#include <stdint.h>

/* Returns the semantic type owner unit. */
const __Program_Unit__ *__Semantic_Type_Owner_Unit__(const __Semantic_Context__ *__Context__,
                                                     const __Ast_Type__ *__Type__)
{
    /* Stores the binary-search lower bound. */
    size_t __Low__ = 0U;
    /* Stores the binary-search upper bound. */
    size_t __High__;
    /* Stores the pointer key. */
    uintptr_t __Key__;

    if (__Context__ == NULL || __Type__ == NULL)
    {
        return NULL;
    }
    __High__ = __Context__->__Type_Owners__.__Count__;
    __Key__ = (uintptr_t)__Type__;
    while (__Low__ < __High__)
    {
        /* Stores the midpoint. */
        size_t __Middle__ = __Low__ + (__High__ - __Low__) / 2U;
        /* References the fact. */
        const __Semantic_Type_Owner_Fact__ *__Fact__ =
            (const __Semantic_Type_Owner_Fact__ *)__Vector_At_Const__(&__Context__->__Type_Owners__,
                                                                      __Middle__);
        /* Stores the midpoint key. */
        uintptr_t __Middle_Key__ = __Fact__ == NULL ? 0U : (uintptr_t)__Fact__->__Type__;

        if (__Middle_Key__ < __Key__)
        {
            __Low__ = __Middle__ + 1U;
        }
        else
        {
            __High__ = __Middle__;
        }
    }
    if (__Low__ < __Context__->__Type_Owners__.__Count__)
    {
        const __Semantic_Type_Owner_Fact__ *__Fact__ =
            (const __Semantic_Type_Owner_Fact__ *)__Vector_At_Const__(&__Context__->__Type_Owners__,
                                                                      __Low__);
        if (__Fact__ != NULL && __Fact__->__Type__ == __Type__)
        {
            return __Fact__->__Unit__;
        }
    }
    /* Generic instances append owner facts after the initial sorted collection. */
    for (__Low__ = 0U; __Low__ < __Context__->__Type_Owners__.__Count__; ++__Low__)
    {
        const __Semantic_Type_Owner_Fact__ *__Fact__ =
            (const __Semantic_Type_Owner_Fact__ *)__Vector_At_Const__(
                &__Context__->__Type_Owners__, __Low__);
        if (__Fact__ != NULL && __Fact__->__Type__ == __Type__)
        {
            return __Fact__->__Unit__;
        }
    }
    return NULL;
}

/* Resolves the semantic named entry. */
int __Semantic_Resolve_Named_Entry__(__Semantic_Context__ *__Context__,
                                     __Ast_Type__ *__Type__,
                                     __Semantic_Type_Entry__ **__Out_Entry__)
{
    /* References the entry. */
    __Semantic_Type_Entry__ *__Entry__ = NULL;
    /* Stores the status. */
    __Name_Lookup_Status__ __Status__;
    /* References the owner unit. */
    const __Program_Unit__ *__Owner_Unit__;

    if (__Type__ == NULL || __Type__->__Kind__ != __Ast_Type_Named__)
    {
        return 0;
    }
    __Owner_Unit__ = __Semantic_Type_Owner_Unit__(__Context__, __Type__);
    if (__Owner_Unit__ == NULL)
    {
        __Owner_Unit__ = __Context__->__Active_Unit__;
    }
    __Status__ = __Name_Resolve_Type__(
        __Context__, __Owner_Unit__, __Type__->__As__.__Named__.__Name__, &__Entry__);
    if (__Status__ != __Name_Lookup_Found__ || __Entry__ == NULL)
    {
        return __Semantic_Fail__(
            __Context__, __Name_Lookup_Error_Id__(__Status__), (__Source_Span__){0});
    }
    return __Semantic_Generic_Resolve_Type_Instance__(
        __Context__, __Entry__, __Type__, __Out_Entry__);
}
