#include "semantic/body_internal.h"
#include "kernel/memory/memory.h"
#include "kernel/name/name.h"
#include "frontend/identifier_identity.h"

static __Name_Builtin_Function__ __Body_Builtin_Identity__(__Semantic_Body_Context__ *__Context__,
                                                           __Ast_Expression__ *__Expression__)
{
    __Ast_Lvalue__ *__Function__;

    if (__Expression__ == NULL || __Expression__->__Kind__ != __Ast_Expression_Call__)
    {
        return __Name_Builtin_None__;
    }
    __Function__ = __Expression__->__As__.__Call__.__Function__;
    if (__Function__ == NULL || __Function__->__Kind__ != __Ast_Lvalue_Base__ ||
        __Function__->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__ ||
        __Body_Find_Local__(__Context__, __Function__) != NULL)
    {
        return __Name_Builtin_None__;
    }
    {
        __Text_Slice__ __Source_Name__ = __Function__->__As__.__Base__.__As__.__Identifier__;
        __Text_Slice__ __Resolved_Name__ = __Source_Name__;
        __Name_Builtin_Function__ __Direct__ = __Name_Find_Builtin_Function__(__Source_Name__);
        const __Program_Unit__ *__Unit__ =
            (__Context__->__Function__ == NULL) ? NULL : __Context__->__Function__->__Unit__;

        if (__Direct__ != __Name_Builtin_None__ && __Name_Builtin_Is_Direct_Source__(__Direct__))
        {
            return __Direct__;
        }
        if (__Unit__ == NULL ||
            !__Name_Resolve_Module_Alias_Name__(__Unit__, __Source_Name__, &__Resolved_Name__) ||
            __Identifier_Identity_Equals__(__Resolved_Name__, __Source_Name__))
        {
            return __Name_Builtin_None__;
        }
        return __Name_Find_Builtin_Function__(__Resolved_Name__);
    }
}

static int __Body_Builtin_Length__(__Semantic_Body_Context__ *__Context__,
                                   __Ast_Expression__ *__Expression__,
                                   __Ast_Type__ **__Out_Type__)
{
    __Ast_Type__ *__Argument_Type__ = NULL;
    __Resolved_Type__ __Resolved__;

    if (__Expression__->__As__.__Call__.__Argument_Count__ != 1U)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    if (!__Body_Infer_Expression__(
            __Context__, __Expression__->__As__.__Call__.__Arguments__[0], &__Argument_Type__) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Argument_Type__, &__Resolved__))
    {
        return 0;
    }
    if (__Resolved__.__Kind__ != __Resolved_Type_String__ &&
        __Resolved__.__Kind__ != __Resolved_Type_Vector__)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    *__Out_Type__ = &__Body_Builtin_Int_Type__;
    return 1;
}

static int __Body_Builtin_Append__(__Semantic_Body_Context__ *__Context__,
                                   __Ast_Expression__ *__Expression__,
                                   __Ast_Type__ **__Out_Type__)
{
    __Ast_Expression__ *__Vector_Expression__;
    __Ast_Lvalue__ *__Vector_Lvalue__;
    __Ast_Type__ *__Vector_Type__ = NULL;
    __Semantic_Local__ *__Vector_Local__ = NULL;
    __Resolved_Type__ __Resolved__;
    __Ast_Expression__ *__Value_Expression__;

    if (__Expression__->__As__.__Call__.__Argument_Count__ != 2U)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }

    /* The same call node may be inferred again while checking its enclosing
     * statement. Ownership transfer was already proven/applied on the first
     * inference, so do not re-read a value that is now correctly moved. */
    if (__Expression__->__Semantic_Effects_Applied__)
    {
        *__Out_Type__ = &__Body_Builtin_Void_Type__;
        return 1;
    }

    __Vector_Expression__ = __Expression__->__As__.__Call__.__Arguments__[0];
    if (__Vector_Expression__ == NULL ||
        __Vector_Expression__->__Kind__ != __Ast_Expression_Atom__ ||
        __Vector_Expression__->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    __Vector_Lvalue__ = __Vector_Expression__->__As__.__Atom__.__As__.__Lvalue__;
    if (!__Body_Check_Assignable__(
            __Context__, __Vector_Lvalue__, &__Vector_Type__, &__Vector_Local__) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Vector_Type__, &__Resolved__))
    {
        return 0;
    }
    if (__Resolved__.__Kind__ != __Resolved_Type_Vector__)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }

    __Value_Expression__ = __Expression__->__As__.__Call__.__Arguments__[1];
    if (!__Body_Check_Expression_Compatible__(__Context__,
                                              __Resolved__.__Inner__,
                                              __Value_Expression__,
                                              __Value_Expression__->__Header__.__Span__))
    {
        return 0;
    }
    if (!__Expression__->__Semantic_Effects_Applied__)
    {
        if (__Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Resolved__.__Inner__) &&
            !__Safety_Type_Is_Reference__(__Resolved__.__Inner__, NULL) &&
            !__Body_Safety_Move_Expression__(__Context__, __Value_Expression__, NULL))
        {
            return 0;
        }

        if (__Vector_Local__ != NULL && __Vector_Local__->__Has_Known_Length__)
        {
            if (__Vector_Local__->__Known_Length__ == SIZE_MAX)
            {
                __Vector_Local__->__Has_Known_Length__ = 0;
            }
            else
            {
                ++__Vector_Local__->__Known_Length__;
            }
        }
    }
    *__Out_Type__ = &__Body_Builtin_Void_Type__;
    return 1;
}

