/* Resolves lvalue types and access rules. */

#include "semantic/body_internal.h"
#include "kernel/name/name.h"

#include <stdio.h>

/* Checks the body local read. */
static int __Body_Check_Local_Read__(__Semantic_Body_Context__ *__Context__,
                                     __Semantic_Local__ *__Local__,
                                     __Source_Span__ __Span__)
{
    if (__Local__ == NULL)
    {
        return 1;
    }

    switch (__Local__->__Safety__.__State__)
    {
        case __Safety_Value_Initialized__:
            if (!__Safety_Fact_Can_Read__(&__Local__->__Safety__))
            {
                return __Body_Report_Borrow_Conflict__(__Context__, __Local__, __Span__);
            }
            return 1;
        case __Safety_Value_Moved__:
        case __Safety_Value_Maybe_Moved__:
        {
            /* References the diagnostic state. */
            __Diagnostic__ *__Diagnostic_State__ =
                __Body_Begin_Diagnostic__(__Context__, __E1301_Use_After_Move__, __Span__);
            if (__Diagnostic_State__ != NULL)
            {
                __Diagnostic_Set_Message_Key__(__Diagnostic_State__,
                                               __Diag_Word_Use_After_Move_Causal__);
                __Diagnostic_Set_Primary_Role__(__Diagnostic_State__,
                                                __Diagnostic_Span_Role_Invalid_Use__);
                if (__Local__->__Name_Kind__ == __Ast_Lvalue_Base_Identifier__)
                {
                    __Diagnostic_Set_Argument_Text__(
                        __Diagnostic_State__, __Diagnostic_Argument_Value__, __Local__->__Name__);
                }
                if (__Local__->__Safety__.__Has_Origin_Span__)
                {
                    __Diagnostic_Add_Related_Span__(__Diagnostic_State__,
                                                    __Local__->__Safety__.__Origin_Span__,
                                                    __Diagnostic_Span_Role_Original_Owner__);
                    __Diagnostic_Add_Trace__(__Diagnostic_State__,
                                             __Diag_Word_Trace_Value_Created__,
                                             &__Local__->__Safety__.__Origin_Span__);
                }
                if (__Local__->__Safety__.__Has_Move_Span__)
                {
                    __Diagnostic_Add_Related_Span__(__Diagnostic_State__,
                                                    __Local__->__Safety__.__Move_Span__,
                                                    __Diagnostic_Span_Role_Move_Origin__);
                    __Diagnostic_Add_Trace__(__Diagnostic_State__,
                                             __Diag_Word_Trace_Value_Moved__,
                                             &__Local__->__Safety__.__Move_Span__);
                }
                __Diagnostic_Add_Trace__(
                    __Diagnostic_State__, __Diag_Word_Trace_Value_Used__, &__Span__);
                __Diagnostic_Add_Help__(__Diagnostic_State__, __Diag_Word_Help_Borrow_Instead__);
            }
            return 0;
        }
        case __Safety_Value_Uninitialized__:
        case __Safety_Value_Maybe_Uninitialized__:
        {
            /* References the diagnostic state. */
            __Diagnostic__ *__Diagnostic_State__ = __Body_Begin_Diagnostic__(
                __Context__, __E1300_Use_Before_Initialization__, __Span__);
            if (__Diagnostic_State__ != NULL)
            {
                __Diagnostic_Set_Primary_Role__(__Diagnostic_State__,
                                                __Diagnostic_Span_Role_Invalid_Use__);
                if (__Local__->__Name_Kind__ == __Ast_Lvalue_Base_Identifier__)
                {
                    __Diagnostic_Set_Message_Key__(__Diagnostic_State__,
                                                   __Diag_Word_Use_Before_Init_Causal__);
                    __Diagnostic_Set_Argument_Text__(
                        __Diagnostic_State__, __Diagnostic_Argument_Value__, __Local__->__Name__);
                }
                __Diagnostic_Add_Related_Span__(__Diagnostic_State__,
                                                __Local__->__Declaration_Span__,
                                                __Diagnostic_Span_Role_Uninitialized_Declaration__);
            }
            return 0;
        }
    }

    return 0;
}

