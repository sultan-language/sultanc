#ifndef SULTANC__FRONTEND_AST_EXPRESSION_H__
#define SULTANC__FRONTEND_AST_EXPRESSION_H__

#include "base.h"
#include "type.h"

typedef enum
{
    __Ast_Literal_Boolean__,
    __Ast_Literal_Integer__,
    __Ast_Literal_Character__,
    __Ast_Literal_String__,
    __Ast_Literal_Undefined__
} __Ast_Literal_Kind__;

struct __Ast_Literal__
{
    __Ast_Header__ __Header__;
    __Ast_Literal_Kind__ __Kind__;
    union
    {
        int __Boolean__;
        struct
        {
            int64_t __Value__;
            __Text_Slice__ __Spelling__;
        } __Integer__;
        uint32_t __Character__;
        __Text_Slice__ __String__;
    } __As__;
};

typedef enum
{
    __Ast_Lvalue_Base_Identifier__,
    __Ast_Lvalue_Base_Temporary__
} __Ast_Lvalue_Base_Kind__;

typedef enum
{
    __Ast_Lvalue_Base__,
    __Ast_Lvalue_Field__,
    __Ast_Lvalue_Index__,
    __Ast_Lvalue_Dereference__
} __Ast_Lvalue_Kind__;

typedef enum
{
    __Ast_Atom_Literal__,
    __Ast_Atom_Lvalue__
} __Ast_Atom_Kind__;

typedef struct
{
    __Ast_Atom_Kind__ __Kind__;
    union
    {
        __Ast_Literal__ *__Literal__;
        __Ast_Lvalue__ *__Lvalue__;
    } __As__;
} __Ast_Atom__;

struct __Ast_Lvalue__
{
    __Ast_Header__ __Header__;
    __Ast_Lvalue_Kind__ __Kind__;
    union
    {
        struct
        {
            __Ast_Lvalue_Base_Kind__ __Kind__;
            union
            {
                __Text_Slice__ __Identifier__;
                __Temporary_Id__ __Temporary__;
            } __As__;
        } __Base__;
        struct
        {
            __Ast_Lvalue__ *__Parent__;
            __Text_Slice__ __Field__;
        } __Field__;
        struct
        {
            __Ast_Lvalue__ *__Parent__;
            __Ast_Atom__ __Index__;
        } __Index__;
        __Ast_Lvalue__ *__Dereference_Parent__;
    } __As__;
};

typedef enum
{
    __Binary_Or__,
    __Binary_And__,
    __Binary_Xor__,
    __Binary_Equal__,
    __Binary_Not_Equal__,
    __Binary_Less_Than__,
    __Binary_Less_Or_Equal__,
    __Binary_Greater_Or_Equal__,
    __Binary_Greater_Than__,
    __Binary_Shift_Left_Logical__,
    __Binary_Shift_Right_Logical__,
    __Binary_Add__,
    __Binary_Subtract__,
    __Binary_Multiply__,
    __Binary_Divide__,
    __Binary_Modulo__,
    __Binary_Logical_And__,
    __Binary_Logical_Or__
} __Ast_Binary_Operation__;

typedef enum
{
    __Unary_Not__,
    __Unary_Bitwise_Not__,
    __Unary_Negate__,
    __Unary_Dereference__,
    __Unary_Address__,
    __Unary_Address_Mutable__
} __Ast_Unary_Operation__;

typedef enum
{
    __Ast_Expression_Binary__,
    __Ast_Expression_Unary__,
    __Ast_Expression_Atom__,
    __Ast_Expression_Call__,
    __Ast_Expression_Conversion__
} __Ast_Expression_Kind__;

struct __Ast_Expression__
{
    __Ast_Header__ __Header__;
    __Ast_Expression_Kind__ __Kind__;
    __Ast_Type__ *__Contextual_Type__;
    /* Semantic side effects such as moves/releases are applied at most once. */
    int __Semantic_Effects_Applied__;
    union
    {
        struct
        {
            __Ast_Binary_Operation__ __Operation__;
            __Ast_Expression__ *__Left__;
            __Ast_Expression__ *__Right__;
        } __Binary__;
        struct
        {
            __Ast_Unary_Operation__ __Operation__;
            __Ast_Expression__ *__Operand__;
        } __Unary__;
        __Ast_Atom__ __Atom__;
        struct
        {
            __Ast_Lvalue__ *__Function__;
            __Ast_Expression__ **__Arguments__;
            size_t __Argument_Count__;
        } __Call__;
        struct
        {
            __Ast_Expression__ *__Operand__;
            __Ast_Type__ *__Target_Type__;
        } __Conversion__;
    } __As__;
};

#endif
