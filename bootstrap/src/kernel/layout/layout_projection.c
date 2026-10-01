/* Computes field and tagged-value layout projections. */

#include "kernel/layout/layout.h"
#include "kernel/type/type.h"
#include "kernel/type/tagged.h"
#include "kernel/name/name.h"
#include "support/memory/alignment.h"
#include "support/text/equality.h"

/* Returns the layout struct field. */
int __Layout_Struct_Field__(__Semantic_Context__ *__Context__,
                            __Semantic_Type_Entry__ *__Type__,
                            __Text_Slice__ __Field_Name__,
                            size_t *__Out_Offset__,
                            __Ast_Type__ **__Out_Type__)
{
    /* Stores the offset. */
    size_t __Offset__ = 0U;
    /* Tracks the index. */
    size_t __Index__ = 0U;
    /* References the declaration. */
    __Ast_Type_Declaration__ *__Declaration__ = __Type__ == NULL ? NULL : __Type__->__Declaration__;
    if (__Declaration__ == NULL || __Declaration__->__Kind__ != __Ast_Type_Decl_Struct__)
    {
        return 0;
    }
    for (__Index__ = 0U; __Index__ < __Declaration__->__As__.__Struct__.__Count__; ++__Index__)
    {
        /* References the field. */
        __Ast_Struct_Field__ *__Field__ = &__Declaration__->__As__.__Struct__.__Fields__[__Index__];
        /* Stores the field size. */
        size_t __Field_Size__ = 0U;
        /* Stores the field alignment. */
        size_t __Field_Alignment__ = 1U;
        if (!__Layout_Type__(
                __Context__, __Field__->__Slot__.__Type__, &__Field_Size__, &__Field_Alignment__))
        {
            return 0;
        }
        __Offset__ = __Align_Up__(__Offset__, __Field_Alignment__);
        if (__Text_Slice_Equals__(__Field__->__Name__, __Field_Name__))
        {
            *__Out_Offset__ = __Offset__;
            *__Out_Type__ = __Field__->__Slot__.__Type__;
            return 1;
        }
        __Offset__ += __Field_Size__;
    }
    return 0;
}

/* Projects canonical tagged layout into tag and payload offsets. */
static int __Layout_Tagged_Constructor_View__(__Semantic_Context__ *__Context__,
                                              __Ast_Type__ *__Type__,
                                              size_t __Constructor_Index__,
                                              __Ast_Slot__ **__Out_Slots__,
                                              size_t *__Out_Count__,
                                              __Ast_Slot__ *__Builtin_Slot__)
{
    /* References the base. */
    __Ast_Type__ *__Base__ = __Type_Unwrap_Mutable__(__Type__);
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;

    if (__Base__ == NULL || !__Type_Resolve__(__Context__, __Base__, &__Resolved__))
    {
        return 0;
    }
    if (__Type_Is_Builtin_Tagged__(__Base__))
    {
        /* Stores the constructor. */
        __Type_Tagged_Constructor__ __Constructor__;
        if (!__Type_Tagged_Constructor_At__(__Base__, __Constructor_Index__, &__Constructor__))
        {
            return 0;
        }
        *__Out_Count__ = __Constructor__.__Payload_Count__;
        if (__Constructor__.__Payload_Count__ == 0U)
        {
            *__Out_Slots__ = NULL;
        }
        else
        {
            __Builtin_Slot__->__Type__ = __Constructor__.__Payload_Type__;
            *__Out_Slots__ = __Builtin_Slot__;
        }
        return 1;
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Enum__ && __Resolved__.__Named__ != NULL &&
        __Resolved__.__Named__->__Declaration__ != NULL &&
        __Resolved__.__Named__->__Declaration__->__Kind__ == __Ast_Type_Decl_Enum__ &&
        __Constructor_Index__ < __Resolved__.__Named__->__Declaration__->__As__.__Enum__.__Count__)
    {
        /* References the constructor. */
        __Ast_Enum_Constructor__ *__Constructor__ =
            &__Resolved__.__Named__->__Declaration__->__As__.__Enum__
                 .__Constructors__[__Constructor_Index__];
        *__Out_Slots__ = __Constructor__->__Payload_Slots__;
        *__Out_Count__ = __Constructor__->__Payload_Count__;
        return 1;
    }
    return 0;
}

