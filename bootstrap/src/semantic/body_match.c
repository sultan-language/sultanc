/* Checks match statements and pattern coverage. */

#include "semantic/body_internal.h"
#include "kernel/pattern/pattern.h"
#include "kernel/type/tagged.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Checks the body match. */
int __Body_Check_Match__(__Semantic_Body_Context__ *__Context__,
                         __Ast_Statement__ *__Statement__,
                         int *__Out_Falls_Through__)
{
    /* References the value type. */
    __Ast_Type__ *__Value_Type__ = NULL;
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;
    /* Tracks the covered constructors state. */
    unsigned char *__Covered_Constructors__ = NULL;
    /* References the covering spans. */
    __Source_Span__ *__Covering_Spans__ = NULL;
    /* Stores the irrefutable covering span. */
    __Source_Span__ __Irrefutable_Covering_Span__ = {0};
    /* Tracks whether the irrefutable covering span is present. */
    int __Has_Irrefutable_Covering_Span__ = 0;
    /* Stores the constructor count. */
    size_t __Constructor_Count__ = 0U;
    /* Tracks the fully covered count state. */
    size_t __Fully_Covered_Count__ = 0U;
    /* Tracks the case index. */
    size_t __Case_Index__ = 0U;
    /* Tracks the covered all state. */
    int __Covered_All__ = 0;
    /* Stores the before. */
    __Body_Safety_Snapshot__ __Before__ = {0};
    /* Stores the accumulated. */
    __Body_Safety_Snapshot__ __Accumulated__ = {0};
    /* Stores the have fallthrough path. */
    int __Have_Fallthrough_Path__ = 0;

    if (!__Body_Infer_Expression__(
            __Context__, __Statement__->__As__.__Match__.__Value__, &__Value_Type__) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Value_Type__, &__Resolved__))
    {
        return 0;
    }

    if (__Type_Is_Builtin_Tagged__(__Value_Type__))
    {
        __Constructor_Count__ = __Type_Tagged_Constructor_Count__(__Value_Type__);
        if (__Constructor_Count__ != 0U)
        {
            __Covered_Constructors__ =
                (unsigned char *)calloc(__Constructor_Count__, sizeof(*__Covered_Constructors__));
            __Covering_Spans__ =
                (__Source_Span__ *)calloc(__Constructor_Count__, sizeof(*__Covering_Spans__));
            if (__Covered_Constructors__ == NULL || __Covering_Spans__ == NULL)
            {
                free(__Covered_Constructors__);
                free(__Covering_Spans__);
                return __Body_Fail__(__Context__,
                                     __E1100_Internal_Context_Error__,
                                     __Statement__->__Header__.__Span__);
            }
        }
    }
    else if (__Resolved__.__Kind__ == __Resolved_Type_Enum__ && __Resolved__.__Named__ != NULL)
    {
        __Constructor_Count__ = __Resolved__.__Named__->__Declaration__->__As__.__Enum__.__Count__;
        if (__Constructor_Count__ != 0U)
        {
            __Covered_Constructors__ =
                (unsigned char *)calloc(__Constructor_Count__, sizeof(*__Covered_Constructors__));
            __Covering_Spans__ =
                (__Source_Span__ *)calloc(__Constructor_Count__, sizeof(*__Covering_Spans__));
            if (__Covered_Constructors__ == NULL || __Covering_Spans__ == NULL)
            {
                free(__Covered_Constructors__);
                free(__Covering_Spans__);
                return __Body_Fail__(__Context__,
                                     __E1100_Internal_Context_Error__,
                                     __Statement__->__Header__.__Span__);
            }
        }
    }

    if (!__Body_Safety_Capture__(__Context__, &__Before__))
    {
        free(__Covered_Constructors__);
        free(__Covering_Spans__);
        return 0;
    }

    for (__Case_Index__ = 0U; __Case_Index__ < __Statement__->__As__.__Match__.__Case_Count__;
         ++__Case_Index__)
    {
        /* References the case. */
        __Ast_Match_Case__ *__Case__ = &__Statement__->__As__.__Match__.__Cases__[__Case_Index__];
        /* Stores the analysis. */
        __Pattern_Analysis__ __Analysis__;
        /* Stores the pattern error. */
        __Pattern_Error__ __Pattern_Error__ = {0};
        /* Stores the saved local count. */
        size_t __Saved_Local_Count__ = __Before__.__Count__;
        /* Tracks the binding index. */
        size_t __Binding_Index__ = 0U;
        /* Tracks the case falls state. */
        int __Case_Falls__ = 1;
        /* Stores the case after. */
        __Body_Safety_Snapshot__ __Case_After__ = {0};

        if (__Covered_All__)
        {
            /* References the diagnostic state. */
            __Diagnostic__ *__Diagnostic_State__ =
                __Body_Begin_Diagnostic__(__Context__,
                                          __E1307_Unreachable_Pattern__,
                                          __Case__->__Pattern__->__Header__.__Span__);
            if (__Diagnostic_State__ != NULL)
            {
                __Diagnostic_Set_Primary_Role__(__Diagnostic_State__,
                                                __Diagnostic_Span_Role_Unreachable_Pattern__);
                if (__Has_Irrefutable_Covering_Span__)
                {
                    __Diagnostic_Add_Related_Span__(__Diagnostic_State__,
                                                    __Irrefutable_Covering_Span__,
                                                    __Diagnostic_Span_Role_Earlier_Coverage__);
                }
            }
            free(__Covered_Constructors__);
            free(__Covering_Spans__);
            __Body_Safety_Snapshot_Destroy__(&__Before__);
            __Body_Safety_Snapshot_Destroy__(&__Accumulated__);
            return 0;
        }

        __Pattern_Analysis_Init__(&__Analysis__);
        if (!__Pattern_Analyze__(__Context__->__Semantic__,
                                 __Value_Type__,
                                 __Case__->__Pattern__,
                                 &__Analysis__,
                                 &__Pattern_Error__))
        {
            __Pattern_Analysis_Destroy__(&__Analysis__);
            free(__Covered_Constructors__);
            free(__Covering_Spans__);
            __Body_Safety_Snapshot_Destroy__(&__Before__);
            __Body_Safety_Snapshot_Destroy__(&__Accumulated__);
            return __Body_Fail__(__Context__, __Pattern_Error__.__Id__, __Pattern_Error__.__Span__);
        }

        if (__Analysis__.__Has_Top_Enum_Constructor__ &&
            __Analysis__.__Top_Enum_Constructor_Fully_Covered__ && __Covered_Constructors__ != NULL)
        {
            /* Tracks the constructor index. */
            size_t __Constructor_Index__ = __Analysis__.__Top_Enum_Constructor_Index__;
            if (__Covered_Constructors__[__Constructor_Index__])
            {
                /* References the diagnostic state. */
                __Diagnostic__ *__Diagnostic_State__ =
                    __Body_Begin_Diagnostic__(__Context__,
                                              __E1307_Unreachable_Pattern__,
                                              __Case__->__Pattern__->__Header__.__Span__);
                if (__Diagnostic_State__ != NULL)
                {
                    __Diagnostic_Set_Primary_Role__(__Diagnostic_State__,
                                                    __Diagnostic_Span_Role_Unreachable_Pattern__);
                    if (__Covering_Spans__ != NULL)
                    {
                        __Diagnostic_Add_Related_Span__(__Diagnostic_State__,
                                                        __Covering_Spans__[__Constructor_Index__],
                                                        __Diagnostic_Span_Role_Earlier_Coverage__);
                    }
                }
                __Pattern_Analysis_Destroy__(&__Analysis__);
                free(__Covered_Constructors__);
                free(__Covering_Spans__);
                __Body_Safety_Snapshot_Destroy__(&__Before__);
                __Body_Safety_Snapshot_Destroy__(&__Accumulated__);
                return 0;
            }
            __Covered_Constructors__[__Constructor_Index__] = 1U;
            if (__Covering_Spans__ != NULL)
            {
                __Covering_Spans__[__Constructor_Index__] =
                    __Case__->__Pattern__->__Header__.__Span__;
            }
            ++__Fully_Covered_Count__;
        }

        if (__Analysis__.__Irrefutable__)
        {
            __Covered_All__ = 1;
            __Irrefutable_Covering_Span__ = __Case__->__Pattern__->__Header__.__Span__;
            __Has_Irrefutable_Covering_Span__ = 1;
        }

        __Body_Safety_Restore__(__Context__, &__Before__);
        __Context__->__Locals__.__Count__ = __Saved_Local_Count__;
        for (__Binding_Index__ = 0U; __Binding_Index__ < __Analysis__.__Bindings__.__Count__;
             ++__Binding_Index__)
        {
            /* References the binding. */
            const __Pattern_Binding__ *__Binding__ =
                (const __Pattern_Binding__ *)__Vector_At_Const__(&__Analysis__.__Bindings__,
                                                                 __Binding_Index__);
            /* Stores the local. */
            __Semantic_Local__ __Local__;

            if (__Binding__ == NULL)
            {
                continue;
            }
            memset(&__Local__, 0, sizeof(__Local__));
            __Local__.__Name_Kind__ = __Ast_Lvalue_Base_Identifier__;
            __Local__.__Name__ = __Binding__->__Name__;
            __Local__.__Type__ = __Binding__->__Type__;
            __Local__.__Scope_Depth__ = __Context__->__Scope_Depth__ + 1U;
            __Local__.__Declaration_Span__ = __Binding__->__Span__;
            __Safety_Fact_Init__(&__Local__.__Safety__, 1);
            __Safety_Fact_Mark_Initialized_At__(&__Local__.__Safety__, __Binding__->__Span__);
            if (__Vector_Push__(&__Context__->__Locals__, &__Local__) == NULL)
            {
                __Pattern_Analysis_Destroy__(&__Analysis__);
                free(__Covered_Constructors__);
                free(__Covering_Spans__);
                __Context__->__Locals__.__Count__ = __Saved_Local_Count__;
                __Body_Safety_Snapshot_Destroy__(&__Before__);
                __Body_Safety_Snapshot_Destroy__(&__Accumulated__);
                return __Body_Fail__(__Context__,
                                     __E1100_Internal_Context_Error__,
                                     __Statement__->__Header__.__Span__);
            }
        }

        if (!__Body_Check_Block_Flow__(__Context__, __Case__->__Body__, &__Case_Falls__))
        {
            __Pattern_Analysis_Destroy__(&__Analysis__);
            free(__Covered_Constructors__);
            free(__Covering_Spans__);
            __Context__->__Locals__.__Count__ = __Saved_Local_Count__;
            __Body_Safety_Snapshot_Destroy__(&__Before__);
            __Body_Safety_Snapshot_Destroy__(&__Accumulated__);
            return 0;
        }

        __Context__->__Locals__.__Count__ = __Saved_Local_Count__;
        if (__Case_Falls__)
        {
            if (!__Body_Safety_Capture__(__Context__, &__Case_After__))
            {
                __Pattern_Analysis_Destroy__(&__Analysis__);
                free(__Covered_Constructors__);
                free(__Covering_Spans__);
                __Body_Safety_Snapshot_Destroy__(&__Before__);
                __Body_Safety_Snapshot_Destroy__(&__Accumulated__);
                return 0;
            }

            if (!__Have_Fallthrough_Path__)
            {
                __Accumulated__ = __Case_After__;
                __Case_After__.__States__ = NULL;
                __Case_After__.__Count__ = 0U;
                __Have_Fallthrough_Path__ = 1;
            }
            else
            {
                __Body_Safety_Merge__(__Context__, &__Accumulated__, &__Case_After__);
                __Body_Safety_Snapshot_Destroy__(&__Accumulated__);
                if (!__Body_Safety_Capture__(__Context__, &__Accumulated__))
                {
                    __Body_Safety_Snapshot_Destroy__(&__Case_After__);
                    __Pattern_Analysis_Destroy__(&__Analysis__);
                    free(__Covered_Constructors__);
                    free(__Covering_Spans__);
                    __Body_Safety_Snapshot_Destroy__(&__Before__);
                    return 0;
                }
            }
        }

        __Body_Safety_Snapshot_Destroy__(&__Case_After__);
        __Pattern_Analysis_Destroy__(&__Analysis__);
    }

    if (!__Covered_All__ &&
        !(__Constructor_Count__ != 0U && __Fully_Covered_Count__ == __Constructor_Count__))
    {
        /* Stores the missing pattern. */
        char __Missing_Pattern__[__Diagnostic_Max_Argument_Value__] = "_";
        /* References the diagnostic state. */
        __Diagnostic__ *__Diagnostic_State__ = __Body_Begin_Diagnostic__(
            __Context__, __E1306_Non_Exhaustive_Pattern__, __Statement__->__Header__.__Span__);
        /* Tracks the missing index. */
        size_t __Missing_Index__ = 0U;

        while (__Missing_Index__ < __Constructor_Count__ && __Covered_Constructors__ != NULL &&
               __Covered_Constructors__[__Missing_Index__])
        {
            ++__Missing_Index__;
        }

        if (__Missing_Index__ < __Constructor_Count__)
        {
            /* References the base type. */
            __Ast_Type__ *__Base_Type__ = __Type_Unwrap_Mutable__(__Value_Type__);
            if (__Base_Type__ != NULL && __Base_Type__->__Kind__ == __Ast_Type_Option__)
            {
                (void)snprintf(__Missing_Pattern__,
                               sizeof(__Missing_Pattern__),
                               "%s",
                               __Missing_Index__ == 0U ? "none" : "some(_)");
            }
            else if (__Base_Type__ != NULL && __Base_Type__->__Kind__ == __Ast_Type_Result__)
            {
                (void)snprintf(__Missing_Pattern__,
                               sizeof(__Missing_Pattern__),
                               "%s",
                               __Missing_Index__ == 0U ? "ok(_)" : "err(_)");
            }
            else if (__Resolved__.__Kind__ == __Resolved_Type_Enum__ &&
                     __Resolved__.__Named__ != NULL &&
                     __Resolved__.__Named__->__Declaration__ != NULL &&
                     __Missing_Index__ <
                         __Resolved__.__Named__->__Declaration__->__As__.__Enum__.__Count__)
            {
                /* References the constructor. */
                const __Ast_Enum_Constructor__ *__Constructor__ =
                    &__Resolved__.__Named__->__Declaration__->__As__.__Enum__
                         .__Constructors__[__Missing_Index__];
                /* Stores the name length. */
                size_t __Name_Length__ = __Constructor__->__Name__.__Length__;
                /* Stores the copy length. */
                size_t __Copy_Length__ = __Name_Length__ < sizeof(__Missing_Pattern__) - 4U
                                             ? __Name_Length__
                                             : sizeof(__Missing_Pattern__) - 4U;
                memcpy(__Missing_Pattern__, __Constructor__->__Name__.__Data__, __Copy_Length__);
                if (__Constructor__->__Payload_Count__ != 0U)
                {
                    memcpy(__Missing_Pattern__ + __Copy_Length__, "(_)", 4U);
                }
                else
                {
                    __Missing_Pattern__[__Copy_Length__] = '\0';
                }
            }
        }

        if (__Diagnostic_State__ != NULL)
        {
            __Diagnostic_Set_Message_Key__(__Diagnostic_State__, __Diag_Word_Pattern_Missing__);
            __Diagnostic_Set_Argument_Cstr__(
                __Diagnostic_State__, __Diagnostic_Argument_Pattern__, __Missing_Pattern__);
            __Diagnostic_Add_Note__(__Diagnostic_State__, __Diag_Word_Note_Missing_Pattern__);
            __Diagnostic_Add_Help__(__Diagnostic_State__, __Diag_Word_Help_Add_Pattern__);
        }
        free(__Covered_Constructors__);
        free(__Covering_Spans__);
        __Body_Safety_Snapshot_Destroy__(&__Before__);
        __Body_Safety_Snapshot_Destroy__(&__Accumulated__);
        return 0;
    }

    free(__Covered_Constructors__);
    free(__Covering_Spans__);
    if (__Have_Fallthrough_Path__)
    {
        __Body_Safety_Restore__(__Context__, &__Accumulated__);
    }
    else
    {
        __Body_Safety_Restore__(__Context__, &__Before__);
    }

    if (__Out_Falls_Through__ != NULL)
    {
        *__Out_Falls_Through__ = __Have_Fallthrough_Path__;
    }

    __Body_Safety_Snapshot_Destroy__(&__Before__);
    __Body_Safety_Snapshot_Destroy__(&__Accumulated__);
    return 1;
}
