/* Defines statement AST nodes. */

#ifndef SULTANC__FRONTEND_AST_STATEMENT_H__
#define SULTANC__FRONTEND_AST_STATEMENT_H__

#include "expression.h"

/* Defines the AST record input structure. */
typedef struct
{
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* Stores the value. */
    __Ast_Atom__ __Value__;
} __Ast_Record_Input__;

/* Defines the AST pattern kind values. */
typedef enum
{
    /* Represents the AST pattern wildcard value. */
    __Ast_Pattern_Wildcard__,
    /* Represents the AST pattern binding value. */
    __Ast_Pattern_Binding__,
    /* Represents the AST pattern enum value. */
    __Ast_Pattern_Enum__,
    /* Represents the AST pattern struct value. */
    __Ast_Pattern_Struct__
} __Ast_Pattern_Kind__;

/* Defines the AST struct pattern field structure. */
typedef struct
{
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* References the pattern. */
    __Ast_Pattern__ *__Pattern__;
} __Ast_Struct_Pattern_Field__;

/* Defines the AST pattern structure. */
struct __Ast_Pattern__
{
    /* Stores the header. */
    __Ast_Header__ __Header__;
    /* Stores the kind. */
    __Ast_Pattern_Kind__ __Kind__;
    /* Defines the struct structure. */
    union
    {
        /* Stores the binding. */
        __Text_Slice__ __Binding__;
        /* Defines the struct structure. */
        struct
        {
            /* Stores the type name. */
            __Text_Slice__ __Type_Name__;
            /* Stores the constructor name. */
            __Text_Slice__ __Constructor_Name__;
            /* References the payloads. */
            __Ast_Pattern__ **__Payloads__;
            /* Stores the payload count. */
            size_t __Payload_Count__;
        } __Enum__;
        /* Defines the struct structure. */
        struct
        {
            /* References the fields. */
            __Ast_Struct_Pattern_Field__ *__Fields__;
            /* Stores the field count. */
            size_t __Field_Count__;
        } __Struct__;
    } __As__;
};

/* Defines the AST match case structure. */
typedef struct
{
    /* References the pattern. */
    __Ast_Pattern__ *__Pattern__;
    /* References the body. */
    __Ast_Block__ *__Body__;
} __Ast_Match_Case__;

/* Defines the AST statement kind values. */
typedef enum
{
    /* Represents the AST statement variable declaration value. */
    __Ast_Statement_Variable_Declaration__,
    /* Represents the AST statement initialize record value. */
    __Ast_Statement_Initialize_Record__,
    /* Represents the AST statement initialize vector value. */
    __Ast_Statement_Initialize_Vector__,
    /* Represents the AST statement initialize box value. */
    __Ast_Statement_Initialize_Box__,
    /* Represents the AST statement copy value. */
    __Ast_Statement_Copy__,
    /* Represents the AST statement while value. */
    __Ast_Statement_While__,
    /* Represents the AST statement if value. */
    __Ast_Statement_If__,
    /* Represents the AST statement match value. */
    __Ast_Statement_Match__,
    /* Represents the AST statement return value. */
    __Ast_Statement_Return__
} __Ast_Statement_Kind__;

/* Defines the AST variable declaration structure. */
typedef struct
{
    /* Stores the name kind. */
    __Ast_Lvalue_Base_Kind__ __Name_Kind__;
    /* Stores the name. */
    union
    {
        /* Stores the identifier. */
        __Text_Slice__ __Identifier__;
        /* Stores the temporary. */
        __Temporary_Id__ __Temporary__;
    } __Name__;
    /* Stores the slot. */
    __Ast_Slot__ __Slot__;
} __Ast_Variable_Declaration__;

/* Defines the AST block structure. */
struct __Ast_Block__
{
    /* Stores the header. */
    __Ast_Header__ __Header__;
    /* References the statements. */
    __Ast_Statement__ **__Statements__;
    /* Stores the statement count. */
    size_t __Statement_Count__;
};

/* Defines the AST statement structure. */
struct __Ast_Statement__
{
    /* Stores the header. */
    __Ast_Header__ __Header__;
    /* Stores the kind. */
    __Ast_Statement_Kind__ __Kind__;
    /* Defines the struct structure. */
    union
    {
        /* Stores the variable. */
        __Ast_Variable_Declaration__ __Variable__;
        /* Defines the struct structure. */
        struct
        {
            /* References the destination. */
            __Ast_Lvalue__ *__Destination__;
            /* References the fields. */
            __Ast_Record_Input__ *__Fields__;
            /* Stores the field count. */
            size_t __Field_Count__;
        } __Record__;
        /* Defines the struct structure. */
        struct
        {
            /* References the destination. */
            __Ast_Lvalue__ *__Destination__;
            /* References the values. */
            __Ast_Atom__ *__Values__;
            /* Stores the value count. */
            size_t __Value_Count__;
        } __Aggregate__;
        /* Defines the struct structure. */
        struct
        {
            /* References the destination. */
            __Ast_Lvalue__ *__Destination__;
            /* Stores the value. */
            __Ast_Atom__ __Value__;
        } __Box__;
        /* Defines the struct structure. */
        struct
        {
            /* References the destination. */
            __Ast_Lvalue__ *__Destination__;
            /* References the expression. */
            __Ast_Expression__ *__Expression__;
        } __Copy__;
        /* Defines the struct structure. */
        struct
        {
            /* References the condition. */
            __Ast_Expression__ *__Condition__;
            /* References the body. */
            __Ast_Block__ *__Body__;
        } __While__;
        /* Defines the struct structure. */
        struct
        {
            /* References the condition. */
            __Ast_Expression__ *__Condition__;
            /* References the then. */
            __Ast_Block__ *__Then__;
            /* References the else. */
            __Ast_Block__ *__Else__;
        } __If__;
        /* Defines the struct structure. */
        struct
        {
            /* References the value. */
            __Ast_Expression__ *__Value__;
            /* References the cases. */
            __Ast_Match_Case__ *__Cases__;
            /* Stores the case count. */
            size_t __Case_Count__;
        } __Match__;
        /* References the return. */
        __Ast_Expression__ *__Return__;
    } __As__;
};

/* Defines the AST function parameter structure. */
typedef struct
{
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* Stores the slot. */
    __Ast_Slot__ __Slot__;
} __Ast_Function_Parameter__;

/* Defines the AST function structure. */
struct __Ast_Function__
{
    /* Stores the header. */
    __Ast_Header__ __Header__;
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* Tracks the public state. */
    int __Public__;
    /* References the parameters. */
    __Ast_Function_Parameter__ *__Parameters__;
    /* Stores the parameter count. */
    size_t __Parameter_Count__;
    /* Stores the output. */
    __Ast_Slot__ __Output__;
    /* References the body. */
    __Ast_Block__ *__Body__;
};

#endif
