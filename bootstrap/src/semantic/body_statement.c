/* Checks statements and block flow. */

#include "semantic/body_internal.h"

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

            if (!__Body_Safety_Transfer_Expression__(
                    __Context__, __Statement__->__As__.__Copy__.__Expression__, __Local__))
            {
                return 0;
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

            if (!__Body_Infer_Expression__(__Context__,
                                           __Statement__->__As__.__While__.__Condition__,
                                           &__Condition_Type__) ||
                !__Body_Is_Bool_Convertible__(__Context__, __Condition_Type__))
            {
                return __Body_Fail__(
                    __Context__, __E0400_Mismatched_Types__, __Statement__->__Header__.__Span__);
            }

            if (!__Body_Safety_Capture__(__Context__, &__Before__) ||
                !__Body_Check_Block_Flow__(
                    __Context__, __Statement__->__As__.__While__.__Body__, &__Body_Falls__) ||
                !__Body_Safety_Capture__(__Context__, &__After_Body__))
            {
                __Body_Safety_Snapshot_Destroy__(&__Before__);
                __Body_Safety_Snapshot_Destroy__(&__After_Body__);
                return 0;
            }

            __Body_Safety_Restore__(__Context__, &__Before__);
            if (__Body_Falls__)
            {
                __Body_Safety_Merge__(__Context__, &__Before__, &__After_Body__);
            }
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
