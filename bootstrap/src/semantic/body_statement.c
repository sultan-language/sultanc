/* Checks statements and block flow. */

#include "semantic/body_internal.h"
#include "semantic/never_return.h"

#include <string.h>

/* Answers whether EXPRESSION calls the never-returning stop primitive. */
static int __Body_Call_Never_Returns__(__Semantic_Body_Context__ *__Context__,
                                       __Ast_Expression__ *__Expression__)
{
    if (__Expression__ == NULL || __Expression__->__Kind__ != __Ast_Expression_Call__ ||
        __Body_Find_Local__(__Context__, __Expression__->__As__.__Call__.__Function__) != NULL)
    {
        return 0;
    }
    return __Semantic_Call_Never_Returns__(
        __Context__->__Semantic__, __Context__->__Function__->__Unit__, __Expression__);
}

/* Records the state reaching the innermost loop's exit from a `break` or
 * `continue`: the loop body's own locals end here, so their borrows are
 * released in the recorded state (the current state is left unchanged; the
 * rest of the block is unreachable). */
static int __Body_Record_Loop_Exit__(__Semantic_Body_Context__ *__Context__, __Source_Span__ __Span__)
{
    __Body_Loop_Frame__ *__Frame__ = __Context__->__Loop__;
    __Body_Safety_Snapshot__ __Current__ = {0};
    __Body_Safety_Snapshot__ __Exit__ = {0};
    if (__Frame__ == NULL)
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Span__);
    }
    if (!__Body_Safety_Capture__(__Context__, &__Current__))
    {
        return 0;
    }
    __Body_Safety_Release_Scope__(__Context__, __Frame__->__Locals_At_Entry__);
    if (!__Body_Safety_Capture__(__Context__, &__Exit__))
    {
        __Body_Safety_Snapshot_Destroy__(&__Current__);
        return 0;
    }
    if (__Frame__->__Has_Exits__)
    {
        __Body_Safety_Merge__(__Context__, &__Frame__->__Exits__, &__Exit__);
        __Body_Safety_Snapshot_Destroy__(&__Frame__->__Exits__);
        __Body_Safety_Snapshot_Destroy__(&__Exit__);
        if (!__Body_Safety_Capture__(__Context__, &__Frame__->__Exits__))
        {
            __Body_Safety_Snapshot_Destroy__(&__Current__);
            return 0;
        }
    }
    else
    {
        __Frame__->__Exits__ = __Exit__;
        __Frame__->__Has_Exits__ = 1;
    }
    __Body_Safety_Restore__(__Context__, &__Current__);
    __Body_Safety_Snapshot_Destroy__(&__Current__);
    __Context__->__Falls_Through__ = 0;
    return 1;
}

