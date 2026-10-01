/* Provides shared body-checking context helpers. */

#include "semantic/diagnostic.h"
#include "semantic/body_internal.h"
#include "kernel/type/conversion.h"
#include "frontend/ast/identity.h"

#include <stdalign.h>
#include <string.h>

/* Stores the body builtin int type. */
__Ast_Type__ __Body_Builtin_Int_Type__ = {.__Kind__ = __Ast_Type_Integer__};
/* Stores the body builtin bool type. */
__Ast_Type__ __Body_Builtin_Bool_Type__ = {.__Kind__ = __Ast_Type_Boolean__};
/* Stores the body builtin char type. */
__Ast_Type__ __Body_Builtin_Char_Type__ = {.__Kind__ = __Ast_Type_Character__};
/* Stores the body builtin string type. */
__Ast_Type__ __Body_Builtin_String_Type__ = {.__Kind__ = __Ast_Type_String__};
/* Stores the body builtin void type. */
__Ast_Type__ __Body_Builtin_Void_Type__ = {.__Kind__ = __Ast_Type_Void__};
/* Stores the body builtin u 8 type. */
__Ast_Type__ __Body_Builtin_U8_Type__ = {.__Kind__ = __Ast_Type_Machine__,
                                         .__As__.__Machine__ = __Machine_U8__};

/* Begins the body diagnostic. */
__Diagnostic__ *__Body_Begin_Diagnostic__(__Semantic_Body_Context__ *__Context__,
                                          __Error_Id__ __Id__,
                                          __Source_Span__ __Span__)
{
    /* References the diagnostic state. */
    __Diagnostic__ *__Diagnostic_State__;

    if (__Context__ == NULL)
    {
        return NULL;
    }

    __Diagnostic_State__ =
        __Semantic_Begin_Diagnostic__(__Context__->__Semantic__, __Id__, __Span__);
    if (__Diagnostic_State__ != NULL && __Context__->__Function__ != NULL &&
        __Context__->__Function__->__Function__ != NULL)
    {
        (void)__Diagnostic_Set_Argument_Text__(__Diagnostic_State__,
                                               __Diagnostic_Argument_Function__,
                                               __Context__->__Function__->__Function__->__Name__);
    }
    return __Diagnostic_State__;
}

/* Records a failure for the body. */
int __Body_Fail__(__Semantic_Body_Context__ *__Context__,
                  __Error_Id__ __Id__,
                  __Source_Span__ __Span__)
{
    (void)__Body_Begin_Diagnostic__(__Context__, __Id__, __Span__);
    return 0;
}

/* Finds the body local. */
__Semantic_Local__ *__Body_Find_Local__(__Semantic_Body_Context__ *__Context__,
                                        const __Ast_Lvalue__ *__Lvalue__)
{
    /* Tracks the index. */
    size_t __Index__ = __Context__->__Locals__.__Count__;
    if (__Lvalue__ == NULL || __Lvalue__->__Kind__ != __Ast_Lvalue_Base__)
    {
        return NULL;
    }
    while (__Index__ > 0U)
    {
        /* References the local. */
        __Semantic_Local__ *__Local__ =
            (__Semantic_Local__ *)__Vector_At__(&__Context__->__Locals__, __Index__ - 1U);
        --__Index__;
        if (__Local__ != NULL && __Ast_Base_Identity_Matches_Lvalue__(__Local__->__Name_Kind__,
                                                                      __Local__->__Name__,
                                                                      __Local__->__Temporary__,
                                                                      __Lvalue__))
        {
            return __Local__;
        }
    }
    return NULL;
}

/* Returns the body local index. */
size_t __Body_Local_Index__(__Semantic_Body_Context__ *__Context__,
                            const __Semantic_Local__ *__Local__)
{
    /* Tracks the index. */
    size_t __Index__;

    if (__Context__ == NULL || __Local__ == NULL)
    {
        return SIZE_MAX;
    }
    for (__Index__ = 0U; __Index__ < __Context__->__Locals__.__Count__; ++__Index__)
    {
        /* References the candidate. */
        const __Semantic_Local__ *__Candidate__ =
            (const __Semantic_Local__ *)__Vector_At_Const__(&__Context__->__Locals__, __Index__);
        if (__Candidate__ == __Local__)
        {
            return __Index__;
        }
    }
    return SIZE_MAX;
}