/* Infers the body lvalue access. */
static int __Body_Infer_Lvalue_Access__(__Semantic_Body_Context__ *__Context__,
                                        __Ast_Lvalue__ *__Lvalue__,
                                        __Ast_Type__ **__Out_Type__,
                                        __Semantic_Local__ **__Out_Local__,
                                        int __Require_Read__)
{
    if (__Out_Local__ != NULL)
    {
        *__Out_Local__ = NULL;
    }

    if (__Lvalue__ == NULL)
    {
        return __Body_Fail__(__Context__, __E0300_Unknown_Name__, (__Source_Span__){0});
    }

    if (__Lvalue__->__Kind__ == __Ast_Lvalue_Base__)
    {
        /* References the local. */
        __Semantic_Local__ *__Local__ = __Body_Find_Local__(__Context__, __Lvalue__);

        if (__Local__ == NULL)
        {
            return __Body_Fail__(
                __Context__, __E0300_Unknown_Name__, __Lvalue__->__Header__.__Span__);
        }

        if (__Local__->__Type__ == NULL)
        {
            return __Body_Fail__(
                __Context__, __E0409_Unresolved_Type__, __Lvalue__->__Header__.__Span__);
        }

        if (__Require_Read__ &&
            !__Body_Check_Local_Read__(__Context__, __Local__, __Lvalue__->__Header__.__Span__))
        {
            return 0;
        }

        *__Out_Type__ = __Local__->__Type__;
        if (__Out_Local__ != NULL)
        {
            *__Out_Local__ = __Local__;
        }
        return 1;
    }

    if (__Lvalue__->__Kind__ == __Ast_Lvalue_Field__)
    {
        /* References the parent type. */
        __Ast_Type__ *__Parent_Type__ = NULL;
        /* Stores the resolved. */
        __Resolved_Type__ __Resolved__;
        /* Stores the offset. */
        size_t __Offset__ = 0U;
        /* References the field type. */
        __Ast_Type__ *__Field_Type__ = NULL;

        if (!__Body_Infer_Lvalue_Access__(
                __Context__, __Lvalue__->__As__.__Field__.__Parent__, &__Parent_Type__, NULL, 1) ||
            !__Type_Resolve__(__Context__->__Semantic__, __Parent_Type__, &__Resolved__))
        {
            return 0;
        }

        if (__Resolved__.__Kind__ != __Resolved_Type_Struct__ || __Resolved__.__Named__ == NULL ||
            !__Layout_Struct_Field__(__Context__->__Semantic__,
                                     __Resolved__.__Named__,
                                     __Lvalue__->__As__.__Field__.__Field__,
                                     &__Offset__,
                                     &__Field_Type__))
        {
            return __Body_Fail__(
                __Context__, __E0502_Unknown_Record_Member__, __Lvalue__->__Header__.__Span__);
        }

        (void)__Offset__;
        *__Out_Type__ = __Field_Type__;
        return 1;
    }

    if (__Lvalue__->__Kind__ == __Ast_Lvalue_Index__)
    {
        /* References the parent type. */
        __Ast_Type__ *__Parent_Type__ = NULL;
        /* References the index type. */
        __Ast_Type__ *__Index_Type__ = NULL;
        /* Stores the resolved. */
        __Resolved_Type__ __Resolved__;

        if (!__Body_Infer_Lvalue_Access__(
                __Context__, __Lvalue__->__As__.__Index__.__Parent__, &__Parent_Type__, NULL, 1) ||
            !__Body_Infer_Atom__(
                __Context__, &__Lvalue__->__As__.__Index__.__Index__, &__Index_Type__))
        {
            return 0;
        }

        if (!__Body_Is_Integer_Like__(__Context__, __Index_Type__, NULL))
        {
            return __Body_Fail__(
                __Context__, __E0400_Mismatched_Types__, __Lvalue__->__Header__.__Span__);
        }

        if (!__Type_Resolve__(__Context__->__Semantic__, __Parent_Type__, &__Resolved__))
        {
            return 0;
        }

        if (__Resolved__.__Kind__ == __Resolved_Type_Vector__ ||
            __Resolved__.__Kind__ == __Resolved_Type_String__)
        {
            /* Tracks the known index. */
            int64_t __Known_Index__ = 0;
            if (__Semantic_Integer_Literal_Atom_Value__(&__Lvalue__->__As__.__Index__.__Index__,
                                                        &__Known_Index__))
            {
                /* References the sequence local. */
                __Semantic_Local__ *__Sequence_Local__ =
                    __Lvalue__->__As__.__Index__.__Parent__->__Kind__ == __Ast_Lvalue_Base__
                        ? __Body_Find_Local__(__Context__, __Lvalue__->__As__.__Index__.__Parent__)
                        : NULL;
                if (__Known_Index__ < 0 ||
                    (__Sequence_Local__ != NULL && __Sequence_Local__->__Has_Known_Length__ &&
                     (uint64_t)__Known_Index__ >= (uint64_t)__Sequence_Local__->__Known_Length__))
                {
                    /* References the diagnostic state. */
                    __Diagnostic__ *__Diagnostic_State__ = __Body_Begin_Diagnostic__(
                        __Context__, __E1305_Bounds_Violation__, __Lvalue__->__Header__.__Span__);
                    if (__Diagnostic_State__ != NULL)
                    {
                        __Diagnostic_Set_Primary_Role__(__Diagnostic_State__,
                                                        __Diagnostic_Span_Role_Index_Value__);
                        __Diagnostic_Set_Argument_U64__(
                            __Diagnostic_State__,
                            __Diagnostic_Argument_Index__,
                            __Known_Index__ < 0 ? 0U : (uint64_t)__Known_Index__);
                        if (__Sequence_Local__ != NULL && __Sequence_Local__->__Has_Known_Length__)
                        {
                            /* Stores the range. */
                            char __Range__[64];
                            __Diagnostic_Set_Message_Key__(__Diagnostic_State__,
                                                           __Diag_Word_Bounds_Known__);
                            __Diagnostic_Set_Argument_U64__(
                                __Diagnostic_State__,
                                __Diagnostic_Argument_Length__,
                                (uint64_t)__Sequence_Local__->__Known_Length__);
                            if (__Sequence_Local__->__Known_Length__ == 0U)
                            {
                                (void)snprintf(__Range__, sizeof(__Range__), "empty");
                            }
                            else
                            {
                                (void)snprintf(__Range__,
                                               sizeof(__Range__),
                                               "0..%zu",
                                               __Sequence_Local__->__Known_Length__ - 1U);
                            }
                            __Diagnostic_Set_Argument_Cstr__(
                                __Diagnostic_State__, __Diagnostic_Argument_Range__, __Range__);
                            __Diagnostic_Add_Note__(__Diagnostic_State__,
                                                    __Diag_Word_Note_Valid_Range__);
                        }
                    }
                    return 0;
                }
            }
        }

        if (__Resolved__.__Kind__ != __Resolved_Type_Vector__ &&
            __Resolved__.__Kind__ != __Resolved_Type_String__ &&
            __Resolved__.__Kind__ != __Resolved_Type_Box__)
        {
            return __Body_Fail__(
                __Context__, __E0400_Mismatched_Types__, __Lvalue__->__Header__.__Span__);
        }

        if (__Resolved__.__Kind__ == __Resolved_Type_String__)
        {
            if (!__Require_Read__)
            {
                return __Body_Fail__(
                    __Context__, __E0703_Write_To_Immutable__, __Lvalue__->__Header__.__Span__);
            }
            *__Out_Type__ = &__Body_Builtin_U8_Type__;
            return 1;
        }

        *__Out_Type__ = __Resolved__.__Inner__;
        return 1;
    }

    if (__Lvalue__->__Kind__ == __Ast_Lvalue_Dereference__)
    {
        /* References the parent type. */
        __Ast_Type__ *__Parent_Type__ = NULL;
        /* Stores the resolved. */
        __Resolved_Type__ __Resolved__;

        if (!__Body_Infer_Lvalue_Access__(__Context__,
                                          __Lvalue__->__As__.__Dereference_Parent__,
                                          &__Parent_Type__,
                                          NULL,
                                          1) ||
            !__Type_Resolve__(__Context__->__Semantic__, __Parent_Type__, &__Resolved__))
        {
            return 0;
        }

        if (__Resolved__.__Kind__ != __Resolved_Type_Reference__ &&
            __Resolved__.__Kind__ != __Resolved_Type_Box__)
        {
            return __Body_Fail__(
                __Context__, __E0612_Dereference_Non_Pointer__, __Lvalue__->__Header__.__Span__);
        }

        if (!__Require_Read__ && __Resolved__.__Kind__ == __Resolved_Type_Reference__ &&
            !__Resolved__.__Reference_Mutable__)
        {
            return __Body_Fail__(
                __Context__, __E0703_Write_To_Immutable__, __Lvalue__->__Header__.__Span__);
        }

        *__Out_Type__ = __Resolved__.__Inner__;
        return 1;
    }

    return __Body_Fail__(__Context__, __E0300_Unknown_Name__, __Lvalue__->__Header__.__Span__);
}

/* Infers the body lvalue. */
int __Body_Infer_Lvalue__(__Semantic_Body_Context__ *__Context__,
                          __Ast_Lvalue__ *__Lvalue__,
                          __Ast_Type__ **__Out_Type__,
                          __Semantic_Local__ **__Out_Local__)
{
    return __Body_Infer_Lvalue_Access__(__Context__, __Lvalue__, __Out_Type__, __Out_Local__, 1);
}

/* Infers the body lvalue for write. */
int __Body_Infer_Lvalue_For_Write__(__Semantic_Body_Context__ *__Context__,
                                    __Ast_Lvalue__ *__Lvalue__,
                                    __Ast_Type__ **__Out_Type__,
                                    __Semantic_Local__ **__Out_Local__)
{
    return __Body_Infer_Lvalue_Access__(__Context__, __Lvalue__, __Out_Type__, __Out_Local__, 0);
}
