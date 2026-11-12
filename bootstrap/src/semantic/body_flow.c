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
        if (__Context__->__Transfer_Into_Plain_Local__ &&
            __Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Projected_Type__))
        {
            /* A projection through a reference (`(*r).f`, `(*r)[i]`) copied
             * into a plain local is a borrowed view, as `let x: T = *r;` is
             * (docs/en/reference/ownership.md, "Borrowed views are not
             * moves"): it reads the borrowed storage in place and moves
             * nothing. The local is classed as borrowed by the copy, and
             * the drop lowering keeps it non-owning. */
            __Semantic_Ownership__ __Ownership__ = __Semantic_Ownership_Owner__;
            if (!__Body_Lvalue_Ownership__(__Context__, __Lvalue__, &__Ownership__))
            {
                return 0;
            }
            if (__Ownership__ == __Semantic_Ownership_Borrowed__)
            {
                return 1;
            }
        }
        if (__Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Projected_Type__) &&
            !(__Lvalue__->__Kind__ == __Ast_Lvalue_Field__ &&
              __Lvalue__->__As__.__Field__.__Parent__ != NULL &&
              __Lvalue__->__As__.__Field__.__Parent__->__Kind__ == __Ast_Lvalue_Base__ &&
              __Lvalue__->__As__.__Field__.__Parent__->__As__.__Base__.__Kind__ ==
                  __Ast_Lvalue_Base_Call__))
        {
            /* (An owned field of a call result moves out: lowering destroys
             * the temporary's other owned fields, as Rust does.) */
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
    /* A pattern binding of a match on a field, element or box payload is
     * still owned by the enclosing aggregate: moving it out would split that
     * aggregate's ownership exactly like moving the projection itself. */
    if (__Source__->__Ownership__ == __Semantic_Ownership_Projected__)
    {
        return __Body_Fail__(__Context__, __E1312_Partial_Owned_Move__, __Span__);
    }
    /* A borrowed view (a binding of a match on a place read through a
     * reference, or an immutable local copied from one) owns nothing. Only a
     * copy into a plain local keeps it non-owning; a call argument, return
     * value or aggregate member would become a second owner of storage the
     * referenced aggregate still owns. */
    if (__Source__->__Ownership__ == __Semantic_Ownership_Borrowed__ &&
        !__Context__->__Transfer_Into_Plain_Local__)
    {
        return __Body_Fail__(__Context__, __E1312_Partial_Owned_Move__, __Span__);
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
    /* Exports this choke point's already-computed move fact onto the AST
     * node itself, so LLVM lowering (a separate pass that no longer has
     * __Context__'s per-local safety facts) can clear the matching runtime
     * drop flag without recomputing ownership flow analysis. */
    __Lvalue__->__Drop_Elaboration_Moved__ = 1;
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

/* Classifies who owns the value `__Lvalue__` names. A base local reports its
 * own class. A projection (field, element, dereference) is borrowed when any
 * step reads through a reference (mirroring the drop lowering's
 * reads-through-reference rule), and otherwise belongs to the enclosing owned
 * place: it is projected. Assumes `__Lvalue__` has already been inferred. */
int __Body_Lvalue_Ownership__(__Semantic_Body_Context__ *__Context__,
                              __Ast_Lvalue__ *__Lvalue__,
                              __Semantic_Ownership__ *__Out_Ownership__)
{
    /* References the parent this projection reads from. */
    __Ast_Lvalue__ *__Parent__ = NULL;
    /* References the parent type. */
    __Ast_Type__ *__Parent_Type__ = NULL;
    /* Stores the resolved parent type. */
    __Resolved_Type__ __Resolved__;
    /* Stores the parent's ownership class. */
    __Semantic_Ownership__ __Parent_Ownership__ = __Semantic_Ownership_Owner__;

    *__Out_Ownership__ = __Semantic_Ownership_Owner__;
    if (__Lvalue__ == NULL)
    {
        return 1;
    }
    switch (__Lvalue__->__Kind__)
    {
        case __Ast_Lvalue_Base__:
        {
            /* References the local, if the base names one. */
            __Semantic_Local__ *__Local__ = __Body_Find_Local__(__Context__, __Lvalue__);
            __Semantic_Global_Entry__ *__Global__ = NULL;
            if (__Local__ != NULL)
            {
                *__Out_Ownership__ = __Local__->__Ownership__;
            }
            else if (__Lvalue__->__As__.__Base__.__Kind__ == __Ast_Lvalue_Base_Identifier__ &&
                     __Name_Resolve_Global__(__Context__->__Semantic__,
                                             __Context__->__Function__->__Unit__,
                                             __Lvalue__->__As__.__Base__.__As__.__Identifier__,
                                             &__Global__) == __Name_Lookup_Found__ &&
                     __Global__ != NULL && __Global__->__Declaration__ != NULL &&
                     __Global__->__Declaration__->__Type__ != NULL &&
                     __Global__->__Declaration__->__Type__->__Kind__ == __Ast_Type_Mutable__)
            {
                /* A module variable keeps its value: a read by value is a
                 * view of storage the module still owns (an immutable module
                 * constant's storage is static, so its reads are fresh). */
                *__Out_Ownership__ = __Semantic_Ownership_Borrowed__;
            }
            return 1;
        }
        case __Ast_Lvalue_Field__:
            __Parent__ = __Lvalue__->__As__.__Field__.__Parent__;
            break;
        case __Ast_Lvalue_Index__:
            __Parent__ = __Lvalue__->__As__.__Index__.__Parent__;
            break;
        case __Ast_Lvalue_Dereference__:
            __Parent__ = __Lvalue__->__As__.__Dereference_Parent__;
            break;
        default:
            return 1;
    }
    if (!__Body_Infer_Lvalue__(__Context__, __Parent__, &__Parent_Type__, NULL) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Parent_Type__, &__Resolved__))
    {
        return 0;
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Reference__)
    {
        *__Out_Ownership__ = __Semantic_Ownership_Borrowed__;
        return 1;
    }
    if (!__Body_Lvalue_Ownership__(__Context__, __Parent__, &__Parent_Ownership__))
    {
        return 0;
    }
    *__Out_Ownership__ = __Parent_Ownership__ == __Semantic_Ownership_Borrowed__
                             ? __Semantic_Ownership_Borrowed__
                             : __Semantic_Ownership_Projected__;
    return 1;
}

/* Classifies who owns the value `__Expression__` yields. Only a place can be
 * projected. A unary dereference yields a borrowed view when its operand is
 * a reference or is itself borrowed; dereferencing an owned Box consumes it
 * (a projected Box with an owned payload is rejected in
 * __Body_Infer_Unary__). Calls, literals and constructions are fresh. */
int __Body_Expression_Ownership__(__Semantic_Body_Context__ *__Context__,
                                  __Ast_Expression__ *__Expression__,
                                  __Semantic_Ownership__ *__Out_Ownership__)
{
    *__Out_Ownership__ = __Semantic_Ownership_Owner__;
    if (__Expression__ == NULL)
    {
        return 1;
    }
    if (__Expression__->__Kind__ == __Ast_Expression_Atom__)
    {
        if (__Expression__->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__)
        {
            return 1;
        }
        return __Body_Lvalue_Ownership__(
            __Context__, __Expression__->__As__.__Atom__.__As__.__Lvalue__, __Out_Ownership__);
    }
    if (__Expression__->__Kind__ == __Ast_Expression_Unary__ &&
        __Expression__->__As__.__Unary__.__Operation__ == __Unary_Dereference__)
    {
        /* References the dereferenced operand. */
        __Ast_Expression__ *__Operand__ = __Expression__->__As__.__Unary__.__Operand__;
        /* Stores the operand's ownership class. */
        __Semantic_Ownership__ __Operand_Ownership__ = __Semantic_Ownership_Owner__;

        if (!__Body_Expression_Ownership__(__Context__, __Operand__, &__Operand_Ownership__))
        {
            return 0;
        }
        if (__Operand_Ownership__ == __Semantic_Ownership_Borrowed__)
        {
            *__Out_Ownership__ = __Semantic_Ownership_Borrowed__;
            return 1;
        }
        if (__Operand__ != NULL && __Operand__->__Kind__ == __Ast_Expression_Atom__ &&
            __Operand__->__As__.__Atom__.__Kind__ == __Ast_Atom_Lvalue__)
        {
            /* References the operand type. */
            __Ast_Type__ *__Operand_Type__ = NULL;
            /* Stores the resolved operand type. */
            __Resolved_Type__ __Resolved__;
            if (!__Body_Infer_Lvalue__(__Context__,
                                       __Operand__->__As__.__Atom__.__As__.__Lvalue__,
                                       &__Operand_Type__,
                                       NULL) ||
                !__Type_Resolve__(__Context__->__Semantic__, __Operand_Type__, &__Resolved__))
            {
                return 0;
            }
            if (__Resolved__.__Kind__ == __Resolved_Type_Reference__)
            {
                *__Out_Ownership__ = __Semantic_Ownership_Borrowed__;
            }
        }
        return 1;
    }
    return 1;
}
