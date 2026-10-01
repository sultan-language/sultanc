/* Tracks body-level control-flow safety state. */

#include "semantic/body_internal.h"

#include <stdlib.h>

/* Captures the body safety. */
int __Body_Safety_Capture__(__Semantic_Body_Context__ *__Context__,
                            __Body_Safety_Snapshot__ *__Snapshot__)
{
    /* Tracks the index. */
    size_t __Index__ = 0U;

    if (__Snapshot__ == NULL)
    {
        return 0;
    }

    __Snapshot__->__States__ = NULL;
    __Snapshot__->__Count__ = __Context__->__Locals__.__Count__;
    if (__Snapshot__->__Count__ == 0U)
    {
        return 1;
    }

    __Snapshot__->__States__ = (__Body_Safety_Local_State__ *)calloc(
        __Snapshot__->__Count__, sizeof(*__Snapshot__->__States__));
    if (__Snapshot__->__States__ == NULL)
    {
        return __Body_Fail__(__Context__, __E1100_Internal_Context_Error__, (__Source_Span__){0});
    }

    for (__Index__ = 0U; __Index__ < __Snapshot__->__Count__; ++__Index__)
    {
        /* References the local. */
        const __Semantic_Local__ *__Local__ =
            (const __Semantic_Local__ *)__Vector_At_Const__(&__Context__->__Locals__, __Index__);
        if (__Local__ != NULL)
        {
            __Snapshot__->__States__[__Index__].__Safety__ = __Local__->__Safety__;
            __Snapshot__->__States__[__Index__].__Has_Known_Length__ =
                __Local__->__Has_Known_Length__;
            __Snapshot__->__States__[__Index__].__Known_Length__ = __Local__->__Known_Length__;
        }
    }

    return 1;
}

/* Restores the body safety. */
void __Body_Safety_Restore__(__Semantic_Body_Context__ *__Context__,
                             const __Body_Safety_Snapshot__ *__Snapshot__)
{
    /* Tracks the index. */
    size_t __Index__ = 0U;
    /* Stores the count. */
    size_t __Count__ = 0U;

    if (__Snapshot__ == NULL)
    {
        return;
    }

    __Count__ = __Snapshot__->__Count__ < __Context__->__Locals__.__Count__
                    ? __Snapshot__->__Count__
                    : __Context__->__Locals__.__Count__;
    for (__Index__ = 0U; __Index__ < __Count__; ++__Index__)
    {
        /* References the local. */
        __Semantic_Local__ *__Local__ =
            (__Semantic_Local__ *)__Vector_At__(&__Context__->__Locals__, __Index__);
        if (__Local__ != NULL)
        {
            __Local__->__Safety__ = __Snapshot__->__States__[__Index__].__Safety__;
            __Local__->__Has_Known_Length__ =
                __Snapshot__->__States__[__Index__].__Has_Known_Length__;
            __Local__->__Known_Length__ = __Snapshot__->__States__[__Index__].__Known_Length__;
        }
    }
}

/* Merges the body safety. */
void __Body_Safety_Merge__(__Semantic_Body_Context__ *__Context__,
                           const __Body_Safety_Snapshot__ *__Left__,
                           const __Body_Safety_Snapshot__ *__Right__)
{
    /* Tracks the index. */
    size_t __Index__ = 0U;
    /* Stores the count. */
    size_t __Count__;

    if (__Left__ == NULL || __Right__ == NULL)
    {
        return;
    }

    __Count__ =
        __Left__->__Count__ < __Right__->__Count__ ? __Left__->__Count__ : __Right__->__Count__;
    if (__Count__ > __Context__->__Locals__.__Count__)
    {
        __Count__ = __Context__->__Locals__.__Count__;
    }

    for (__Index__ = 0U; __Index__ < __Count__; ++__Index__)
    {
        /* References the local. */
        __Semantic_Local__ *__Local__ =
            (__Semantic_Local__ *)__Vector_At__(&__Context__->__Locals__, __Index__);
        if (__Local__ != NULL)
        {
            __Local__->__Safety__ =
                __Safety_Fact_Merge__(&__Left__->__States__[__Index__].__Safety__,
                                      &__Right__->__States__[__Index__].__Safety__);

            if (__Left__->__States__[__Index__].__Has_Known_Length__ &&
                __Right__->__States__[__Index__].__Has_Known_Length__ &&
                __Left__->__States__[__Index__].__Known_Length__ ==
                    __Right__->__States__[__Index__].__Known_Length__)
            {
                __Local__->__Has_Known_Length__ = 1;
                __Local__->__Known_Length__ = __Left__->__States__[__Index__].__Known_Length__;
            }
            else
            {
                __Local__->__Has_Known_Length__ = 0;
                __Local__->__Known_Length__ = 0U;
            }
        }
    }
}

