#ifndef SULTANC__FRONTEND_AST_TYPE_H__
#define SULTANC__FRONTEND_AST_TYPE_H__

#include "base.h"

typedef enum
{
    __Ast_Type_Any__,
    __Ast_Type_Boolean__,
    __Ast_Type_Machine__,
    __Ast_Type_Integer__,
    __Ast_Type_Unsigned_Integer__,
    __Ast_Type_Character__,
    __Ast_Type_String__,
    __Ast_Type_Box__,
    __Ast_Type_Option__,
    __Ast_Type_Result__,
    __Ast_Type_Named__,
    __Ast_Type_Reference__,
    __Ast_Type_Vector__,
    __Ast_Type_Mutable__,
    __Ast_Type_Void__
} __Ast_Type_Kind__;

typedef struct
{
    __Ast_Type__ *__Ok__;
    __Ast_Type__ *__Error__;
} __Ast_Result_Type__;

typedef struct
{
    __Text_Slice__ __Name__;
} __Ast_Named_Type__;

struct __Ast_Type__
{
    __Ast_Type_Kind__ __Kind__;
    union
    {
        __Machine_Type__ __Machine__;
        __Ast_Result_Type__ __Result__;
        __Ast_Named_Type__ __Named__;
        __Ast_Type__ *__Inner__;
    } __As__;
};

typedef struct
{
    __Ast_Header__ __Header__;
    __Ast_Type__ *__Type__;
    /* NULL = inferred */
} __Ast_Slot__;

typedef struct
{
    __Text_Slice__ __Name__;
    __Ast_Slot__ __Slot__;
} __Ast_Struct_Field__;

typedef struct
{
    __Text_Slice__ __Name__;
    __Ast_Slot__ *__Payload_Slots__;
    size_t __Payload_Count__;
} __Ast_Enum_Constructor__;

typedef enum
{
    __Ast_Type_Decl_Struct__,
    __Ast_Type_Decl_Enum__
} __Ast_Type_Decl_Kind__;

typedef struct
{
    __Text_Slice__ __Name__;
    __Ast_Type_Decl_Kind__ __Kind__;
    union
    {
        struct
        {
            __Ast_Struct_Field__ *__Fields__;
            size_t __Count__;
        } __Struct__;
        struct
        {
            __Ast_Enum_Constructor__ *__Constructors__;
            size_t __Count__;
        } __Enum__;
    } __As__;
} __Ast_Type_Declaration__;

#endif