/* Returns the body name exists in current scope. */
int __Body_Name_Exists_In_Current_Scope__(__Semantic_Body_Context__ *__Context__,
                                          const __Semantic_Local__ *__Candidate__)
{
    /* Tracks the index. */
    size_t __Index__ = __Context__->__Locals__.__Count__;
    while (__Index__ > 0U)
    {
        /* References the local. */
        const __Semantic_Local__ *__Local__ = (const __Semantic_Local__ *)__Vector_At_Const__(
            &__Context__->__Locals__, __Index__ - 1U);
        --__Index__;
        if (__Local__ == NULL || __Local__->__Scope_Depth__ != __Context__->__Scope_Depth__)
        {
            continue;
        }
        if (__Ast_Base_Identity_Matches__(__Local__->__Name_Kind__,
                                          __Local__->__Name__,
                                          __Local__->__Temporary__,
                                          __Candidate__->__Name_Kind__,
                                          __Candidate__->__Name__,
                                          __Candidate__->__Temporary__))
        {
            return 1;
        }
    }
    return 0;
}

/* Returns the body synthetic named type. */
__Ast_Type__ *__Body_Synthetic_Named_Type__(__Semantic_Body_Context__ *__Context__,
                                            __Text_Slice__ __Name__)
{
    /* References the type. */
    __Ast_Type__ *__Type__ = (__Ast_Type__ *)__Arena_Allocate__(
        &__Context__->__Synthetic_Types__, sizeof(*__Type__), alignof(__Ast_Type__));
    if (__Type__ == NULL)
    {
        return NULL;
    }
    memset(__Type__, 0, sizeof(*__Type__));
    __Type__->__Kind__ = __Ast_Type_Named__;
    __Type__->__As__.__Named__.__Name__ = __Name__;
    return __Type__;
}

/* Returns the body synthetic reference type. */
__Ast_Type__ *__Body_Synthetic_Reference_Type__(__Semantic_Body_Context__ *__Context__,
                                                __Ast_Type__ *__Inner__,
                                                int __Mutable__)
{
    /* References the reference. */
    __Ast_Type__ *__Reference__ = (__Ast_Type__ *)__Arena_Allocate__(
        &__Context__->__Synthetic_Types__, sizeof(*__Reference__), alignof(__Ast_Type__));
    /* References the referent. */
    __Ast_Type__ *__Referent__ = __Type_Unwrap_Mutable__(__Inner__);

    if (__Reference__ == NULL || __Referent__ == NULL)
    {
        return NULL;
    }
    if (__Mutable__)
    {
        /* Tracks the mutable type state. */
        __Ast_Type__ *__Mutable_Type__ = (__Ast_Type__ *)__Arena_Allocate__(
            &__Context__->__Synthetic_Types__, sizeof(*__Mutable_Type__), alignof(__Ast_Type__));
        if (__Mutable_Type__ == NULL)
        {
            return NULL;
        }
        memset(__Mutable_Type__, 0, sizeof(*__Mutable_Type__));
        __Mutable_Type__->__Kind__ = __Ast_Type_Mutable__;
        __Mutable_Type__->__As__.__Inner__ = __Referent__;
        __Referent__ = __Mutable_Type__;
    }
    memset(__Reference__, 0, sizeof(*__Reference__));
    __Reference__->__Kind__ = __Ast_Type_Reference__;
    __Reference__->__As__.__Inner__ = __Referent__;
    return __Reference__;
}

/* Returns the body synthetic result type. */
__Ast_Type__ *__Body_Synthetic_Result_Type__(__Semantic_Body_Context__ *__Context__,
                                             __Ast_Type__ *__Ok__,
                                             __Ast_Type__ *__Error__)
{
    /* References the type. */
    __Ast_Type__ *__Type__ = (__Ast_Type__ *)__Arena_Allocate__(
        &__Context__->__Synthetic_Types__, sizeof(*__Type__), alignof(__Ast_Type__));

    if (__Type__ == NULL || __Ok__ == NULL || __Error__ == NULL)
    {
        return NULL;
    }
    memset(__Type__, 0, sizeof(*__Type__));
    __Type__->__Kind__ = __Ast_Type_Result__;
    __Type__->__As__.__Result__.__Ok__ = __Ok__;
    __Type__->__As__.__Result__.__Error__ = __Error__;
    return __Type__;
}

