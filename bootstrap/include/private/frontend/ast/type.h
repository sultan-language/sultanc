/* Defines type AST nodes. */

#ifndef SULTANC__FRONTEND_AST_TYPE_H__
#define SULTANC__FRONTEND_AST_TYPE_H__

#include "base.h"

/* Defines the AST type kind values. */
typedef enum
{
    /* Represents the AST type any value. */
    __Ast_Type_Any__,
    /* Represents the AST type boolean value. */
    __Ast_Type_Boolean__,
    /* Represents the AST type machine value. */
    __Ast_Type_Machine__,
    /* Represents the AST type integer value. */
    __Ast_Type_Integer__,
    /* Represents the AST type unsigned integer value. */
    __Ast_Type_Unsigned_Integer__,
    /* Represents the AST type character value. */
    __Ast_Type_Character__,
    /* Represents the AST type string value. */
    __Ast_Type_String__,
    /* Represents the AST type box value. */
    __Ast_Type_Box__,
    /* Represents the AST type option value. */
    __Ast_Type_Option__,
    /* Represents the AST type result value. */
    __Ast_Type_Result__,
    /* Represents the AST type named value. */
    __Ast_Type_Named__,
    /* Represents the AST type reference value. */
    __Ast_Type_Reference__,
    /* Represents the AST type vector value. */
    __Ast_Type_Vector__,
    /* Represents the AST type mutable value. */
    __Ast_Type_Mutable__,
    /* Represents the AST type void value. */
    __Ast_Type_Void__
} __Ast_Type_Kind__;

/* Defines the AST result type structure. */
typedef struct
{
    /* Tracks whether the operation succeeded. */
    __Ast_Type__ *__Ok__;
    /* References the error. */
    __Ast_Type__ *__Error__;
} __Ast_Result_Type__;

/* Defines the AST named type structure. */
typedef struct
{
    /* Stores the name. */
    __Text_Slice__ __Name__;
} __Ast_Named_Type__;

/* Defines the AST type structure. */
struct __Ast_Type__
{
    /* Stores the kind. */
    __Ast_Type_Kind__ __Kind__;
    /* Defines the struct structure. */
    union
    {
        /* Stores the machine. */
        __Machine_Type__ __Machine__;
        /* Stores the operation result. */
        __Ast_Result_Type__ __Result__;
        /* Stores the named. */
        __Ast_Named_Type__ __Named__;
        /* References the inner. */
        __Ast_Type__ *__Inner__;
    } __As__;
};

/* Defines the AST slot structure. */
typedef struct
{
    /* Stores the header. */
    __Ast_Header__ __Header__;
    /* References the type. */
    __Ast_Type__ *__Type__;
    /* NULL requests inferred type. */
} __Ast_Slot__;

/* Defines the AST struct field structure. */
typedef struct
{
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* Stores the slot. */
    __Ast_Slot__ __Slot__;
} __Ast_Struct_Field__;

/* Defines the AST enum constructor structure. */
typedef struct
{
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* References the payload slots. */
    __Ast_Slot__ *__Payload_Slots__;
    /* Stores the payload count. */
    size_t __Payload_Count__;
} __Ast_Enum_Constructor__;

/* Defines the AST type declaration kind values. */
typedef enum
{
    /* Represents the AST type declaration struct value. */
    __Ast_Type_Decl_Struct__,
    /* Represents the AST type declaration enum value. */
    __Ast_Type_Decl_Enum__
} __Ast_Type_Decl_Kind__;

/* Defines the AST type declaration structure. */
typedef struct
{
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* Stores the kind. */
    __Ast_Type_Decl_Kind__ __Kind__;
    /* Stores the active variant payload. */
    union
    {
        /* Stores the struct. */
        struct
        {
            /* References the fields. */
            __Ast_Struct_Field__ *__Fields__;
            /* Stores the count. */
            size_t __Count__;
        } __Struct__;
        /* Stores the enum. */
        struct
        {
            /* References the constructors. */
            __Ast_Enum_Constructor__ *__Constructors__;
            /* Stores the count. */
            size_t __Count__;
        } __Enum__;
    } __As__;
} __Ast_Type_Declaration__;

#endif
