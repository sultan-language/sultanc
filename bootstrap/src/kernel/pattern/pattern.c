/* Analyzes patterns, bindings, and coverage. */

#include "kernel/pattern/pattern.h"
#include "kernel/layout/layout.h"
#include "kernel/name/name.h"
#include "kernel/type/type.h"
#include "kernel/type/tagged.h"
#include "support/text/equality.h"

#include <string.h>

/* Defines the pattern node info structure. */
typedef struct
{
    /* Stores the irrefutable. */
    int __Irrefutable__;
    /* Tracks whether the enum constructor is present. */
    int __Has_Enum_Constructor__;
    /* Tracks the enum constructor index. */
    size_t __Enum_Constructor_Index__;
    /* Tracks the enum constructor fully covered state. */
    int __Enum_Constructor_Fully_Covered__;
} __Pattern_Node_Info__;

/* Records a failure for the pattern. */
static int
__Pattern_Fail__(__Pattern_Error__ *__Error__, __Error_Id__ __Id__, __Source_Span__ __Span__)
{
    if (__Error__ != NULL)
    {
        __Error__->__Id__ = __Id__;
        __Error__->__Span__ = __Span__;
    }
    return 0;
}

/* Initializes the pattern analysis. */
void __Pattern_Analysis_Init__(__Pattern_Analysis__ *__Analysis__)
{
    memset(__Analysis__, 0, sizeof(*__Analysis__));
    __Vector_Init__(&__Analysis__->__Bindings__, sizeof(__Pattern_Binding__));
}

/* Releases the pattern analysis. */
void __Pattern_Analysis_Destroy__(__Pattern_Analysis__ *__Analysis__)
{
    if (__Analysis__ != NULL)
    {
        __Vector_Destroy__(&__Analysis__->__Bindings__);
        memset(__Analysis__, 0, sizeof(*__Analysis__));
    }
}

/* Adds the pattern binding. */
static int __Pattern_Add_Binding__(__Pattern_Analysis__ *__Analysis__,
                                   __Text_Slice__ __Name__,
                                   __Ast_Type__ *__Type__,
                                   __Source_Span__ __Span__,
                                   __Pattern_Error__ *__Error__)
{
    /* Tracks the index. */
    size_t __Index__ = 0U;
    /* Stores the binding. */
    __Pattern_Binding__ __Binding__;

    for (__Index__ = 0U; __Index__ < __Analysis__->__Bindings__.__Count__; ++__Index__)
    {
        /* References the existing. */
        const __Pattern_Binding__ *__Existing__ = (const __Pattern_Binding__ *)__Vector_At_Const__(
            &__Analysis__->__Bindings__, __Index__);
        if (__Existing__ != NULL && __Text_Slice_Equals__(__Existing__->__Name__, __Name__))
        {
            return __Pattern_Fail__(__Error__, __E0205_Duplicate_Declaration__, __Span__);
        }
    }

    __Binding__.__Name__ = __Name__;
    __Binding__.__Type__ = __Type__;
    __Binding__.__Span__ = __Span__;
    if (__Vector_Push__(&__Analysis__->__Bindings__, &__Binding__) == NULL)
    {
        return __Pattern_Fail__(__Error__, __E1100_Internal_Context_Error__, __Span__);
    }
    return 1;
}

