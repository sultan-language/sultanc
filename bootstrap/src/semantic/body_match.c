/* Checks match statements and pattern coverage. */

#include "semantic/body_internal.h"
#include "kernel/pattern/pattern.h"
#include "kernel/type/tagged.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Collects the closed intervals a top-level literal, range or alternatives
 * pattern covers on an integer or boolean scrutinee (false = 0, true = 1). */
static void __Match_Collect_Intervals__(const __Ast_Pattern__ *__Pattern__,
                                        int64_t *__Lows__,
                                        int64_t *__Highs__,
                                        size_t *__Count__,
                                        size_t __Capacity__)
{
    int64_t __Low__ = 0;
    int64_t __High__ = 0;
    const __Ast_Expression__ *__Value__;
    size_t __Index__;
    if (__Pattern__ == NULL)
        return;
    if (__Pattern__->__Kind__ == __Ast_Pattern_Alternatives__)
    {
        for (__Index__ = 0U; __Index__ < __Pattern__->__As__.__Alternatives__.__Count__; ++__Index__)
            __Match_Collect_Intervals__(__Pattern__->__As__.__Alternatives__.__Items__[__Index__], __Lows__,
                                        __Highs__, __Count__, __Capacity__);
        return;
    }
    if (__Pattern__->__Kind__ == __Ast_Pattern_Literal__)
    {
        __Value__ = __Pattern__->__As__.__Literal__;
        if (__Value__ != NULL && __Value__->__Kind__ == __Ast_Expression_Atom__ &&
            __Value__->__As__.__Atom__.__Kind__ == __Ast_Atom_Literal__ &&
            __Value__->__As__.__Atom__.__As__.__Literal__ != NULL &&
            __Value__->__As__.__Atom__.__As__.__Literal__->__Kind__ == __Ast_Literal_Boolean__)
        {
            __Low__ = __Value__->__As__.__Atom__.__As__.__Literal__->__As__.__Boolean__ ? 1 : 0;
        }
        else if (!__Semantic_Integer_Literal_Expression_Value__(__Value__, &__Low__))
        {
            return;
        }
        __High__ = __Low__;
    }
    else if (__Pattern__->__Kind__ == __Ast_Pattern_Range__)
    {
        if (!__Semantic_Integer_Literal_Expression_Value__(__Pattern__->__As__.__Range__.__Low__, &__Low__) ||
            !__Semantic_Integer_Literal_Expression_Value__(__Pattern__->__As__.__Range__.__High__, &__High__))
            return;
    }
    else
    {
        return;
    }
    if (*__Count__ < __Capacity__ && __Low__ <= __High__)
    {
        __Lows__[*__Count__] = __Low__;
        __Highs__[*__Count__] = __High__;
        ++*__Count__;
    }
}

/* Answers whether the intervals cover [MINIMUM, MAXIMUM]. */
static int __Match_Intervals_Cover__(int64_t *__Lows__, int64_t *__Highs__, size_t __Count__,
                                     int64_t __Minimum__, int64_t __Maximum__)
{
    int64_t __Next__ = __Minimum__;
    int __Progress__ = 1;
    while (__Progress__)
    {
        size_t __Index__;
        __Progress__ = 0;
        for (__Index__ = 0U; __Index__ < __Count__; ++__Index__)
        {
            if (__Lows__[__Index__] <= __Next__ && __Highs__[__Index__] >= __Next__)
            {
                if (__Highs__[__Index__] >= __Maximum__)
                    return 1;
                __Next__ = __Highs__[__Index__] + 1;
                __Progress__ = 1;
            }
        }
    }
    return 0;
}

/* For `C(D(…))`, a constructor pattern whose only payload is a constructor
 * pattern: answers C's index, D's index, and the payload Type's constructor
 * count, when D's own payloads are irrefutable. One level of nesting is what
 * std uses (`نتيجة.نجح(اختياري.لا_شيء)` beside `نتيجة.نجح(اختياري.موجود(س))`). */