/* Returns the layout tagged storage. */
int __Layout_Tagged_Storage__(__Semantic_Context__ *__Context__,
                              __Ast_Type__ *__Type__,
                              size_t *__Out_Tag_Size__,
                              size_t *__Out_Payload_Offset__,
                              size_t *__Out_Payload_Size__,
                              size_t *__Out_Alignment__)
{
    /* References the base. */
    __Ast_Type__ *__Base__ = __Type_Unwrap_Mutable__(__Type__);
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;
    /* Stores the constructor count. */
    size_t __Constructor_Count__ = 0U;
    /* Stores the payload max. */
    size_t __Payload_Max__ = 0U;
    /* Stores the payload alignment. */
    size_t __Payload_Alignment__ = 1U;
    /* Tracks the constructor index. */
    size_t __Constructor_Index__;
    /* Stores the canonical size. */
    size_t __Canonical_Size__ = 0U;
    /* Stores the canonical alignment. */
    size_t __Canonical_Alignment__ = 1U;
    /* Stores the projected size. */
    size_t __Projected_Size__;
    /* Stores the projected alignment. */
    size_t __Projected_Alignment__;
    /* Stores the payload offset. */
    size_t __Payload_Offset__;

    if (__Base__ == NULL || !__Type_Resolve__(__Context__, __Base__, &__Resolved__))
    {
        return 0;
    }
    if (__Type_Is_Builtin_Tagged__(__Base__))
    {
        __Constructor_Count__ = __Type_Tagged_Constructor_Count__(__Base__);
    }
    else if (__Resolved__.__Kind__ == __Resolved_Type_Enum__ && __Resolved__.__Named__ != NULL &&
             __Resolved__.__Named__->__Declaration__ != NULL)
    {
        __Constructor_Count__ = __Resolved__.__Named__->__Declaration__->__As__.__Enum__.__Count__;
    }
    else
    {
        return 0;
    }

    for (__Constructor_Index__ = 0U; __Constructor_Index__ < __Constructor_Count__;
         ++__Constructor_Index__)
    {
        /* References the slots. */
        __Ast_Slot__ *__Slots__ = NULL;
        /* Stores the builtin slot. */
        __Ast_Slot__ __Builtin_Slot__ = {0};
        /* Stores the slot count. */
        size_t __Slot_Count__ = 0U;
        /* Stores the payload size. */
        size_t __Payload_Size__ = 0U;
        /* Stores the constructor alignment. */
        size_t __Constructor_Alignment__ = 1U;
        /* Tracks the slot index. */
        size_t __Slot_Index__;
        if (!__Layout_Tagged_Constructor_View__(__Context__,
                                                __Base__,
                                                __Constructor_Index__,
                                                &__Slots__,
                                                &__Slot_Count__,
                                                &__Builtin_Slot__))
        {
            return 0;
        }
        for (__Slot_Index__ = 0U; __Slot_Index__ < __Slot_Count__; ++__Slot_Index__)
        {
            /* Stores the field size. */
            size_t __Field_Size__ = 0U;
            /* Stores the field alignment. */
            size_t __Field_Alignment__ = 1U;
            if (!__Layout_Type__(__Context__,
                                 __Slots__[__Slot_Index__].__Type__,
                                 &__Field_Size__,
                                 &__Field_Alignment__))
            {
                return 0;
            }
            __Payload_Size__ = __Align_Up__(__Payload_Size__, __Field_Alignment__);
            __Payload_Size__ += __Field_Size__;
            if (__Field_Alignment__ > __Constructor_Alignment__)
            {
                __Constructor_Alignment__ = __Field_Alignment__;
            }
        }
        __Payload_Size__ = __Align_Up__(__Payload_Size__, __Constructor_Alignment__);
        if (__Payload_Size__ > __Payload_Max__)
        {
            __Payload_Max__ = __Payload_Size__;
        }
        if (__Constructor_Alignment__ > __Payload_Alignment__)
        {
            __Payload_Alignment__ = __Constructor_Alignment__;
        }
    }

    __Payload_Offset__ = __Align_Up__(8U, __Payload_Alignment__);
    __Projected_Alignment__ = __Payload_Alignment__ > 8U ? __Payload_Alignment__ : 8U;
    __Projected_Size__ =
        __Align_Up__(__Payload_Offset__ + __Payload_Max__, __Projected_Alignment__);
    if (!__Layout_Type__(__Context__, __Base__, &__Canonical_Size__, &__Canonical_Alignment__) ||
        __Canonical_Size__ != __Projected_Size__ ||
        __Canonical_Alignment__ != __Projected_Alignment__)
    {
        return 0;
    }

    if (__Out_Tag_Size__ != NULL)
        *__Out_Tag_Size__ = 8U;
    if (__Out_Payload_Offset__ != NULL)
        *__Out_Payload_Offset__ = __Payload_Offset__;
    if (__Out_Payload_Size__ != NULL)
        *__Out_Payload_Size__ = __Payload_Max__;
    if (__Out_Alignment__ != NULL)
        *__Out_Alignment__ = __Projected_Alignment__;
    return 1;
}

