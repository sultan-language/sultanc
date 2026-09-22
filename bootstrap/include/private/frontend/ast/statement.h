#ifndef SULTANC__FRONTEND_AST_STATEMENT_H__
#define SULTANC__FRONTEND_AST_STATEMENT_H__

#include "expression.h"

typedef struct
{
    __Text_Slice__ __Name__;
    __Ast_Atom__ __Value__;
} __Ast_Record_Input__;

typedef enum
{
    __Ast_Pattern_Wildcard__,
    __Ast_Pattern_Binding__,
    __Ast_Pattern_Enum__,
    __Ast_Pattern_Struct__
} __Ast_Pattern_Kind__;

typedef struct
{
    __Text_Slice__ __Name__;
    __Ast_Pattern__ *__Pattern__;
} __Ast_Struct_Pattern_Field__;

struct __Ast_Pattern__
{
    __Ast_Header__ __Header__;
    __Ast_Pattern_Kind__ __Kind__;
    union
    {
        __Text_Slice__ __Binding__;
        struct
        {
            __Text_Slice__ __Type_Name__;
            __Text_Slice__ __Constructor_Name__;
            __Ast_Pattern__ **__Payloads__;
            size_t __Payload_Count__;
        } __Enum__;
        struct
        {
            __Ast_Struct_Pattern_Field__ *__Fields__;
            size_t __Field_Count__;
        } __Struct__;
    } __As__;
};

typedef struct
{
    __Ast_Pattern__ *__Pattern__;
    __Ast_Block__ *__Body__;
} __Ast_Match_Case__;

typedef enum
{
    __Ast_Statement_Variable_Declaration__,
    __Ast_Statement_Initialize_Record__,
    __Ast_Statement_Initialize_Vector__,
    __Ast_Statement_Initialize_Box__,
    __Ast_Statement_Copy__,
    __Ast_Statement_While__,
    __Ast_Statement_If__,
    __Ast_Statement_Match__,
    __Ast_Statement_Return__
} __Ast_Statement_Kind__;

typedef struct
{
    __Ast_Lvalue_Base_Kind__ __Name_Kind__;
    union
    {
        __Text_Slice__ __Identifier__;
        __Temporary_Id__ __Temporary__;
    } __Name__;
    __Ast_Slot__ __Slot__;
} __Ast_Variable_Declaration__;

struct __Ast_Block__
{
    __Ast_Header__ __Header__;
    __Ast_Statement__ **__Statements__;
    size_t __Statement_Count__;
};

struct __Ast_Statement__
{
    __Ast_Header__ __Header__;
    __Ast_Statement_Kind__ __Kind__;
    union
    {
        __Ast_Variable_Declaration__ __Variable__;
        struct
        {
            __Ast_Lvalue__ *__Destination__;
            __Ast_Record_Input__ *__Fields__;
            size_t __Field_Count__;
        } __Record__;
        struct
        {
            __Ast_Lvalue__ *__Destination__;
            __Ast_Atom__ *__Values__;
            size_t __Value_Count__;
        } __Aggregate__;
        struct
        {
            __Ast_Lvalue__ *__Destination__;
            __Ast_Atom__ __Value__;
        } __Box__;
        struct
        {
            __Ast_Lvalue__ *__Destination__;
            __Ast_Expression__ *__Expression__;
        } __Copy__;
        struct
        {
            __Ast_Expression__ *__Condition__;
            __Ast_Block__ *__Body__;
        } __While__;
        struct
        {
            __Ast_Expression__ *__Condition__;
            __Ast_Block__ *__Then__;
            __Ast_Block__ *__Else__;
        } __If__;
        struct
        {
            __Ast_Expression__ *__Value__;
            __Ast_Match_Case__ *__Cases__;
            size_t __Case_Count__;
        } __Match__;
        __Ast_Expression__ *__Return__;
    } __As__;
};

typedef struct
{
    __Text_Slice__ __Name__;
    __Ast_Slot__ __Slot__;
} __Ast_Function_Parameter__;

struct __Ast_Function__
{
    __Ast_Header__ __Header__;
    __Text_Slice__ __Name__;
    int __Public__;
    __Ast_Function_Parameter__ *__Parameters__;
    size_t __Parameter_Count__;
    __Ast_Slot__ __Output__;
    __Ast_Block__ *__Body__;
};

#endif