/* Checks the body statement. */
static int __Body_Check_Statement__(__Semantic_Body_Context__ *__Context__,
                                    __Ast_Statement__ *__Statement__)
{
    if (__Statement__ == NULL)
    {
        return 1;
    }

    switch (__Statement__->__Kind__)
    {
        case __Ast_Statement_Variable_Declaration__:
            return __Body_Declare_Local__(__Context__,
                                          &__Statement__->__As__.__Variable__,
                                          __Statement__->__Header__.__Span__);

        case __Ast_Statement_Copy__:
        {
            /* References the destination type. */
            __Ast_Type__ *__Destination_Type__ = NULL;
            /* References the value type. */
            __Ast_Type__ *__Value_Type__ = NULL;
            /* References the local. */
            __Semantic_Local__ *__Local__ = NULL;
            /* Stores the expression already inferred. */
            int __Expression_Already_Inferred__ = 0;
            /* Stores who owns the assigned value (for a plain local destination). */
            __Semantic_Ownership__ __Assigned_Ownership__ = __Semantic_Ownership_Owner__;
            /* Tracks whether the destination is a plain local. */
            int __Destination_Is_Local__ =
                __Statement__->__As__.__Copy__.__Destination__->__Kind__ == __Ast_Lvalue_Base__;

            if (__Statement__->__As__.__Copy__.__Destination__->__Kind__ == __Ast_Lvalue_Base__)
            {
                __Local__ = __Body_Find_Local__(__Context__,
                                                __Statement__->__As__.__Copy__.__Destination__);
                if (__Local__ != NULL && __Local__->__Type__ == NULL)
                {
                    if (!__Body_Infer_Expression__(__Context__,
                                                   __Statement__->__As__.__Copy__.__Expression__,
                                                   &__Value_Type__))
                    {
                        return 0;
                    }
                    __Local__->__Type__ = __Value_Type__;
                    __Local__->__Mutable__ = 1;
                    /* Avoid rechecking call temporaries after ownership effects are applied. */
                    __Expression_Already_Inferred__ = 1;
                    if (__Local__->__Slot__ != NULL)
                    {
                        __Local__->__Slot__->__Type__ = __Value_Type__;
                    }
                }
            }

            if (!__Body_Check_Assignable__(__Context__,
                                           __Statement__->__As__.__Copy__.__Destination__,
                                           &__Destination_Type__,
                                           &__Local__) ||
                (!__Expression_Already_Inferred__ &&
                 !__Body_Check_Expression_Compatible__(
                     __Context__,
                     __Destination_Type__,
                     __Statement__->__As__.__Copy__.__Expression__,
                     __Statement__->__Header__.__Span__)))
            {
                return 0;
            }

            /* Classified before the transfer: a move marks its source moved. */
            if (__Destination_Is_Local__ &&
                !__Body_Expression_Ownership__(
                    __Context__, __Statement__->__As__.__Copy__.__Expression__,
                    &__Assigned_Ownership__))
            {
                return 0;
            }
            __Context__->__Transfer_Into_Plain_Local__ = __Destination_Is_Local__;
            if (!__Body_Safety_Transfer_Expression__(
                    __Context__, __Statement__->__As__.__Copy__.__Expression__, __Local__))
            {
                __Context__->__Transfer_Into_Plain_Local__ = 0;
                return 0;
            }
            __Context__->__Transfer_Into_Plain_Local__ = 0;
            if (__Local__ != NULL && __Destination_Is_Local__)
            {
                /* Mirrors the drop lowering's per-copy borrowed provenance. An
                 * immutable local that any copy assigns a borrowed value may
                 * hold a borrowed view (a deferred-init local is assigned on
                 * several branches), so it is classed as borrowed and cannot
                 * be moved out; the per-copy drop flag still destroys an owned
                 * copy. Only a local whose every copy is owned is an owner.
                 * Mutable locals stay owners: a later reassignment (also in a
                 * loop) could make them own. */
                if (!__Local__->__Mutable__ &&
                    (__Assigned_Ownership__ == __Semantic_Ownership_Borrowed__ ||
                     (__Local__->__Ownership_Assigned__ &&
                      __Local__->__Ownership__ == __Semantic_Ownership_Borrowed__)))
                {
                    __Local__->__Ownership__ = __Semantic_Ownership_Borrowed__;
                }
                else
                {
                    __Local__->__Ownership__ = __Semantic_Ownership_Owner__;
                }
                __Local__->__Ownership_Assigned__ = 1;
            }
            if (__Local__ != NULL)
            {
                /* References the assigned expression. */
                __Ast_Expression__ *__Assigned_Expression__ =
                    __Statement__->__As__.__Copy__.__Expression__;

                if (__Assigned_Expression__ != NULL &&
                    __Assigned_Expression__->__Kind__ == __Ast_Expression_Atom__ &&
                    __Assigned_Expression__->__As__.__Atom__.__Kind__ == __Ast_Atom_Literal__ &&
                    __Assigned_Expression__->__As__.__Atom__.__As__.__Literal__ != NULL &&
                    __Assigned_Expression__->__As__.__Atom__.__As__.__Literal__->__Kind__ ==
                        __Ast_Literal_String__)
                {
                    __Local__->__Has_Known_Length__ = 1;
                    __Local__->__Known_Length__ = __Assigned_Expression__->__As__.__Atom__.__As__
                                                      .__Literal__->__As__.__String__.__Length__;
                }
                else
                {
                    /* Flow metadata must never outlive the value that established it. */
                    __Local__->__Has_Known_Length__ = 0;
                    __Local__->__Known_Length__ = 0U;
                }
                __Safety_Fact_Mark_Initialized_At__(&__Local__->__Safety__,
                                                    __Statement__->__Header__.__Span__);
            }
            /* A call statement of the stop primitive ends the path: a function
             * returning a value needs no return after it (docs/ERRORS.md). */
            if (__Statement__->__As__.__Copy__.__Destination__->__Kind__ == __Ast_Lvalue_Base__ &&
                __Statement__->__As__.__Copy__.__Destination__->__As__.__Base__.__Kind__ ==
                    __Ast_Lvalue_Base_Temporary__ &&
                __Body_Call_Never_Returns__(__Context__,
                                            __Statement__->__As__.__Copy__.__Expression__))
            {
                __Context__->__Falls_Through__ = 0;
            }
            return 1;
        }

        case __Ast_Statement_Initialize_Record__:
            return __Body_Check_Record_Init__(__Context__, __Statement__);
        case __Ast_Statement_Initialize_Vector__:
            return __Body_Check_Vector_Init__(__Context__, __Statement__);
        case __Ast_Statement_Initialize_Box__:
            return __Body_Check_Box_Init__(__Context__, __Statement__);

        case __Ast_Statement_While__:
        {
            /* References the condition type. */
            __Ast_Type__ *__Condition_Type__ = NULL;
            /* Stores the before. */
            __Body_Safety_Snapshot__ __Before__ = {0};
            /* Stores the after body. */
            __Body_Safety_Snapshot__ __After_Body__ = {0};
            /* Tracks the body falls state. */
            int __Body_Falls__ = 1;
            /* Stores the loop frame. */
            __Body_Loop_Frame__ __Frame__;
            /* Tracks the step falls state. */
            int __Step_Falls__ = 1;

            if (!__Body_Infer_Expression__(__Context__,
                                           __Statement__->__As__.__While__.__Condition__,
                                           &__Condition_Type__) ||
                !__Body_Is_Bool_Convertible__(__Context__, __Condition_Type__))
            {
                return __Body_Fail__(
                    __Context__, __E0400_Mismatched_Types__, __Statement__->__Header__.__Span__);
            }

            {
                /* A length known before the loop may change on a later
                 * iteration (an append at the end of the body): no static
                 * bound holds inside the loop. */
                size_t __Local_Index__;
                for (__Local_Index__ = 0U; __Local_Index__ < __Context__->__Locals__.__Count__;
                     ++__Local_Index__)
                {
                    __Semantic_Local__ *__Known__ =
                        (__Semantic_Local__ *)__Vector_At__(&__Context__->__Locals__, __Local_Index__);
                    if (__Known__ != NULL)
                        __Known__->__Has_Known_Length__ = 0;
                }
            }
            memset(&__Frame__, 0, sizeof(__Frame__));
            __Frame__.__Outer__ = __Context__->__Loop__;
            __Frame__.__Locals_At_Entry__ = __Context__->__Locals__.__Count__;
            __Context__->__Loop__ = &__Frame__;
            if (!__Body_Safety_Capture__(__Context__, &__Before__) ||
                !__Body_Check_Block_Flow__(
                    __Context__, __Statement__->__As__.__While__.__Body__, &__Body_Falls__))
            {
                __Context__->__Loop__ = __Frame__.__Outer__;
                __Body_Safety_Snapshot_Destroy__(&__Frame__.__Exits__);
                __Body_Safety_Snapshot_Destroy__(&__Before__);
                return 0;
            }
            /* The step of a `لكل` loop runs after the body and after continue. */
            if (__Statement__->__As__.__While__.__Step__ != NULL)
            {
                if (!__Body_Falls__ && __Frame__.__Has_Exits__)
                {
                    __Body_Safety_Restore__(__Context__, &__Frame__.__Exits__);
                }
                __Context__->__Loop__ = __Frame__.__Outer__;
                if (!__Body_Check_Block_Flow__(
                        __Context__, __Statement__->__As__.__While__.__Step__, &__Step_Falls__))
                {
                    __Body_Safety_Snapshot_Destroy__(&__Frame__.__Exits__);
                    __Body_Safety_Snapshot_Destroy__(&__Before__);
                    return 0;
                }
                __Body_Falls__ = 1;
            }
            __Context__->__Loop__ = __Frame__.__Outer__;
            if (!__Body_Safety_Capture__(__Context__, &__After_Body__))
            {
                __Body_Safety_Snapshot_Destroy__(&__Frame__.__Exits__);
                __Body_Safety_Snapshot_Destroy__(&__Before__);
                return 0;
            }

            __Body_Safety_Restore__(__Context__, &__Before__);
            if (__Body_Falls__)
            {
                __Body_Safety_Merge__(__Context__, &__Before__, &__After_Body__);
            }
            if (__Frame__.__Has_Exits__)
            {
                /* Stores the state merged so far. */
                __Body_Safety_Snapshot__ __Merged__ = {0};
                if (!__Body_Safety_Capture__(__Context__, &__Merged__))
                {
                    __Body_Safety_Snapshot_Destroy__(&__Frame__.__Exits__);
                    __Body_Safety_Snapshot_Destroy__(&__Before__);
                    __Body_Safety_Snapshot_Destroy__(&__After_Body__);
                    return 0;
                }
                __Body_Safety_Merge__(__Context__, &__Merged__, &__Frame__.__Exits__);
                __Body_Safety_Snapshot_Destroy__(&__Merged__);
            }
            __Body_Safety_Snapshot_Destroy__(&__Frame__.__Exits__);
            __Body_Safety_Snapshot_Destroy__(&__Before__);
            __Body_Safety_Snapshot_Destroy__(&__After_Body__);

            /* A while loop may execute zero times unless a future proof says otherwise. */
            __Context__->__Falls_Through__ = 1;
            return 1;
        }

        case __Ast_Statement_If__:
        {
            /* References the condition type. */
            __Ast_Type__ *__Condition_Type__ = NULL;
            /* Stores the before. */
            __Body_Safety_Snapshot__ __Before__ = {0};
            /* Stores the then. */
            __Body_Safety_Snapshot__ __Then__ = {0};
            /* Stores the else. */
            __Body_Safety_Snapshot__ __Else__ = {0};
            /* Tracks the then falls state. */
            int __Then_Falls__ = 1;
            /* Tracks the else falls state. */
            int __Else_Falls__ = 1;

            if (!__Body_Infer_Expression__(
                    __Context__, __Statement__->__As__.__If__.__Condition__, &__Condition_Type__) ||
                !__Body_Is_Bool_Convertible__(__Context__, __Condition_Type__))
            {
                return __Body_Fail__(
                    __Context__, __E0400_Mismatched_Types__, __Statement__->__Header__.__Span__);
            }

            if (!__Body_Safety_Capture__(__Context__, &__Before__) ||
                !__Body_Check_Block_Flow__(
                    __Context__, __Statement__->__As__.__If__.__Then__, &__Then_Falls__) ||
                !__Body_Safety_Capture__(__Context__, &__Then__))
            {
                __Body_Safety_Snapshot_Destroy__(&__Before__);
                __Body_Safety_Snapshot_Destroy__(&__Then__);
                return 0;
            }

            __Body_Safety_Restore__(__Context__, &__Before__);
            if (__Statement__->__As__.__If__.__Else__ != NULL)
            {
                if (!__Body_Check_Block_Flow__(
                        __Context__, __Statement__->__As__.__If__.__Else__, &__Else_Falls__) ||
                    !__Body_Safety_Capture__(__Context__, &__Else__))
                {
                    __Body_Safety_Snapshot_Destroy__(&__Before__);
                    __Body_Safety_Snapshot_Destroy__(&__Then__);
                    __Body_Safety_Snapshot_Destroy__(&__Else__);
                    return 0;
                }
            }
            else if (!__Body_Safety_Capture__(__Context__, &__Else__))
            {
                __Body_Safety_Snapshot_Destroy__(&__Before__);
                __Body_Safety_Snapshot_Destroy__(&__Then__);
                return 0;
            }

            if (__Then_Falls__ && __Else_Falls__)
            {
                __Body_Safety_Merge__(__Context__, &__Then__, &__Else__);
            }
            else if (__Then_Falls__)
            {
                __Body_Safety_Restore__(__Context__, &__Then__);
            }
            else if (__Else_Falls__)
            {
                __Body_Safety_Restore__(__Context__, &__Else__);
            }
            else
            {
                __Body_Safety_Restore__(__Context__, &__Before__);
            }

            __Context__->__Falls_Through__ = __Then_Falls__ || __Else_Falls__;
            __Body_Safety_Snapshot_Destroy__(&__Before__);
            __Body_Safety_Snapshot_Destroy__(&__Then__);
            __Body_Safety_Snapshot_Destroy__(&__Else__);
            return 1;
        }

        case __Ast_Statement_Match__:
        {
            /* Tracks the match falls state. */
            int __Match_Falls__ = 1;
            if (!__Body_Check_Match__(__Context__, __Statement__, &__Match_Falls__))
            {
                return 0;
            }
            __Context__->__Falls_Through__ = __Match_Falls__;
            return 1;
        }

        case __Ast_Statement_Break__:
        case __Ast_Statement_Continue__:
            return __Body_Record_Loop_Exit__(__Context__, __Statement__->__Header__.__Span__);

        case __Ast_Statement_Block__:
        {
            int __Block_Falls__ = 1;
            if (!__Body_Check_Block_Flow__(
                    __Context__, __Statement__->__As__.__Block__, &__Block_Falls__))
            {
                return 0;
            }
            __Context__->__Falls_Through__ = __Block_Falls__;
            return 1;
        }

        case __Ast_Statement_Unsafe__:
        {
            int __Unsafe_Falls__ = 1;
            ++__Context__->__Unsafe_Depth__;
            if (!__Body_Check_Block_Flow__(
                    __Context__, __Statement__->__As__.__Unsafe__, &__Unsafe_Falls__))
            {
                --__Context__->__Unsafe_Depth__;
                return 0;
            }
            --__Context__->__Unsafe_Depth__;
            __Context__->__Falls_Through__ = __Unsafe_Falls__;
            return 1;
        }

        case __Ast_Statement_Return__:
        {
            /* References the expected. */
            __Ast_Type__ *__Expected__ =
                __Context__->__Function__->__Function__->__Output__.__Type__;
            /* Stores the resolved. */
            __Resolved_Type__ __Resolved__;

            if (!__Type_Resolve__(__Context__->__Semantic__, __Expected__, &__Resolved__))
            {
                return 0;
            }

            if (__Statement__->__As__.__Return__ == NULL)
            {
                if (__Resolved__.__Kind__ != __Resolved_Type_Void__)
                {
                    return __Body_Fail__(__Context__,
                                         __E0400_Mismatched_Types__,
                                         __Statement__->__Header__.__Span__);
                }
                __Context__->__Falls_Through__ = 0;
                return 1;
            }

            if (__Resolved__.__Kind__ == __Resolved_Type_Void__)
            {
                return __Body_Fail__(
                    __Context__, __E0400_Mismatched_Types__, __Statement__->__Header__.__Span__);
            }

            if (!__Body_Check_Expression_Compatible__(__Context__,
                                                      __Expected__,
                                                      __Statement__->__As__.__Return__,
                                                      __Statement__->__Header__.__Span__))
            {
                return 0;
            }

            if (!__Body_Safety_Check_Return_Reference__(__Context__,
                                                        __Statement__->__As__.__Return__) ||
                !__Body_Safety_Check_Return_Composite_View__(__Context__,
                                                             __Statement__->__As__.__Return__))
            {
                return 0;
            }

            if (__Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Expected__) &&
                !__Safety_Type_Is_Reference__(__Expected__, NULL) &&
                !__Body_Safety_Move_Expression__(
                    __Context__, __Statement__->__As__.__Return__, NULL))
            {
                return 0;
            }
            __Context__->__Falls_Through__ = 0;
            return 1;
        }
    }

    return __Body_Fail__(__Context__,
                         __E0404_Statement_Typecheck_Not_Implemented__,
                         __Statement__->__Header__.__Span__);
}

