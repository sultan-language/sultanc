/* Collects global semantic declarations. */

#include "semantic/context_collection.h"
#include "frontend/identifier_identity.h"
#include "kernel/name/name.h"
#include "semantic/diagnostic.h"
#include "semantic/context.h"

#include <string.h>

/* Returns the semantic record type owner. */
static int __Semantic_Record_Type_Owner__(__Semantic_Context__ *__Context__,
                                          __Ast_Type__ *__Type__,
                                          const __Program_Unit__ *__Unit__)
{
    if (__Type__ == NULL)
    {
        return 1;
    }
    if (__Type__->__Kind__ == __Ast_Type_Named__)
    {
        /* Stores the fact. */
        __Semantic_Type_Owner_Fact__ __Fact__;
        __Fact__.__Type__ = __Type__;
        __Fact__.__Unit__ = __Unit__;
        return __Vector_Push__(&__Context__->__Type_Owners__, &__Fact__) != NULL;
    }
    if (__Type__->__Kind__ == __Ast_Type_Reference__ || __Type__->__Kind__ == __Ast_Type_Vector__ ||
        __Type__->__Kind__ == __Ast_Type_Box__ || __Type__->__Kind__ == __Ast_Type_Option__ ||
        __Type__->__Kind__ == __Ast_Type_Mutable__)
    {
        return __Semantic_Record_Type_Owner__(__Context__, __Type__->__As__.__Inner__, __Unit__);
    }
    if (__Type__->__Kind__ == __Ast_Type_Result__)
    {
        return __Semantic_Record_Type_Owner__(
                   __Context__, __Type__->__As__.__Result__.__Ok__, __Unit__) &&
               __Semantic_Record_Type_Owner__(
                   __Context__, __Type__->__As__.__Result__.__Error__, __Unit__);
    }
    return 1;
}

/* Returns the semantic record type declaration owners. */
static int __Semantic_Record_Type_Declaration_Owners__(__Semantic_Context__ *__Context__,
                                                       __Ast_Type_Declaration__ *__Declaration__,
                                                       const __Program_Unit__ *__Unit__)
{
    /* Tracks the index. */
    size_t __Index__;

    if (__Declaration__ == NULL)
    {
        return 1;
    }
    if (__Declaration__->__Kind__ == __Ast_Type_Decl_Struct__)
    {
        for (__Index__ = 0U; __Index__ < __Declaration__->__As__.__Struct__.__Count__; ++__Index__)
        {
            if (!__Semantic_Record_Type_Owner__(
                    __Context__,
                    __Declaration__->__As__.__Struct__.__Fields__[__Index__].__Slot__.__Type__,
                    __Unit__))
            {
                return 0;
            }
        }
        return 1;
    }
    for (__Index__ = 0U; __Index__ < __Declaration__->__As__.__Enum__.__Count__; ++__Index__)
    {
        /* References the constructor. */
        __Ast_Enum_Constructor__ *__Constructor__ =
            &__Declaration__->__As__.__Enum__.__Constructors__[__Index__];
        /* Tracks the payload index. */
        size_t __Payload_Index__;

        for (__Payload_Index__ = 0U; __Payload_Index__ < __Constructor__->__Payload_Count__;
             ++__Payload_Index__)
        {
            if (!__Semantic_Record_Type_Owner__(
                    __Context__,
                    __Constructor__->__Payload_Slots__[__Payload_Index__].__Type__,
                    __Unit__))
            {
                return 0;
            }
        }
    }
    return 1;
}

/* Returns the semantic record function signature owners. */
static int __Semantic_Record_Function_Signature_Owners__(__Semantic_Context__ *__Context__,
                                                         __Ast_Function__ *__Function__,
                                                         const __Program_Unit__ *__Unit__)
{
    /* Tracks the index. */
    size_t __Index__;

    if (__Function__ == NULL)
    {
        return 1;
    }
    for (__Index__ = 0U; __Index__ < __Function__->__Parameter_Count__; ++__Index__)
    {
        if (!__Semantic_Record_Type_Owner__(
                __Context__, __Function__->__Parameters__[__Index__].__Slot__.__Type__, __Unit__))
        {
            return 0;
        }
    }
    return __Semantic_Record_Type_Owner__(__Context__, __Function__->__Output__.__Type__, __Unit__);
}

/* Compares the semantic function name. */
static int __Semantic_Function_Name_Equals__(const __Semantic_Function_Entry__ *__Entry__,
                                             __Text_Slice__ __Name__)
{
    return __Entry__ != NULL && __Identifier_Identity_Equals__(__Entry__->__Name__, __Name__);
}

/* Returns the semantic unit name exists. */
static int __Semantic_Unit_Name_Exists__(__Semantic_Context__ *__Context__,
                                         const __Program_Unit__ *__Unit__,
                                         __Text_Slice__ __Name__)
{
    /* Tracks the index. */
    size_t __Index__;

    for (__Index__ = 0U; __Index__ < __Context__->__Types__.__Count__; ++__Index__)
    {
        /* References the entry. */
        const __Semantic_Type_Entry__ *__Entry__ =
            (const __Semantic_Type_Entry__ *)__Vector_At_Const__(&__Context__->__Types__,
                                                                 __Index__);
        if (__Entry__ != NULL && __Entry__->__Unit__ == __Unit__ &&
            __Identifier_Identity_Equals__(__Entry__->__Name__, __Name__))
        {
            return 1;
        }
    }
    for (__Index__ = 0U; __Index__ < __Context__->__Functions__.__Count__; ++__Index__)
    {
        /* References the entry. */
        const __Semantic_Function_Entry__ *__Entry__ =
            (const __Semantic_Function_Entry__ *)__Vector_At_Const__(&__Context__->__Functions__,
                                                                     __Index__);
        if (__Entry__ != NULL && __Entry__->__Unit__ == __Unit__ &&
            __Semantic_Function_Name_Equals__(__Entry__, __Name__))
        {
            return 1;
        }
    }
    return 0;
}

