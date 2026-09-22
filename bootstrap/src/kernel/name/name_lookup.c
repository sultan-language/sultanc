#include "kernel/name/name.h"
#include "frontend/identifier_identity.h"

static int __Name_Function_Entry_Matches__(const __Semantic_Function_Entry__ *__Entry__,
                                           __Text_Slice__ __Name__)
{
    return __Entry__ != NULL && __Identifier_Identity_Equals__(__Entry__->__Name__, __Name__);
}

int __Name_Resolve_Module_Alias_Name__(const __Program_Unit__ *__Unit__,
                                       __Text_Slice__ __Name__,
                                       __Text_Slice__ *__Out_Name__)
{
    size_t __Depth__;
    __Text_Slice__ __Resolved__ = __Name__;

    if (__Out_Name__ == NULL)
    {
        return 0;
    }
    if (__Unit__ == NULL)
    {
        *__Out_Name__ = __Resolved__;
        return 1;
    }
    for (__Depth__ = 0U; __Depth__ <= __Unit__->__Parse__.__Module__.__Item_Count__; ++__Depth__)
    {
        size_t __Index__;
        int __Found_Alias__ = 0;

        for (__Index__ = 0U; __Index__ < __Unit__->__Parse__.__Module__.__Item_Count__; ++__Index__)
        {
            const __Ast_Module_Item__ *__Item__ =
                __Unit__->__Parse__.__Module__.__Items__[__Index__];

            if (__Item__ == NULL || __Item__->__Kind__ != __Ast_Module_Item_Alias__ ||
                __Item__->__As__.__Alias__.__Destination__.__Parent__.__Length__ != 0U)
            {
                continue;
            }
            if (!__Identifier_Identity_Equals__(__Item__->__As__.__Alias__.__Destination__.__Name__,
                                                __Resolved__))
            {
                continue;
            }
            __Resolved__ = __Item__->__As__.__Alias__.__Target__.__Name__;
            __Found_Alias__ = 1;
            break;
        }
        if (!__Found_Alias__)
        {
            *__Out_Name__ = __Resolved__;
            return 1;
        }
    }
    return 0;
}

static int __Name_Unit_Imports__(const __Semantic_Context__ *__Context__,
                                 const __Program_Unit__ *__From_Unit__,
                                 const __Program_Unit__ *__Target_Unit__)
{
    size_t __Index__;

    if (__Context__ == NULL || __From_Unit__ == NULL || __Target_Unit__ == NULL)
    {
        return 0;
    }
    for (__Index__ = 0U; __Index__ < __From_Unit__->__Imported_Unit_Indexes__.__Count__;
         ++__Index__)
    {
        const size_t *__Imported_Index__ = (const size_t *)__Vector_At_Const__(
            &__From_Unit__->__Imported_Unit_Indexes__, __Index__);
        const __Program_Unit__ *__Imported_Unit__;

        if (__Imported_Index__ == NULL)
        {
            continue;
        }
        __Imported_Unit__ =
            __Program_Unit_At_Const__(__Context__->__Program__, *__Imported_Index__);
        if (__Imported_Unit__ == __Target_Unit__)
        {
            return 1;
        }
    }
    return 0;
}

__Name_Lookup_Status__ __Name_Resolve_Type__(__Semantic_Context__ *__Context__,
                                             const __Program_Unit__ *__From_Unit__,
                                             __Text_Slice__ __Name__,
                                             __Semantic_Type_Entry__ **__Out_Entry__)
{
    size_t __Index__;
    __Semantic_Type_Entry__ *__Imported_Match__ = NULL;
    int __Private_Match__ = 0;

    if (__Out_Entry__ != NULL)
    {
        *__Out_Entry__ = NULL;
    }
    if (__Context__ == NULL)
    {
        return __Name_Lookup_Missing__;
    }

    for (__Index__ = 0U; __Index__ < __Context__->__Types__.__Count__; ++__Index__)
    {
        __Semantic_Type_Entry__ *__Entry__ =
            (__Semantic_Type_Entry__ *)__Vector_At__(&__Context__->__Types__, __Index__);
        __Text_Slice__ __Resolved_Name__;

        if (__Entry__ == NULL ||
            !__Name_Resolve_Module_Alias_Name__(
                __Entry__->__Unit__, __Name__, &__Resolved_Name__) ||
            !__Identifier_Identity_Equals__(__Entry__->__Name__, __Resolved_Name__))
        {
            continue;
        }
        if (__From_Unit__ == NULL || __Entry__->__Unit__ == __From_Unit__)
        {
            if (__Out_Entry__ != NULL)
            {
                *__Out_Entry__ = __Entry__;
            }
            return __Name_Lookup_Found__;
        }
        if (!__Name_Unit_Imports__(__Context__, __From_Unit__, __Entry__->__Unit__))
        {
            continue;
        }
        if (!__Entry__->__Public__)
        {
            __Private_Match__ = 1;
            continue;
        }
        if (__Imported_Match__ != NULL && __Imported_Match__ != __Entry__)
        {
            return __Name_Lookup_Ambiguous__;
        }
        __Imported_Match__ = __Entry__;
    }

    if (__Imported_Match__ != NULL)
    {
        if (__Out_Entry__ != NULL)
        {
            *__Out_Entry__ = __Imported_Match__;
        }
        return __Name_Lookup_Found__;
    }
    return __Private_Match__ ? __Name_Lookup_Private__ : __Name_Lookup_Missing__;
}