/* Analyzes the pattern node. */
static int __Pattern_Analyze_Node__(__Semantic_Context__ *__Context__,
                                    __Ast_Type__ *__Expected_Type__,
                                    __Ast_Pattern__ *__Pattern__,
                                    __Pattern_Analysis__ *__Analysis__,
                                    __Pattern_Node_Info__ *__Out_Info__,
                                    __Pattern_Error__ *__Error__)
{
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;
    /* References the base. */
    __Ast_Type__ *__Base__ = __Type_Unwrap_Mutable__(__Expected_Type__);
    /* Tracks the index. */
    size_t __Index__ = 0U;

    memset(__Out_Info__, 0, sizeof(*__Out_Info__));
    if (__Pattern__ == NULL || __Base__ == NULL ||
        !__Type_Resolve__(__Context__, __Expected_Type__, &__Resolved__))
    {
        return __Pattern_Fail__(__Error__,
                                __E0409_Unresolved_Type__,
                                __Pattern__ == NULL ? (__Source_Span__){0}
                                                    : __Pattern__->__Header__.__Span__);
    }

    switch (__Pattern__->__Kind__)
    {
        case __Ast_Pattern_Wildcard__:
            __Out_Info__->__Irrefutable__ = 1;
            return 1;

        case __Ast_Pattern_Binding__:
            if (!__Pattern_Add_Binding__(__Analysis__,
                                         __Pattern__->__As__.__Binding__,
                                         __Expected_Type__,
                                         __Pattern__->__Header__.__Span__,
                                         __Error__))
            {
                return 0;
            }
            __Out_Info__->__Irrefutable__ = 1;
            return 1;

        case __Ast_Pattern_Struct__:
        {
            /* Stores the all irrefutable. */
            int __All_Irrefutable__ = 1;
            if (__Resolved__.__Kind__ != __Resolved_Type_Struct__ || __Resolved__.__Named__ == NULL)
            {
                return __Pattern_Fail__(
                    __Error__, __E0400_Mismatched_Types__, __Pattern__->__Header__.__Span__);
            }

            for (__Index__ = 0U; __Index__ < __Pattern__->__As__.__Struct__.__Field_Count__;
                 ++__Index__)
            {
                /* References the field. */
                __Ast_Struct_Pattern_Field__ *__Field__ =
                    &__Pattern__->__As__.__Struct__.__Fields__[__Index__];
                /* References the field type. */
                __Ast_Type__ *__Field_Type__ = NULL;
                /* Stores the offset. */
                size_t __Offset__ = 0U;
                /* Stores the prior. */
                size_t __Prior__ = 0U;
                /* Stores the child. */
                __Pattern_Node_Info__ __Child__;

                for (__Prior__ = 0U; __Prior__ < __Index__; ++__Prior__)
                {
                    if (__Text_Slice_Equals__(
                            __Pattern__->__As__.__Struct__.__Fields__[__Prior__].__Name__,
                            __Field__->__Name__))
                    {
                        return __Pattern_Fail__(__Error__,
                                                __E0502_Unknown_Record_Member__,
                                                __Pattern__->__Header__.__Span__);
                    }
                }

                if (!__Layout_Struct_Field__(__Context__,
                                             __Resolved__.__Named__,
                                             __Field__->__Name__,
                                             &__Offset__,
                                             &__Field_Type__))
                {
                    return __Pattern_Fail__(__Error__,
                                            __E0502_Unknown_Record_Member__,
                                            __Pattern__->__Header__.__Span__);
                }
                (void)__Offset__;
                if (!__Pattern_Analyze_Node__(__Context__,
                                              __Field_Type__,
                                              __Field__->__Pattern__,
                                              __Analysis__,
                                              &__Child__,
                                              __Error__))
                {
                    return 0;
                }
                __All_Irrefutable__ = __All_Irrefutable__ && __Child__.__Irrefutable__;
            }
            __Out_Info__->__Irrefutable__ = __All_Irrefutable__;
            return 1;
        }

        case __Ast_Pattern_Enum__:
        {
            /* Tracks the constructor index. */
            size_t __Constructor_Index__ = 0U;
            /* Stores the payload count. */
            size_t __Payload_Count__ = 0U;
            /* References the payload type. */
            __Ast_Type__ *__Payload_Type__ = NULL;
            /* Stores the payloads irrefutable. */
            int __Payloads_Irrefutable__ = 1;
            /* Stores the constructor count. */
            size_t __Constructor_Count__ = 0U;

            if (__Type_Is_Builtin_Tagged__(__Base__))
            {
                /* Stores the constructor. */
                __Type_Tagged_Constructor__ __Constructor__;
                if (!__Type_Tagged_Name_Matches__(__Base__,
                                                  __Pattern__->__As__.__Enum__.__Type_Name__) ||
                    !__Type_Tagged_Find_Constructor__(
                        __Base__,
                        __Pattern__->__As__.__Enum__.__Constructor_Name__,
                        &__Constructor__))
                {
                    return __Pattern_Fail__(
                        __Error__, __E0400_Mismatched_Types__, __Pattern__->__Header__.__Span__);
                }
                __Constructor_Index__ = __Constructor__.__Tag__;
                __Payload_Count__ = __Constructor__.__Payload_Count__;
                __Payload_Type__ = __Constructor__.__Payload_Type__;
                __Constructor_Count__ = __Type_Tagged_Constructor_Count__(__Base__);
            }
            else
            {
                /* References the pattern type. */
                __Semantic_Type_Entry__ *__Pattern_Type__ = NULL;
                /* References the constructor. */
                __Ast_Enum_Constructor__ *__Constructor__ = NULL;

                if (__Resolved__.__Kind__ != __Resolved_Type_Enum__ ||
                    __Resolved__.__Named__ == NULL)
                {
                    return __Pattern_Fail__(
                        __Error__, __E0400_Mismatched_Types__, __Pattern__->__Header__.__Span__);
                }

                __Pattern_Type__ =
                    __Name_Find_Type__(__Context__, __Pattern__->__As__.__Enum__.__Type_Name__);
                if (__Pattern_Type__ != __Resolved__.__Named__ ||
                    !__Name_Find_Enum_Constructor__(
                        __Pattern_Type__,
                        __Pattern__->__As__.__Enum__.__Constructor_Name__,
                        &__Constructor_Index__,
                        &__Constructor__))
                {
                    return __Pattern_Fail__(
                        __Error__, __E0400_Mismatched_Types__, __Pattern__->__Header__.__Span__);
                }
                __Payload_Count__ = __Constructor__->__Payload_Count__;
                __Payload_Type__ =
                    __Payload_Count__ == 1U ? __Constructor__->__Payload_Slots__[0].__Type__ : NULL;
                __Constructor_Count__ =
                    __Resolved__.__Named__->__Declaration__->__As__.__Enum__.__Count__;
            }

            if (__Pattern__->__As__.__Enum__.__Payload_Count__ != __Payload_Count__)
            {
                return __Pattern_Fail__(
                    __Error__, __E0400_Mismatched_Types__, __Pattern__->__Header__.__Span__);
            }

            if (__Payload_Count__ == 1U)
            {
                /* Stores the child. */
                __Pattern_Node_Info__ __Child__;
                if (!__Pattern_Analyze_Node__(__Context__,
                                              __Payload_Type__,
                                              __Pattern__->__As__.__Enum__.__Payloads__[0],
                                              __Analysis__,
                                              &__Child__,
                                              __Error__))
                {
                    return 0;
                }
                __Payloads_Irrefutable__ = __Child__.__Irrefutable__;
            }
            else if (__Payload_Count__ > 1U)
            {
                /* User enums may carry multiple payload slots. */
                __Semantic_Type_Entry__ *__Pattern_Type__ = __Resolved__.__Named__;
                /* References the constructor. */
                __Ast_Enum_Constructor__ *__Constructor__ = NULL;
                /* Tracks the ignored index. */
                size_t __Ignored_Index__ = 0U;
                if (__Pattern_Type__ == NULL ||
                    !__Name_Find_Enum_Constructor__(
                        __Pattern_Type__,
                        __Pattern__->__As__.__Enum__.__Constructor_Name__,
                        &__Ignored_Index__,
                        &__Constructor__))
                {
                    return 0;
                }
                for (__Index__ = 0U; __Index__ < __Payload_Count__; ++__Index__)
                {
                    /* Stores the child. */
                    __Pattern_Node_Info__ __Child__;
                    if (!__Pattern_Analyze_Node__(
                            __Context__,
                            __Constructor__->__Payload_Slots__[__Index__].__Type__,
                            __Pattern__->__As__.__Enum__.__Payloads__[__Index__],
                            __Analysis__,
                            &__Child__,
                            __Error__))
                    {
                        return 0;
                    }
                    __Payloads_Irrefutable__ =
                        __Payloads_Irrefutable__ && __Child__.__Irrefutable__;
                }
            }

            __Out_Info__->__Has_Enum_Constructor__ = 1;
            __Out_Info__->__Enum_Constructor_Index__ = __Constructor_Index__;
            __Out_Info__->__Enum_Constructor_Fully_Covered__ = __Payloads_Irrefutable__;
            __Out_Info__->__Irrefutable__ = __Constructor_Count__ == 1U && __Payloads_Irrefutable__;
            return 1;
        }
    }

    return __Pattern_Fail__(
        __Error__, __E0404_Statement_Typecheck_Not_Implemented__, __Pattern__->__Header__.__Span__);
}

/* Analyzes the pattern. */
int __Pattern_Analyze__(__Semantic_Context__ *__Context__,
                        __Ast_Type__ *__Expected_Type__,
                        __Ast_Pattern__ *__Pattern__,
                        __Pattern_Analysis__ *__Out_Analysis__,
                        __Pattern_Error__ *__Out_Error__)
{
    /* Stores the info. */
    __Pattern_Node_Info__ __Info__;

    if (__Out_Analysis__ == NULL)
    {
        return 0;
    }

    if (!__Pattern_Analyze_Node__(__Context__,
                                  __Expected_Type__,
                                  __Pattern__,
                                  __Out_Analysis__,
                                  &__Info__,
                                  __Out_Error__))
    {
        return 0;
    }

    __Out_Analysis__->__Irrefutable__ = __Info__.__Irrefutable__;
    __Out_Analysis__->__Has_Top_Enum_Constructor__ = __Info__.__Has_Enum_Constructor__;
    __Out_Analysis__->__Top_Enum_Constructor_Index__ = __Info__.__Enum_Constructor_Index__;
    __Out_Analysis__->__Top_Enum_Constructor_Fully_Covered__ =
        __Info__.__Enum_Constructor_Fully_Covered__;
    return 1;
}