static int __Body_Builtin_Require_Argument_Count__(__Semantic_Body_Context__ *__Context__,
                                                   __Ast_Expression__ *__Expression__,
                                                   size_t __Expected__)
{
    if (__Expression__->__As__.__Call__.__Argument_Count__ == __Expected__)
    {
        return 1;
    }
    return __Body_Fail__(
        __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
}

static int __Body_Builtin_Check_Runtime_Argument__(__Semantic_Body_Context__ *__Context__,
                                                   __Ast_Expression__ *__Expression__,
                                                   size_t __Index__,
                                                   __Ast_Type__ *__Expected__)
{
    __Ast_Expression__ *__Argument__;

    if (__Index__ >= __Expression__->__As__.__Call__.__Argument_Count__)
    {
        return 0;
    }
    __Argument__ = __Expression__->__As__.__Call__.__Arguments__[__Index__];
    return __Body_Check_Expression_Compatible__(
        __Context__, __Expected__, __Argument__, __Argument__->__Header__.__Span__);
}

static int __Body_Builtin_Runtime__(__Semantic_Body_Context__ *__Context__,
                                    __Ast_Expression__ *__Expression__,
                                    __Name_Builtin_Function__ __Builtin__,
                                    __Ast_Type__ **__Out_Type__)
{
    __Ast_Type__ *__Result_Type__ = NULL;

    switch (__Builtin__)
    {
        case __Name_Builtin_Open_File_Read__:
        case __Name_Builtin_Create_File_Write__:
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 1U) ||
                !__Body_Builtin_Check_Runtime_Argument__(
                    __Context__, __Expression__, 0U, &__Body_Builtin_String_Type__))
            {
                return 0;
            }
            __Result_Type__ = &__Body_Builtin_Int_Type__;
            break;
        case __Name_Builtin_Read_File_Byte__:
        case __Name_Builtin_Close_File__:
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 1U) ||
                !__Body_Builtin_Check_Runtime_Argument__(
                    __Context__, __Expression__, 0U, &__Body_Builtin_Int_Type__))
            {
                return 0;
            }
            __Result_Type__ = &__Body_Builtin_Int_Type__;
            break;
        case __Name_Builtin_Read_Stdin_Byte__:
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 0U))
                return 0;
            __Result_Type__ = &__Body_Builtin_Int_Type__;
            break;
        case __Name_Builtin_Read_Stdin_Segment__:
        {
            __Ast_Type__ *__Bytes_Type__ = NULL;
            __Resolved_Type__ __Bytes_Resolved__;
            __Resolved_Type__ __Element_Resolved__;
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 3U) ||
                !__Body_Infer_Expression__(__Context__,
                                           __Expression__->__As__.__Call__.__Arguments__[0],
                                           &__Bytes_Type__) ||
                !__Type_Resolve__(__Context__->__Semantic__, __Bytes_Type__, &__Bytes_Resolved__) ||
                __Bytes_Resolved__.__Kind__ != __Resolved_Type_Vector__ ||
                !__Type_Resolve__(__Context__->__Semantic__,
                                  __Bytes_Resolved__.__Inner__,
                                  &__Element_Resolved__) ||
                __Element_Resolved__.__Kind__ != __Resolved_Type_Unsigned_Integer__ ||
                __Element_Resolved__.__Bits__ != 8U ||
                !__Body_Builtin_Check_Runtime_Argument__(
                    __Context__, __Expression__, 1U, &__Body_Builtin_Int_Type__) ||
                !__Body_Builtin_Check_Runtime_Argument__(
                    __Context__, __Expression__, 2U, &__Body_Builtin_Int_Type__))
            {
                return __Body_Fail__(
                    __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
            }
            __Result_Type__ = &__Body_Builtin_Int_Type__;
            break;
        }
        case __Name_Builtin_Read_File_Segment__:
        case __Name_Builtin_Write_File_Segment__:
        {
            __Ast_Type__ *__Bytes_Type__ = NULL;
            __Resolved_Type__ __Bytes_Resolved__;
            __Resolved_Type__ __Element_Resolved__;
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 4U) ||
                !__Body_Builtin_Check_Runtime_Argument__(
                    __Context__, __Expression__, 0U, &__Body_Builtin_Int_Type__) ||
                !__Body_Infer_Expression__(__Context__,
                                           __Expression__->__As__.__Call__.__Arguments__[1],
                                           &__Bytes_Type__) ||
                !__Type_Resolve__(__Context__->__Semantic__, __Bytes_Type__, &__Bytes_Resolved__) ||
                __Bytes_Resolved__.__Kind__ != __Resolved_Type_Vector__ ||
                !__Type_Resolve__(__Context__->__Semantic__,
                                  __Bytes_Resolved__.__Inner__,
                                  &__Element_Resolved__) ||
                __Element_Resolved__.__Kind__ != __Resolved_Type_Unsigned_Integer__ ||
                __Element_Resolved__.__Bits__ != 8U ||
                !__Body_Builtin_Check_Runtime_Argument__(
                    __Context__, __Expression__, 2U, &__Body_Builtin_Int_Type__) ||
                !__Body_Builtin_Check_Runtime_Argument__(
                    __Context__, __Expression__, 3U, &__Body_Builtin_Int_Type__))
            {
                return __Body_Fail__(
                    __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
            }
            __Result_Type__ = &__Body_Builtin_Int_Type__;
            break;
        }
        case __Name_Builtin_Write_Executable_Bytes__:
        {
            __Ast_Type__ *__Bytes_Type__ = NULL;
            __Resolved_Type__ __Bytes_Resolved__;
            __Resolved_Type__ __Element_Resolved__;
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 2U) ||
                !__Body_Builtin_Check_Runtime_Argument__(
                    __Context__, __Expression__, 0U, &__Body_Builtin_String_Type__) ||
                !__Body_Infer_Expression__(__Context__,
                                           __Expression__->__As__.__Call__.__Arguments__[1],
                                           &__Bytes_Type__) ||
                !__Type_Resolve__(__Context__->__Semantic__, __Bytes_Type__, &__Bytes_Resolved__) ||
                __Bytes_Resolved__.__Kind__ != __Resolved_Type_Vector__ ||
                !__Type_Resolve__(__Context__->__Semantic__,
                                  __Bytes_Resolved__.__Inner__,
                                  &__Element_Resolved__) ||
                __Element_Resolved__.__Kind__ != __Resolved_Type_Unsigned_Integer__ ||
                __Element_Resolved__.__Bits__ != 8U)
            {
                return __Body_Fail__(
                    __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
            }
            __Result_Type__ = __Body_Synthetic_Result_Type__(
                __Context__, &__Body_Builtin_Int_Type__, &__Body_Builtin_Int_Type__);
            break;
        }
        case __Name_Builtin_Stdout_Write__:
        case __Name_Builtin_Stderr_Write__:
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 1U) ||
                !__Body_Builtin_Check_Runtime_Argument__(
                    __Context__, __Expression__, 0U, &__Body_Builtin_String_Type__))
            {
                return 0;
            }
            __Result_Type__ = &__Body_Builtin_Int_Type__;
            break;
        case __Name_Builtin_Host_Architecture__:
        case __Name_Builtin_Host_Platform__:
        case __Name_Builtin_Host_Environment__:
        case __Name_Builtin_Argument_Count__:
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 0U))
            {
                return 0;
            }
            __Result_Type__ = &__Body_Builtin_Int_Type__;
            break;
        case __Name_Builtin_Argument__:
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 1U) ||
                !__Body_Builtin_Check_Runtime_Argument__(
                    __Context__, __Expression__, 0U, &__Body_Builtin_Int_Type__))
            {
                return 0;
            }
            __Result_Type__ = &__Body_Builtin_String_Type__;
            break;
        case __Name_Builtin_Process_Exit__:
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 1U) ||
                !__Body_Builtin_Check_Runtime_Argument__(
                    __Context__, __Expression__, 0U, &__Body_Builtin_Int_Type__))
            {
                return 0;
            }
            __Result_Type__ = &__Body_Builtin_Void_Type__;
            break;
        case __Name_Builtin_Text_From_Bytes__:
        {
            __Ast_Type__ *__Bytes_Type__ = NULL;
            __Resolved_Type__ __Bytes_Resolved__;
            __Resolved_Type__ __Element_Resolved__;

            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 1U))
            {
                return 0;
            }
            if (!__Body_Infer_Expression__(__Context__,
                                           __Expression__->__As__.__Call__.__Arguments__[0],
                                           &__Bytes_Type__) ||
                !__Type_Resolve__(__Context__->__Semantic__, __Bytes_Type__, &__Bytes_Resolved__))
            {
                return 0;
            }
            if (__Bytes_Resolved__.__Kind__ != __Resolved_Type_Vector__ ||
                !__Type_Resolve__(__Context__->__Semantic__,
                                  __Bytes_Resolved__.__Inner__,
                                  &__Element_Resolved__) ||
                __Element_Resolved__.__Kind__ != __Resolved_Type_Unsigned_Integer__ ||
                __Element_Resolved__.__Bits__ != 8U)
            {
                return __Body_Fail__(
                    __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
            }
            __Result_Type__ = &__Body_Builtin_String_Type__;
            break;
        }
        case __Name_Builtin_None__:
        case __Name_Builtin_Length__:
        case __Name_Builtin_Append__:
            return 0;
    }

    if (__Result_Type__ == NULL)
    {
        return __Body_Fail__(
            __Context__, __E1100_Internal_Context_Error__, __Expression__->__Header__.__Span__);
    }
    *__Out_Type__ = __Result_Type__;
    __Expression__->__Semantic_Effects_Applied__ = 1;
    return 1;
}