__Name_Lookup_Status__ __Name_Resolve_Function__(__Semantic_Context__ *__Context__,
                                                 const __Program_Unit__ *__From_Unit__,
                                                 __Text_Slice__ __Name__,
                                                 __Semantic_Function_Entry__ **__Out_Entry__)
{
    size_t __Index__;
    __Semantic_Function_Entry__ *__Imported_Match__ = NULL;
    int __Private_Match__ = 0;

    if (__Out_Entry__ != NULL)
    {
        *__Out_Entry__ = NULL;
    }
    if (__Context__ == NULL)
    {
        return __Name_Lookup_Missing__;
    }

    for (__Index__ = 0U; __Index__ < __Context__->__Functions__.__Count__; ++__Index__)
    {
        __Semantic_Function_Entry__ *__Entry__ =
            (__Semantic_Function_Entry__ *)__Vector_At__(&__Context__->__Functions__, __Index__);
        __Text_Slice__ __Resolved_Name__;

        if (__Entry__ == NULL ||
            !__Name_Resolve_Module_Alias_Name__(
                __Entry__->__Unit__, __Name__, &__Resolved_Name__) ||
            !__Name_Function_Entry_Matches__(__Entry__, __Resolved_Name__))
        {
            continue;
        }
        if (__From_Unit__ == NULL || __Entry__->__Unit__ == __From_Unit__)
        {
            if (__Out_Entry__ != NULL)
            {
                *__Out_Entry__ = __Entry__;
            }
            return __Name_Lookup_Found__;
        }
        if (!__Name_Unit_Imports__(__Context__, __From_Unit__, __Entry__->__Unit__))
        {
            continue;
        }
        if (!__Entry__->__Public__)
        {
            __Private_Match__ = 1;
            continue;
        }
        if (__Imported_Match__ != NULL && __Imported_Match__ != __Entry__)
        {
            return __Name_Lookup_Ambiguous__;
        }
        __Imported_Match__ = __Entry__;
    }

    if (__Imported_Match__ != NULL)
    {
        if (__Out_Entry__ != NULL)
        {
            *__Out_Entry__ = __Imported_Match__;
        }
        return __Name_Lookup_Found__;
    }
    return __Private_Match__ ? __Name_Lookup_Private__ : __Name_Lookup_Missing__;
}

__Semantic_Type_Entry__ *__Name_Find_Type__(__Semantic_Context__ *__Context__,
                                            __Text_Slice__ __Name__)
{
    __Semantic_Type_Entry__ *__Entry__ = NULL;
    return __Name_Resolve_Type__(__Context__,
                                 __Context__ == NULL ? NULL : __Context__->__Active_Unit__,
                                 __Name__,
                                 &__Entry__) == __Name_Lookup_Found__
               ? __Entry__
               : NULL;
}

__Semantic_Function_Entry__ *__Name_Find_Function__(__Semantic_Context__ *__Context__,
                                                    __Text_Slice__ __Name__)
{
    __Semantic_Function_Entry__ *__Entry__ = NULL;
    return __Name_Resolve_Function__(__Context__,
                                     __Context__ == NULL ? NULL : __Context__->__Active_Unit__,
                                     __Name__,
                                     &__Entry__) == __Name_Lookup_Found__
               ? __Entry__
               : NULL;
}

__Error_Id__ __Name_Lookup_Error_Id__(__Name_Lookup_Status__ __Status__)
{
    if (__Status__ == __Name_Lookup_Private__)
    {
        return __E0309_Private_Declaration__;
    }
    if (__Status__ == __Name_Lookup_Ambiguous__)
    {
        return __E0310_Ambiguous_Import__;
    }
    return __E0300_Unknown_Name__;
}
