/* Declares canonical name resolution. */

#ifndef SULTANC__KERNEL_NAME_NAME_H__
#define SULTANC__KERNEL_NAME_NAME_H__

#include "semantic/context.h"

/* Defines the name lookup status values. */
typedef enum
{
    /* Represents the name lookup missing value. */
    __Name_Lookup_Missing__,
    /* Represents the name lookup found value. */
    __Name_Lookup_Found__,
    /* Represents the name lookup private value. */
    __Name_Lookup_Private__,
    /* Represents the name lookup ambiguous value. */
    __Name_Lookup_Ambiguous__
} __Name_Lookup_Status__;

/* Canonical intrinsic function identities. */
typedef enum
{
    /* Represents the name builtin none value. */
    __Name_Builtin_None__,
    /* Represents the name builtin length value. */
    __Name_Builtin_Length__,
    /* Represents the name builtin append value. */
    __Name_Builtin_Append__,
    /* Represents the name builtin open file read value. */
    __Name_Builtin_Open_File_Read__,
    /* Represents the name builtin read file byte value. */
    __Name_Builtin_Read_File_Byte__,
    /* Represents the name builtin read file segment value. */
    __Name_Builtin_Read_File_Segment__,
    /* Represents the name builtin create file write value. */
    __Name_Builtin_Create_File_Write__,
    /* Represents the name builtin write file segment value. */
    __Name_Builtin_Write_File_Segment__,
    /* Represents the name builtin close file value. */
    __Name_Builtin_Close_File__,
    /* Represents the name builtin open directory value. */
    __Name_Builtin_Open_Directory__,
    /* Represents the name builtin read directory entry value. */
    __Name_Builtin_Read_Directory_Entry__,
    /* Represents the name builtin close directory value. */
    __Name_Builtin_Close_Directory__,
    /* Represents the name builtin read stdin byte value. */
    __Name_Builtin_Read_Stdin_Byte__,
    /* Represents the name builtin read stdin segment value. */
    __Name_Builtin_Read_Stdin_Segment__,
    /* Represents the name builtin write executable bytes value. */
    __Name_Builtin_Write_Executable_Bytes__,
    /* Represents the name builtin stdout write value. */
    __Name_Builtin_Stdout_Write__,
    /* Represents the name builtin stderr write value. */
    __Name_Builtin_Stderr_Write__,
    /* Represents the name builtin host architecture value. */
    __Name_Builtin_Host_Architecture__,
    /* Represents the name builtin host platform value. */
    __Name_Builtin_Host_Platform__,
    /* Represents the name builtin host environment value. */
    __Name_Builtin_Host_Environment__,
    /* Represents the name builtin argument count value. */
    __Name_Builtin_Argument_Count__,
    /* Represents the name builtin argument value. */
    __Name_Builtin_Argument__,
    /* Represents the name builtin process exit value. */
    __Name_Builtin_Process_Exit__,
    /* Represents the name builtin text from bytes value. */
    __Name_Builtin_Text_From_Bytes__,
    /* Represents the name builtin path type value. */
    __Name_Builtin_Path_Type__,
    /* Represents the name builtin path size value. */
    __Name_Builtin_Path_Size__,
    /* Represents the name builtin path modified time value. */
    __Name_Builtin_Path_Modified_Time__
} __Name_Builtin_Function__;

/* Finds the name builtin function. */
__Name_Builtin_Function__ __Name_Find_Builtin_Function__(__Text_Slice__ __Name__);

/* Checks whether the name builtin is direct source. */
int __Name_Builtin_Is_Direct_Source__(__Name_Builtin_Function__ __Builtin__);

/* Resolves the name module alias name. */
int __Name_Resolve_Module_Alias_Name__(const __Program_Unit__ *__Unit__,
                                       __Text_Slice__ __Name__,
                                       __Text_Slice__ *__Out_Name__);

/* Resolves the name type. */
__Name_Lookup_Status__ __Name_Resolve_Type__(__Semantic_Context__ *__Context__,
                                             const __Program_Unit__ *__From_Unit__,
                                             __Text_Slice__ __Name__,
                                             __Semantic_Type_Entry__ **__Out_Entry__);

/* Resolves the name function. */
__Name_Lookup_Status__ __Name_Resolve_Function__(__Semantic_Context__ *__Context__,
                                                 const __Program_Unit__ *__From_Unit__,
                                                 __Text_Slice__ __Name__,
                                                 __Semantic_Function_Entry__ **__Out_Entry__);

/* Finds the name error ID. */
__Error_Id__ __Name_Lookup_Error_Id__(__Name_Lookup_Status__ __Status__);

/* Context-scoped accessors that delegate to canonical name resolution. */
__Semantic_Type_Entry__ *__Name_Find_Type__(__Semantic_Context__ *__Context__,
                                            __Text_Slice__ __Name__);

/* Finds the name function. */
__Semantic_Function_Entry__ *__Name_Find_Function__(__Semantic_Context__ *__Context__,
                                                    __Text_Slice__ __Name__);

/* Finds the name enum constructor. */
int __Name_Find_Enum_Constructor__(__Semantic_Type_Entry__ *__Type__,
                                   __Text_Slice__ __Constructor_Name__,
                                   size_t *__Out_Index__,
                                   __Ast_Enum_Constructor__ **__Out_Constructor__);

/* Resolves the name enum constructor lvalue. */
int __Name_Resolve_Enum_Constructor_Lvalue__(__Semantic_Context__ *__Context__,
                                             const __Ast_Lvalue__ *__Lvalue__,
                                             __Semantic_Type_Entry__ **__Out_Type__,
                                             size_t *__Out_Index__,
                                             __Ast_Enum_Constructor__ **__Out_Constructor__);

#endif