static int __Match_Nested_Constructor__(__Semantic_Context__ *__Semantic__,
                                        __Ast_Type__ *__Value_Type__,
                                        __Ast_Pattern__ *__Pattern__,
                                        size_t *__Out_Outer__,
                                        size_t *__Out_Inner__,
                                        size_t *__Out_Inner_Count__)
{
    __Pattern_Analysis__ __Outer__;
    __Pattern_Analysis__ __Inner__;
    __Pattern_Error__ __Error__ = {0};
    __Ast_Type__ *__Payload_Type__ = NULL;
    __Resolved_Type__ __Resolved__;
    __Resolved_Type__ __Payload_Resolved__;
    int __Ok__;
    if (__Pattern__ == NULL || __Pattern__->__Kind__ != __Ast_Pattern_Enum__ ||
        __Pattern__->__As__.__Enum__.__Payload_Count__ != 1U ||
        __Pattern__->__As__.__Enum__.__Payloads__[0]->__Kind__ != __Ast_Pattern_Enum__)
        return 0;
    __Pattern_Analysis_Init__(&__Outer__);
    __Ok__ = __Pattern_Analyze__(__Semantic__, __Value_Type__, __Pattern__, &__Outer__, &__Error__) &&
             __Outer__.__Has_Top_Enum_Constructor__;
    *__Out_Outer__ = __Outer__.__Top_Enum_Constructor_Index__;
    __Pattern_Analysis_Destroy__(&__Outer__);
    if (!__Ok__)
        return 0;
    if (__Type_Is_Builtin_Tagged__(__Value_Type__))
    {
        __Type_Tagged_Constructor__ __Constructor__;
        if (!__Type_Tagged_Constructor_At__(__Value_Type__, *__Out_Outer__, &__Constructor__) ||
            __Constructor__.__Payload_Count__ != 1U)
            return 0;
        __Payload_Type__ = __Constructor__.__Payload_Type__;
    }
    else if (__Type_Resolve__(__Semantic__, __Value_Type__, &__Resolved__) &&
             __Resolved__.__Kind__ == __Resolved_Type_Enum__ && __Resolved__.__Named__ != NULL &&
             __Resolved__.__Named__->__Declaration__ != NULL &&
             *__Out_Outer__ < __Resolved__.__Named__->__Declaration__->__As__.__Enum__.__Count__ &&
             __Resolved__.__Named__->__Declaration__->__As__.__Enum__.__Constructors__[*__Out_Outer__]
                     .__Payload_Count__ == 1U)
    {
        __Payload_Type__ = __Resolved__.__Named__->__Declaration__->__As__.__Enum__
                               .__Constructors__[*__Out_Outer__]
                               .__Payload_Slots__[0]
                               .__Type__;
    }
    if (__Payload_Type__ == NULL)
        return 0;
    __Pattern_Analysis_Init__(&__Inner__);
    __Ok__ = __Pattern_Analyze__(__Semantic__, __Payload_Type__, __Pattern__->__As__.__Enum__.__Payloads__[0],
                                 &__Inner__, &__Error__) &&
             __Inner__.__Has_Top_Enum_Constructor__ && __Inner__.__Top_Enum_Constructor_Fully_Covered__;
    *__Out_Inner__ = __Inner__.__Top_Enum_Constructor_Index__;
    __Pattern_Analysis_Destroy__(&__Inner__);
    if (!__Ok__)
        return 0;
    if (__Type_Is_Builtin_Tagged__(__Payload_Type__))
    {
        *__Out_Inner_Count__ = __Type_Tagged_Constructor_Count__(__Payload_Type__);
    }
    else if (__Type_Resolve__(__Semantic__, __Payload_Type__, &__Payload_Resolved__) &&
             __Payload_Resolved__.__Kind__ == __Resolved_Type_Enum__ && __Payload_Resolved__.__Named__ != NULL &&
             __Payload_Resolved__.__Named__->__Declaration__ != NULL)
    {
        *__Out_Inner_Count__ = __Payload_Resolved__.__Named__->__Declaration__->__As__.__Enum__.__Count__;
    }
    else
    {
        return 0;
    }
    return *__Out_Inner_Count__ != 0U && *__Out_Inner_Count__ <= 64U && *__Out_Inner__ < *__Out_Inner_Count__;
}

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
    /* Stores the constructor count. */
    size_t __Constructor_Count__ = 0U;
    /* Tracks the fully covered count state. */
    size_t __Fully_Covered_Count__ = 0U;
    /* The inner constructors covered under each constructor (one level). */
    uint64_t *__Nested_Covered__ = NULL;
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
    /* Stores who owns the scrutinee; pattern bindings inherit it. */
    __Semantic_Ownership__ __Scrutinee_Ownership__ = __Semantic_Ownership_Owner__;

    if (!__Body_Infer_Expression__(
            __Context__, __Statement__->__As__.__Match__.__Value__, &__Value_Type__) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Value_Type__, &__Resolved__) ||
        !__Body_Expression_Ownership__(
            __Context__, __Statement__->__As__.__Match__.__Value__, &__Scrutinee_Ownership__))
    {
        return 0;
    }

    /* NOTE (drop elaboration audit, bootstrap/src/llvm): matching does NOT
     * consume the scrutinee in this checker's existing, actually-relied-on
     * semantics -- compiler/driver/process_driver.sn reads the scrutinee
     * again inside an arm after matching it, and rejecting that (verified
     * by attempting a move-on-match-scrutinee rule here and rebuilding
     * Stage1, which failed with Use-After-Move on exactly that call) would
     * reject real, currently-accepted programs. Left unchanged: this file
     * remains the canonical, unmodified owner of match's move facts.
     * Consequence for LLVM lowering: a pattern binding of an owned-shaped
     * payload is a non-owning alias of the scrutinee's storage, never an
     * independent owner, and must never be given its own release
     * responsibility. */

    if (__Type_Is_Builtin_Tagged__(__Value_Type__))
    {
        __Constructor_Count__ = __Type_Tagged_Constructor_Count__(__Value_Type__);
        if (__Constructor_Count__ != 0U)
        {
            __Covered_Constructors__ =
                (unsigned char *)calloc(__Constructor_Count__, sizeof(*__Covered_Constructors__));
            __Covering_Spans__ =
                (__Source_Span__ *)calloc(__Constructor_Count__, sizeof(*__Covering_Spans__));
            __Nested_Covered__ = (uint64_t *)calloc(__Constructor_Count__, sizeof(*__Nested_Covered__));
            if (__Covered_Constructors__ == NULL || __Covering_Spans__ == NULL)
            {
                free(__Covered_Constructors__);
                free(__Covering_Spans__);
        free(__Nested_Covered__);
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
            __Nested_Covered__ = (uint64_t *)calloc(__Constructor_Count__, sizeof(*__Nested_Covered__));
            if (__Covered_Constructors__ == NULL || __Covering_Spans__ == NULL)
            {
                free(__Covered_Constructors__);
                free(__Covering_Spans__);
        free(__Nested_Covered__);
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
        free(__Nested_Covered__);
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

        /* An arm after full coverage is unreachable: the self-hosted
         * compiler warns (SULTANC-PATTERN-UNREACHABLE) and keeps it; Stage0
         * keeps it silently, still checking its body. */
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
        free(__Nested_Covered__);
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
                /* Unreachable arm: kept, as above. */
            }
            else
            {
            __Covered_Constructors__[__Constructor_Index__] = 1U;
            if (__Covering_Spans__ != NULL)
            {
                __Covering_Spans__[__Constructor_Index__] =
                    __Case__->__Pattern__->__Header__.__Span__;
            }
            ++__Fully_Covered_Count__;
            }
        }

        if (__Covered_Constructors__ != NULL && __Nested_Covered__ != NULL)
        {
            size_t __Outer__ = 0U, __Inner__ = 0U, __Inner_Count__ = 0U;
            if (__Match_Nested_Constructor__(__Context__->__Semantic__, __Value_Type__, __Case__->__Pattern__,
                                             &__Outer__, &__Inner__, &__Inner_Count__) &&
                __Outer__ < __Constructor_Count__ && !__Covered_Constructors__[__Outer__])
            {
                uint64_t __All__ = __Inner_Count__ == 64U ? ~(uint64_t)0 : ((((uint64_t)1) << __Inner_Count__) - 1U);
                __Nested_Covered__[__Outer__] |= ((uint64_t)1) << __Inner__;
                if (__Nested_Covered__[__Outer__] == __All__)
                {
                    __Covered_Constructors__[__Outer__] = 1U;
                    ++__Fully_Covered_Count__;
                }
            }
        }
        if (__Analysis__.__Alternative_Constructor_Count__ != 0U && __Covered_Constructors__ != NULL)
        {
            size_t __Alternative__;
            for (__Alternative__ = 0U; __Alternative__ < __Analysis__.__Alternative_Constructor_Count__;
                 ++__Alternative__)
            {
                size_t __Covered_Index__ = __Analysis__.__Alternative_Constructors__[__Alternative__];
                if (__Covered_Index__ < __Constructor_Count__ && !__Covered_Constructors__[__Covered_Index__])
                {
                    __Covered_Constructors__[__Covered_Index__] = 1U;
                    ++__Fully_Covered_Count__;
                }
            }
        }
        if (__Analysis__.__Irrefutable__)
        {
            __Covered_All__ = 1;
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
            /* A binding of a match on a field, element or box payload stays
             * owned by the enclosing aggregate (see __Body_Safety_Move_Atom__). */
            __Local__.__Ownership__ = __Scrutinee_Ownership__;
            __Safety_Fact_Init__(&__Local__.__Safety__, 1);
            __Safety_Fact_Mark_Initialized_At__(&__Local__.__Safety__, __Binding__->__Span__);
            if (__Vector_Push__(&__Context__->__Locals__, &__Local__) == NULL)
            {
                __Pattern_Analysis_Destroy__(&__Analysis__);
                free(__Covered_Constructors__);
                free(__Covering_Spans__);
        free(__Nested_Covered__);
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
        free(__Nested_Covered__);
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
        free(__Nested_Covered__);
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
        free(__Nested_Covered__);
                    __Body_Safety_Snapshot_Destroy__(&__Before__);
                    return 0;
                }
            }
        }

        __Body_Safety_Snapshot_Destroy__(&__Case_After__);
        __Pattern_Analysis_Destroy__(&__Analysis__);
    }

    if (!__Covered_All__ && __Constructor_Count__ == 0U &&
        (__Resolved__.__Kind__ == __Resolved_Type_Boolean__ ||
         ((__Resolved__.__Kind__ == __Resolved_Type_Signed_Integer__ ||
           __Resolved__.__Kind__ == __Resolved_Type_Unsigned_Integer__) &&
          __Resolved__.__Bits__ < 64U)))
    {
        /* Literals and ranges that together cover every value of a boolean
         * or of a narrow integer Type are exhaustive (canonical usefulness). */
        int64_t __Lows__[256];
        int64_t __Highs__[256];
        size_t __Interval_Count__ = 0U;
        int64_t __Minimum__ = 0;
        int64_t __Maximum__ = 1;
        size_t __Arm__;
        for (__Arm__ = 0U; __Arm__ < __Statement__->__As__.__Match__.__Case_Count__; ++__Arm__)
            __Match_Collect_Intervals__(__Statement__->__As__.__Match__.__Cases__[__Arm__].__Pattern__,
                                        __Lows__, __Highs__, &__Interval_Count__, 256U);
        if (__Resolved__.__Kind__ == __Resolved_Type_Signed_Integer__)
        {
            __Minimum__ = -((int64_t)1 << (__Resolved__.__Bits__ - 1U));
            __Maximum__ = ((int64_t)1 << (__Resolved__.__Bits__ - 1U)) - 1;
        }
        else if (__Resolved__.__Kind__ == __Resolved_Type_Unsigned_Integer__)
        {
            __Maximum__ = (int64_t)((((uint64_t)1) << __Resolved__.__Bits__) - 1U);
        }
        if (__Match_Intervals_Cover__(__Lows__, __Highs__, __Interval_Count__, __Minimum__, __Maximum__))
        {
            __Covered_All__ = 1;
        }
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
        free(__Nested_Covered__);
        __Body_Safety_Snapshot_Destroy__(&__Before__);
        __Body_Safety_Snapshot_Destroy__(&__Accumulated__);
        return 0;
    }

    free(__Covered_Constructors__);
    free(__Covering_Spans__);
        free(__Nested_Covered__);
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
