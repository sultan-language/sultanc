#ifndef SULTANC__KERNEL_NAME_NAME_H__
#define SULTANC__KERNEL_NAME_NAME_H__

#include "semantic/context.h"

typedef enum
{
    __Name_Lookup_Missing__,
    __Name_Lookup_Found__,
    __Name_Lookup_Private__,
    __Name_Lookup_Ambiguous__
} __Name_Lookup_Status__;

/* Canonical intrinsic function identities. */
typedef enum
{
    __Name_Builtin_None__,
    __Name_Builtin_Length__,
    __Name_Builtin_Append__,
    __Name_Builtin_Open_File_Read__,
    __Name_Builtin_Read_File_Byte__,
    __Name_Builtin_Read_File_Segment__,
    __Name_Builtin_Create_File_Write__,
    __Name_Builtin_Write_File_Segment__,
    __Name_Builtin_Close_File__,
    __Name_Builtin_Read_Stdin_Byte__,
    __Name_Builtin_Read_Stdin_Segment__,
    __Name_Builtin_Write_Executable_Bytes__,
    __Name_Builtin_Stdout_Write__,
    __Name_Builtin_Stderr_Write__,
    __Name_Builtin_Host_Architecture__,
    __Name_Builtin_Host_Platform__,
    __Name_Builtin_Host_Environment__,
    __Name_Builtin_Argument_Count__,
    __Name_Builtin_Argument__,
    __Name_Builtin_Process_Exit__,
    __Name_Builtin_Text_From_Bytes__
} __Name_Builtin_Function__;

__Name_Builtin_Function__ __Name_Find_Builtin_Function__(__Text_Slice__ __Name__);

int __Name_Builtin_Is_Direct_Source__(__Name_Builtin_Function__ __Builtin__);

int __Name_Resolve_Module_Alias_Name__(const __Program_Unit__ *__Unit__,
                                       __Text_Slice__ __Name__,
                                       __Text_Slice__ *__Out_Name__);

__Name_Lookup_Status__ __Name_Resolve_Type__(__Semantic_Context__ *__Context__,
                                             const __Program_Unit__ *__From_Unit__,
                                             __Text_Slice__ __Name__,
                                             __Semantic_Type_Entry__ **__Out_Entry__);

__Name_Lookup_Status__ __Name_Resolve_Function__(__Semantic_Context__ *__Context__,
                                                 const __Program_Unit__ *__From_Unit__,
                                                 __Text_Slice__ __Name__,
                                                 __Semantic_Function_Entry__ **__Out_Entry__);

__Error_Id__ __Name_Lookup_Error_Id__(__Name_Lookup_Status__ __Status__);

/*
 * Context-scoped convenience accessors. They delegate to the canonical
 * resolver using Context.__Active_Unit__ and therefore do not own lookup rules.
 */
__Semantic_Type_Entry__ *__Name_Find_Type__(__Semantic_Context__ *__Context__,
                                            __Text_Slice__ __Name__);

__Semantic_Function_Entry__ *__Name_Find_Function__(__Semantic_Context__ *__Context__,
                                                    __Text_Slice__ __Name__);

int __Name_Find_Enum_Constructor__(__Semantic_Type_Entry__ *__Type__,
                                   __Text_Slice__ __Constructor_Name__,
                                   size_t *__Out_Index__,
                                   __Ast_Enum_Constructor__ **__Out_Constructor__);

int __Name_Resolve_Enum_Constructor_Lvalue__(__Semantic_Context__ *__Context__,
                                             const __Ast_Lvalue__ *__Lvalue__,
                                             __Semantic_Type_Entry__ **__Out_Type__,
                                             size_t *__Out_Index__,
                                             __Ast_Enum_Constructor__ **__Out_Constructor__);

#endif