/* Returns the layout tagged payload. */
int __Layout_Tagged_Payload__(__Semantic_Context__ *__Context__,
                              __Ast_Type__ *__Type__,
                              size_t __Constructor_Index__,
                              size_t __Payload_Index__,
                              size_t *__Out_Offset__,
                              __Ast_Type__ **__Out_Type__)
{
    /* References the slots. */
    __Ast_Slot__ *__Slots__ = NULL;
    /* Stores the builtin slot. */
    __Ast_Slot__ __Builtin_Slot__ = {0};
    /* Stores the slot count. */
    size_t __Slot_Count__ = 0U;
    /* Stores the payload base. */
    size_t __Payload_Base__ = 0U;
    /* Stores the offset. */
    size_t __Offset__ = 0U;
    /* Tracks the index. */
    size_t __Index__;

    if (!__Layout_Tagged_Storage__(__Context__, __Type__, NULL, &__Payload_Base__, NULL, NULL) ||
        !__Layout_Tagged_Constructor_View__(__Context__,
                                            __Type__,
                                            __Constructor_Index__,
                                            &__Slots__,
                                            &__Slot_Count__,
                                            &__Builtin_Slot__) ||
        __Payload_Index__ >= __Slot_Count__)
    {
        return 0;
    }
    for (__Index__ = 0U; __Index__ <= __Payload_Index__; ++__Index__)
    {
        /* Stores the field size. */
        size_t __Field_Size__ = 0U;
        /* Stores the field alignment. */
        size_t __Field_Alignment__ = 1U;
        if (!__Layout_Type__(
                __Context__, __Slots__[__Index__].__Type__, &__Field_Size__, &__Field_Alignment__))
        {
            return 0;
        }
        __Offset__ = __Align_Up__(__Offset__, __Field_Alignment__);
        if (__Index__ == __Payload_Index__)
        {
            if (__Out_Offset__ != NULL)
                *__Out_Offset__ = __Payload_Base__ + __Offset__;
            if (__Out_Type__ != NULL)
                *__Out_Type__ = __Slots__[__Index__].__Type__;
            return 1;
        }
        __Offset__ += __Field_Size__;
    }
    return 0;
}

/* Returns the layout string data offset. */
size_t __Layout_String_Data_Offset__(void)
{
    return 0U;
}
/* Returns the layout string length offset. */
size_t __Layout_String_Length_Offset__(void)
{
    return 8U;
}
/* Returns the layout string capacity offset. */
size_t __Layout_String_Capacity_Offset__(void)
{
    return 16U;
}
/* Returns the layout vector data offset. */
size_t __Layout_Vector_Data_Offset__(void)
{
    return 0U;
}
/* Returns the layout vector length offset. */
size_t __Layout_Vector_Length_Offset__(void)
{
    return 8U;
}
/* Returns the layout vector capacity offset. */
size_t __Layout_Vector_Capacity_Offset__(void)
{
    return 16U;
}