/* Checks the body block flow. */
int __Body_Check_Block_Flow__(__Semantic_Body_Context__ *__Context__,
                              __Ast_Block__ *__Block__,
                              int *__Out_Falls_Through__)
{
    /* Stores the saved count. */
    size_t __Saved_Count__ = __Context__->__Locals__.__Count__;
    /* Tracks the index. */
    size_t __Index__ = 0U;
    /* Tracks the falls state. */
    int __Falls__ = 1;

    ++__Context__->__Scope_Depth__;
    for (__Index__ = 0U; __Index__ < __Block__->__Statement_Count__; ++__Index__)
    {
        if (!__Falls__)
        {
            break;
        }

        __Context__->__Falls_Through__ = 1;
        if (!__Body_Check_Statement__(__Context__, __Block__->__Statements__[__Index__]))
        {
            __Body_Safety_Release_Scope__(__Context__, __Saved_Count__);
            __Context__->__Locals__.__Count__ = __Saved_Count__;
            --__Context__->__Scope_Depth__;
            return 0;
        }
        __Falls__ = __Context__->__Falls_Through__;
    }

    __Body_Safety_Release_Scope__(__Context__, __Saved_Count__);
    __Context__->__Locals__.__Count__ = __Saved_Count__;
    --__Context__->__Scope_Depth__;
    __Context__->__Falls_Through__ = __Falls__;
    if (__Out_Falls_Through__ != NULL)
    {
        *__Out_Falls_Through__ = __Falls__;
    }
    return 1;
}

/* Checks the body block. */
int __Body_Check_Block__(__Semantic_Body_Context__ *__Context__, __Ast_Block__ *__Block__)
{
    /* Tracks the falls state. */
    int __Falls__ = 1;
    return __Body_Check_Block_Flow__(__Context__, __Block__, &__Falls__);
}