/* Releases the body safety snapshot. */
void __Body_Safety_Snapshot_Destroy__(__Body_Safety_Snapshot__ *__Snapshot__)
{
    if (__Snapshot__ == NULL)
    {
        return;
    }

    free(__Snapshot__->__States__);
    __Snapshot__->__States__ = NULL;
    __Snapshot__->__Count__ = 0U;
}

/* Returns the body safety move atom. */
int __Body_Safety_Move_Atom__(__Semantic_Body_Context__ *__Context__,
                              const __Ast_Atom__ *__Atom__,
                              __Source_Span__ __Span__)
{
    /* References the lvalue. */
    __Ast_Lvalue__ *__Lvalue__;
    /* References the source. */
    __Semantic_Local__ *__Source__;

    if (__Atom__ == NULL || __Atom__->__Kind__ != __Ast_Atom_Lvalue__)
    {
        return 1;
    }
    __Lvalue__ = __Atom__->__As__.__Lvalue__;
    if (__Lvalue__ == NULL)
    {
        return 1;
    }
    if (__Lvalue__->__Kind__ != __Ast_Lvalue_Base__)
    {
        /* References the projected type. */
        __Ast_Type__ *__Projected_Type__ = NULL;

        /* Reject moves that would split ownership of an aggregate. */
        if (!__Body_Infer_Lvalue__(__Context__, __Lvalue__, &__Projected_Type__, NULL))
        {
            return 0;
        }
        if (__Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Projected_Type__))
        {
            return __Body_Fail__(__Context__, __E1312_Partial_Owned_Move__, __Span__);
        }
        return 1;
    }
    __Source__ = __Body_Find_Local__(__Context__, __Lvalue__);
    if (__Source__ == NULL ||
        !__Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Source__->__Type__))
    {
        return 1;
    }
    if (!__Safety_Fact_Can_Move__(&__Source__->__Safety__))
    {
        if (__Source__->__Safety__.__Mutable_Borrow__ ||
            __Source__->__Safety__.__Immutable_Borrows__ != 0U)
        {
            return __Body_Report_Borrow_Conflict__(__Context__, __Source__, __Span__);
        }
        return __Body_Fail__(__Context__, __E1301_Use_After_Move__, __Span__);
    }
    __Safety_Fact_Mark_Moved_At__(&__Source__->__Safety__, __Span__);
    return 1;
}

/* Returns the body safety move expression. */
int __Body_Safety_Move_Expression__(__Semantic_Body_Context__ *__Context__,
                                    __Ast_Expression__ *__Expression__,
                                    __Semantic_Local__ *__Destination__)
{
    /* References the lvalue. */
    __Ast_Lvalue__ *__Lvalue__ = NULL;
    /* References the source. */
    __Semantic_Local__ *__Source__ = NULL;

    if (__Expression__ == NULL || __Expression__->__Kind__ != __Ast_Expression_Atom__ ||
        __Expression__->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__)
    {
        return 1;
    }

    __Lvalue__ = __Expression__->__As__.__Atom__.__As__.__Lvalue__;
    if (__Lvalue__ == NULL)
    {
        return 1;
    }
    if (__Lvalue__->__Kind__ != __Ast_Lvalue_Base__)
    {
        return __Body_Safety_Move_Atom__(
            __Context__, &__Expression__->__As__.__Atom__, __Expression__->__Header__.__Span__);
    }

    __Source__ = __Body_Find_Local__(__Context__, __Lvalue__);
    if (__Source__ == NULL || __Source__ == __Destination__ ||
        !__Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Source__->__Type__))
    {
        return 1;
    }

    return __Body_Safety_Move_Atom__(
        __Context__, &__Expression__->__As__.__Atom__, __Expression__->__Header__.__Span__);
}