/* Collects the semantic globals. */
int __Semantic_Collect_Globals__(__Semantic_Context__ *__Context__)
{
    /* Tracks the unit index. */
    size_t __Unit_Index__;
    /* Tracks the function index. */
    size_t __Function_Index__ = 0U;
    /* References the root unit. */
    const __Program_Unit__ *__Root_Unit__ = __Program_Unit_At_Const__(__Context__->__Program__, 0U);

    for (__Unit_Index__ = 0U; __Unit_Index__ < __Program_Unit_Count__(__Context__->__Program__);
         ++__Unit_Index__)
    {
        /* References the unit. */
        __Program_Unit__ *__Unit__ = __Program_Unit_At__(__Context__->__Program__, __Unit_Index__);
        /* Tracks the item index. */
        size_t __Item_Index__;

        if (__Unit__ == NULL)
        {
            continue;
        }
        for (__Item_Index__ = 0U; __Item_Index__ < __Unit__->__Parse__.__Module__.__Item_Count__;
             ++__Item_Index__)
        {
            /* References the item. */
            __Ast_Module_Item__ *__Item__ =
                __Unit__->__Parse__.__Module__.__Items__[__Item_Index__];

            if (__Item__ == NULL)
            {
                continue;
            }
            if (__Item__->__Kind__ == __Ast_Module_Item_Type__)
            {
                /* Stores the entry. */
                __Semantic_Type_Entry__ __Entry__;

                if (__Semantic_Unit_Name_Exists__(__Context__, __Unit__, __Item__->__Name__))
                {
                    return __Semantic_Fail__(__Context__,
                                             __E0205_Duplicate_Declaration__,
                                             __Item__->__Header__.__Span__);
                }
                memset(&__Entry__, 0, sizeof(__Entry__));
                __Entry__.__Name__ = __Item__->__Name__;
                __Entry__.__Declaration__ = __Item__->__As__.__Type__;
                __Entry__.__Unit__ = __Unit__;
                __Entry__.__Public__ = __Item__->__Public__;
                if (!__Semantic_Record_Type_Declaration_Owners__(
                        __Context__, __Entry__.__Declaration__, __Entry__.__Unit__))
                {
                    return __Semantic_Fail__(__Context__,
                                             __E1100_Internal_Context_Error__,
                                             __Item__->__Header__.__Span__);
                }
                if (__Vector_Push__(&__Context__->__Types__, &__Entry__) == NULL)
                {
                    return __Semantic_Fail__(__Context__,
                                             __E1100_Internal_Context_Error__,
                                             __Item__->__Header__.__Span__);
                }
            }
            else if (__Item__->__Kind__ == __Ast_Module_Item_Function__)
            {
                /* Stores the entry. */
                __Semantic_Function_Entry__ __Entry__;

                if (__Semantic_Unit_Name_Exists__(__Context__, __Unit__, __Item__->__Name__))
                {
                    return __Semantic_Fail__(
                        __Context__, __E0200_Duplicate_Function__, __Item__->__Header__.__Span__);
                }
                memset(&__Entry__, 0, sizeof(__Entry__));
                __Entry__.__Name__ = __Item__->__Name__;
                __Entry__.__Function__ = __Item__->__As__.__Function__;
                __Entry__.__Unit__ = __Unit__;
                __Entry__.__Public__ = __Item__->__Public__;
                if (!__Semantic_Record_Function_Signature_Owners__(
                        __Context__, __Entry__.__Function__, __Entry__.__Unit__))
                {
                    return __Semantic_Fail__(__Context__,
                                             __E1100_Internal_Context_Error__,
                                             __Item__->__Header__.__Span__);
                }
                __Entry__.__Index__ = SIZE_MAX;
                __Entry__.__Reference_Return_Parameter__ = SIZE_MAX;
                __Entry__.__Composite_View_Return_Parameter__ = SIZE_MAX;
                __Entry__.__Index__ = __Function_Index__++;
                if (__Vector_Push__(&__Context__->__Functions__, &__Entry__) == NULL)
                {
                    return __Semantic_Fail__(__Context__,
                                             __E1100_Internal_Context_Error__,
                                             __Item__->__Header__.__Span__);
                }
            }
        }
    }

    for (__Function_Index__ = 0U; __Function_Index__ < __Context__->__Functions__.__Count__;
         ++__Function_Index__)
    {
        /* References the entry. */
        __Semantic_Function_Entry__ *__Entry__ = (__Semantic_Function_Entry__ *)__Vector_At__(
            &__Context__->__Functions__, __Function_Index__);
        if (__Entry__ != NULL && __Entry__->__Unit__ == __Root_Unit__ &&
            __Semantic_Is_Main_Name__(__Entry__->__Name__))
        {
            __Context__->__Main__ = __Entry__;
            break;
        }
    }
    if (__Context__->__Main__ == NULL)
    {
        /* Stores the span. */
        __Source_Span__ __Span__;
        memset(&__Span__, 0, sizeof(__Span__));
        return __Semantic_Fail__(__Context__, __E0203_Main_Function_Not_Found__, __Span__);
    }
    return 1;
}