/* Checks whether the body is integer like. */
int __Body_Is_Integer_Like__(__Semantic_Body_Context__ *__Context__,
                             __Ast_Type__ *__Type__,
                             int *__Out_Signed__)
{
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;
    if (!__Type_Resolve__(__Context__->__Semantic__, __Type__, &__Resolved__))
    {
        return 0;
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Signed_Integer__)
    {
        if (__Out_Signed__ != NULL)
        {
            *__Out_Signed__ = 1;
        }
        return 1;
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Unsigned_Integer__)
    {
        if (__Out_Signed__ != NULL)
        {
            *__Out_Signed__ = 0;
        }
        return 1;
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Character__)
    {
        if (__Out_Signed__ != NULL)
        {
            *__Out_Signed__ = 0;
        }
        return 1;
    }
    return 0;
}

/* Checks whether the body is bool convertible. */
int __Body_Is_Bool_Convertible__(__Semantic_Body_Context__ *__Context__, __Ast_Type__ *__Type__)
{
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;
    if (!__Type_Resolve__(__Context__->__Semantic__, __Type__, &__Resolved__))
    {
        return 0;
    }
    return __Resolved__.__Kind__ == __Resolved_Type_Boolean__ ||
           __Resolved__.__Kind__ == __Resolved_Type_Signed_Integer__ ||
           __Resolved__.__Kind__ == __Resolved_Type_Unsigned_Integer__ ||
           __Resolved__.__Kind__ == __Resolved_Type_Character__ ||
           __Resolved__.__Kind__ == __Resolved_Type_Box__;
}

/* Checks the body compatible. */
int __Body_Check_Compatible__(__Semantic_Body_Context__ *__Context__,
                              __Ast_Type__ *__Expected__,
                              __Ast_Type__ *__Actual__,
                              __Source_Span__ __Span__)
{
    if (__Expected__ == NULL || __Actual__ == NULL ||
        !__Type_Conversion_Is_Safe__(__Context__->__Semantic__, __Actual__, __Expected__))
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Span__);
    }
    return 1;
}

/* Checks the body atom compatible. */
int __Body_Check_Atom_Compatible__(__Semantic_Body_Context__ *__Context__,
                                   __Ast_Type__ *__Expected__,
                                   const __Ast_Atom__ *__Atom__,
                                   __Source_Span__ __Span__)
{
    /* References the actual. */
    __Ast_Type__ *__Actual__ = NULL;
    /* Stores the literal value. */
    int64_t __Literal_Value__ = 0;
    if (__Semantic_Integer_Literal_Atom_Value__(__Atom__, &__Literal_Value__) &&
        __Type_Integer_Literal_Fits__(__Context__->__Semantic__, __Literal_Value__, __Expected__))
    {
        return 1;
    }
    if (!__Body_Infer_Atom__(__Context__, __Atom__, &__Actual__))
    {
        return 0;
    }
    return __Body_Check_Compatible__(__Context__, __Expected__, __Actual__, __Span__);
}

/* Checks the body expression compatible. */
int __Body_Check_Expression_Compatible__(__Semantic_Body_Context__ *__Context__,
                                         __Ast_Type__ *__Expected__,
                                         __Ast_Expression__ *__Expression__,
                                         __Source_Span__ __Span__)
{
    /* References the actual. */
    __Ast_Type__ *__Actual__ = NULL;
    /* Stores the literal value. */
    int64_t __Literal_Value__ = 0;
    /* Tracks the tagged matched state. */
    int __Tagged_Matched__ = 0;

    if (!__Body_Try_Check_Builtin_Tagged_Construct__(
            __Context__, __Expected__, __Expression__, &__Tagged_Matched__))
    {
        return 0;
    }
    if (__Tagged_Matched__)
    {
        return 1;
    }
    if (__Semantic_Integer_Literal_Expression_Value__(__Expression__, &__Literal_Value__) &&
        __Type_Integer_Literal_Fits__(__Context__->__Semantic__, __Literal_Value__, __Expected__))
    {
        return 1;
    }
    if (!__Body_Infer_Expression__(__Context__, __Expression__, &__Actual__))
    {
        return 0;
    }
    return __Body_Check_Compatible__(__Context__, __Expected__, __Actual__, __Span__);
}