int __Body_Try_Infer_Builtin_Call__(__Semantic_Body_Context__ *__Context__,
                                    __Ast_Expression__ *__Expression__,
                                    __Ast_Type__ **__Out_Type__,
                                    int *__Matched__)
{
    __Name_Builtin_Function__ __Builtin__ = __Body_Builtin_Identity__(__Context__, __Expression__);

    *__Matched__ = __Builtin__ != __Name_Builtin_None__;
    switch (__Builtin__)
    {
        case __Name_Builtin_None__:
            return 1;
        case __Name_Builtin_Length__:
            if (!__Body_Builtin_Length__(__Context__, __Expression__, __Out_Type__))
            {
                return 0;
            }
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
        case __Name_Builtin_Append__:
            if (!__Body_Builtin_Append__(__Context__, __Expression__, __Out_Type__))
            {
                return 0;
            }
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
        case __Name_Builtin_Open_File_Read__:
        case __Name_Builtin_Read_File_Byte__:
        case __Name_Builtin_Read_File_Segment__:
        case __Name_Builtin_Create_File_Write__:
        case __Name_Builtin_Write_File_Segment__:
        case __Name_Builtin_Close_File__:
        case __Name_Builtin_Read_Stdin_Byte__:
        case __Name_Builtin_Read_Stdin_Segment__:
        case __Name_Builtin_Write_Executable_Bytes__:
        case __Name_Builtin_Stdout_Write__:
        case __Name_Builtin_Stderr_Write__:
        case __Name_Builtin_Host_Architecture__:
        case __Name_Builtin_Host_Platform__:
        case __Name_Builtin_Host_Environment__:
        case __Name_Builtin_Argument_Count__:
        case __Name_Builtin_Argument__:
        case __Name_Builtin_Process_Exit__:
        case __Name_Builtin_Text_From_Bytes__:
            return __Body_Builtin_Runtime__(__Context__, __Expression__, __Builtin__, __Out_Type__);
    }
    return 0;
}
