#include "kernel/name/name.h"
#include "frontend/identifier_identity.h"

/* Shared canonical resolver implemented by name_lookup.c in this owner. */
int __Name_Resolve_Module_Alias_Name__(const __Program_Unit__ *__Unit__,
                                       __Text_Slice__ __Name__,
                                       __Text_Slice__ *__Out_Name__);

static int __Name_Find_Direct_Enum_Constructor__(__Semantic_Type_Entry__ *__Type__,
                                                 __Text_Slice__ __Constructor_Name__,
                                                 size_t *__Out_Index__,
                                                 __Ast_Enum_Constructor__ **__Out_Constructor__)
{
    __Ast_Type_Declaration__ *__Declaration__ = __Type__ == NULL ? NULL : __Type__->__Declaration__;
    size_t __Index__ = 0U;

    if (__Declaration__ == NULL || __Declaration__->__Kind__ != __Ast_Type_Decl_Enum__)
    {
        return 0;
    }

    for (__Index__ = 0U; __Index__ < __Declaration__->__As__.__Enum__.__Count__; ++__Index__)
    {
        __Ast_Enum_Constructor__ *__Constructor__ =
            &__Declaration__->__As__.__Enum__.__Constructors__[__Index__];

        if (__Identifier_Identity_Equals__(__Constructor__->__Name__, __Constructor_Name__))
        {
            if (__Out_Index__ != NULL)
            {
                *__Out_Index__ = __Index__;
            }
            if (__Out_Constructor__ != NULL)
            {
                *__Out_Constructor__ = __Constructor__;
            }
            return 1;
        }
    }

    return 0;
}

int __Name_Find_Enum_Constructor__(__Semantic_Type_Entry__ *__Type__,
                                   __Text_Slice__ __Constructor_Name__,
                                   size_t *__Out_Index__,
                                   __Ast_Enum_Constructor__ **__Out_Constructor__)
{
    const __Program_Unit__ *__Unit__ = __Type__ == NULL ? NULL : __Type__->__Unit__;
    __Text_Slice__ __Resolved__ = __Constructor_Name__;
    size_t __Depth__;

    if (__Type__ == NULL)
    {
        return 0;
    }

    for (__Depth__ = 0U;
         __Depth__ <= (__Unit__ == NULL ? 0U : __Unit__->__Parse__.__Module__.__Item_Count__);
         ++__Depth__)
    {
        size_t __Index__;
        int __Found_Alias__ = 0;

        if (__Name_Find_Direct_Enum_Constructor__(
                __Type__, __Resolved__, __Out_Index__, __Out_Constructor__))
        {
            return 1;
        }
        if (__Unit__ == NULL)
        {
            return 0;
        }

        for (__Index__ = 0U; __Index__ < __Unit__->__Parse__.__Module__.__Item_Count__; ++__Index__)
        {
            const __Ast_Module_Item__ *__Item__ =
                __Unit__->__Parse__.__Module__.__Items__[__Index__];
            __Text_Slice__ __Destination_Parent__;
            __Text_Slice__ __Target_Parent__;

            if (__Item__ == NULL || __Item__->__Kind__ != __Ast_Module_Item_Alias__ ||
                __Item__->__As__.__Alias__.__Destination__.__Parent__.__Length__ == 0U ||
                !__Identifier_Identity_Equals__(__Item__->__As__.__Alias__.__Destination__.__Name__,
                                                __Resolved__))
            {
                continue;
            }
            if (!__Name_Resolve_Module_Alias_Name__(
                    __Unit__,
                    __Item__->__As__.__Alias__.__Destination__.__Parent__,
                    &__Destination_Parent__) ||
                !__Identifier_Identity_Equals__(__Destination_Parent__, __Type__->__Name__))
            {
                continue;
            }
            if (!__Name_Resolve_Module_Alias_Name__(
                    __Unit__,
                    __Item__->__As__.__Alias__.__Target__.__Parent__,
                    &__Target_Parent__) ||
                !__Identifier_Identity_Equals__(__Target_Parent__, __Type__->__Name__))
            {
                return 0;
            }
            __Resolved__ = __Item__->__As__.__Alias__.__Target__.__Name__;
            __Found_Alias__ = 1;
            break;
        }

        if (!__Found_Alias__)
        {
            return 0;
        }
    }

    return 0;
}

int __Name_Resolve_Enum_Constructor_Lvalue__(__Semantic_Context__ *__Context__,
                                             const __Ast_Lvalue__ *__Lvalue__,
                                             __Semantic_Type_Entry__ **__Out_Type__,
                                             size_t *__Out_Index__,
                                             __Ast_Enum_Constructor__ **__Out_Constructor__)
{
    const __Ast_Lvalue__ *__Parent__ = NULL;
    __Semantic_Type_Entry__ *__Type__ = NULL;

    if (__Lvalue__ == NULL || __Lvalue__->__Kind__ != __Ast_Lvalue_Field__)
    {
        return 0;
    }

    __Parent__ = __Lvalue__->__As__.__Field__.__Parent__;
    if (__Parent__ == NULL || __Parent__->__Kind__ != __Ast_Lvalue_Base__ ||
        __Parent__->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__)
    {
        return 0;
    }

    __Type__ = __Name_Find_Type__(__Context__, __Parent__->__As__.__Base__.__As__.__Identifier__);
    if (__Type__ == NULL ||
        !__Name_Find_Enum_Constructor__(
            __Type__, __Lvalue__->__As__.__Field__.__Field__, __Out_Index__, __Out_Constructor__))
    {
        return 0;
    }

    if (__Out_Type__ != NULL)
    {
        *__Out_Type__ = __Type__;
    }

    return 1;
}
