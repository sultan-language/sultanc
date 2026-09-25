/* Resolves named types within semantic context. */

#include "kernel/name/name.h"
#include "semantic/diagnostic.h"
#include "semantic/type_named.h"

/* Returns the semantic type owner unit. */
const __Program_Unit__ *__Semantic_Type_Owner_Unit__(const __Semantic_Context__ *__Context__,
                                                     const __Ast_Type__ *__Type__)
{
    /* Tracks the index. */
    size_t __Index__;

    if (__Context__ == NULL || __Type__ == NULL)
    {
        return NULL;
    }
    for (__Index__ = 0U; __Index__ < __Context__->__Type_Owners__.__Count__; ++__Index__)
    {
        /* References the fact. */
        const __Semantic_Type_Owner_Fact__ *__Fact__ =
            (const __Semantic_Type_Owner_Fact__ *)__Vector_At_Const__(&__Context__->__Type_Owners__,
                                                                      __Index__);
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
    *__Out_Entry__ = __Entry__;
    return 1;
}
