/* Defines expression AST nodes. */

#ifndef SULTANC__FRONTEND_AST_EXPRESSION_H__
#define SULTANC__FRONTEND_AST_EXPRESSION_H__

#include "base.h"
#include "type.h"

/* Defines the AST literal kind values. */
typedef enum
{
    /* Represents the AST literal boolean value. */
    __Ast_Literal_Boolean__,
    /* Represents the AST literal integer value. */
    __Ast_Literal_Integer__,
    /* Represents the AST literal character value. */
    __Ast_Literal_Character__,
    /* Represents the AST literal string value. */
    __Ast_Literal_String__,
    /* Represents the AST literal undefined value. */
    __Ast_Literal_Undefined__
} __Ast_Literal_Kind__;

/* Defines the AST literal structure. */
struct __Ast_Literal__
{
    /* Stores the header. */
    __Ast_Header__ __Header__;
    /* Stores the kind. */
    __Ast_Literal_Kind__ __Kind__;
    /* Defines the struct structure. */
    union
    {
        /* Stores the boolean. */
        int __Boolean__;
        /* Defines the struct structure. */
        struct
        {
            /* Stores the value. */
            int64_t __Value__;
            /* Stores the spelling. */
            __Text_Slice__ __Spelling__;
        } __Integer__;
        /* Stores the character. */
        uint32_t __Character__;
        /* Stores the string. */
        __Text_Slice__ __String__;
    } __As__;
};

/* Defines the AST lvalue base kind values. */
typedef enum
{
    /* Represents the AST lvalue base identifier value. */
    __Ast_Lvalue_Base_Identifier__,
    /* Represents the AST lvalue base temporary value. */
    __Ast_Lvalue_Base_Temporary__
} __Ast_Lvalue_Base_Kind__;

/* Defines the AST lvalue kind values. */
typedef enum
{
    /* Represents the AST lvalue base value. */
    __Ast_Lvalue_Base__,
    /* Represents the AST lvalue field value. */
    __Ast_Lvalue_Field__,
    /* Represents the AST lvalue index value. */
    __Ast_Lvalue_Index__,
    /* Represents the AST lvalue dereference value. */
    __Ast_Lvalue_Dereference__
} __Ast_Lvalue_Kind__;

/* Defines the AST atom kind values. */
typedef enum
{
    /* Represents the AST atom literal value. */
    __Ast_Atom_Literal__,
    /* Represents the AST atom lvalue value. */
    __Ast_Atom_Lvalue__
} __Ast_Atom_Kind__;

/* Defines the AST atom structure. */
typedef struct
{
    /* Stores the kind. */
    __Ast_Atom_Kind__ __Kind__;
    /* Stores the active variant payload. */
    union
    {
        /* References the literal. */
        __Ast_Literal__ *__Literal__;
        /* References the lvalue. */
        __Ast_Lvalue__ *__Lvalue__;
    } __As__;
} __Ast_Atom__;

/* Defines the AST lvalue structure. */
struct __Ast_Lvalue__
{
    /* Stores the header. */
    __Ast_Header__ __Header__;
    /* Stores the kind. */
    __Ast_Lvalue_Kind__ __Kind__;
    /* Defines the struct structure. */
    union
    {
        /* Defines the struct structure. */
        struct
        {
            /* Stores the kind. */
            __Ast_Lvalue_Base_Kind__ __Kind__;
            /* Defines the struct structure. */
            union
            {
                /* Stores the identifier. */
                __Text_Slice__ __Identifier__;
                /* Stores the temporary. */
                __Temporary_Id__ __Temporary__;
            } __As__;
        } __Base__;
        /* Defines the struct structure. */
        struct
        {
            /* References the parent. */
            __Ast_Lvalue__ *__Parent__;
            /* Stores the field. */
            __Text_Slice__ __Field__;
        } __Field__;
        /* Defines the struct structure. */
        struct
        {
            /* References the parent. */
            __Ast_Lvalue__ *__Parent__;
            /* Tracks the index. */
            __Ast_Atom__ __Index__;
        } __Index__;
        /* References the dereference parent. */
        __Ast_Lvalue__ *__Dereference_Parent__;
    } __As__;
};

/* Defines the AST binary operation values. */
typedef enum
{
    /* Represents the binary or value. */
    __Binary_Or__,
    /* Represents the binary and value. */
    __Binary_And__,
    /* Represents the binary xor value. */
    __Binary_Xor__,
    /* Represents the binary equal value. */
    __Binary_Equal__,
    /* Represents the binary not equal value. */
    __Binary_Not_Equal__,
    /* Represents the binary less than value. */
    __Binary_Less_Than__,
    /* Represents the binary less or equal value. */
    __Binary_Less_Or_Equal__,
    /* Represents the binary greater or equal value. */
    __Binary_Greater_Or_Equal__,
    /* Represents the binary greater than value. */
    __Binary_Greater_Than__,
    /* Represents the binary shift left logical value. */
    __Binary_Shift_Left_Logical__,
    /* Represents the binary shift right logical value. */
    __Binary_Shift_Right_Logical__,
    /* Represents the binary add value. */
    __Binary_Add__,
    /* Represents the binary subtract value. */
    __Binary_Subtract__,
    /* Represents the binary multiply value. */
    __Binary_Multiply__,
    /* Represents the binary divide value. */
    __Binary_Divide__,
    /* Represents the binary modulo value. */
    __Binary_Modulo__,
    /* Represents the binary logical and value. */
    __Binary_Logical_And__,
    /* Represents the binary logical or value. */
    __Binary_Logical_Or__
} __Ast_Binary_Operation__;

/* Defines the AST unary operation values. */
typedef enum
{
    /* Represents the unary not value. */
    __Unary_Not__,
    /* Represents the unary bitwise not value. */
    __Unary_Bitwise_Not__,
    /* Represents the unary negate value. */
    __Unary_Negate__,
    /* Represents the unary dereference value. */
    __Unary_Dereference__,
    /* Represents the unary address value. */
    __Unary_Address__,
    /* Represents the unary address mutable value. */
    __Unary_Address_Mutable__
} __Ast_Unary_Operation__;

/* Defines the AST expression kind values. */
typedef enum
{
    /* Represents the AST expression binary value. */
    __Ast_Expression_Binary__,
    /* Represents the AST expression unary value. */
    __Ast_Expression_Unary__,
    /* Represents the AST expression atom value. */
    __Ast_Expression_Atom__,
    /* Represents the AST expression call value. */
    __Ast_Expression_Call__,
    /* Represents the AST expression conversion value. */
    __Ast_Expression_Conversion__
} __Ast_Expression_Kind__;

/* Defines the AST expression structure. */
struct __Ast_Expression__
{
    /* Stores the header. */
    __Ast_Header__ __Header__;
    /* Stores the kind. */
    __Ast_Expression_Kind__ __Kind__;
    /* References the contextual type. */
    __Ast_Type__ *__Contextual_Type__;
    /* Semantic side effects such as moves/releases are applied at most once. */
    int __Semantic_Effects_Applied__;
    /* Defines the struct structure. */
    union
    {
        /* Defines the struct structure. */
        struct
        {
            /* Stores the operation. */
            __Ast_Binary_Operation__ __Operation__;
            /* References the left. */
            __Ast_Expression__ *__Left__;
            /* References the right. */
            __Ast_Expression__ *__Right__;
        } __Binary__;
        /* Defines the struct structure. */
        struct
        {
            /* Stores the operation. */
            __Ast_Unary_Operation__ __Operation__;
            /* References the operand. */
            __Ast_Expression__ *__Operand__;
        } __Unary__;
        /* Stores the atom. */
        __Ast_Atom__ __Atom__;
        /* Defines the struct structure. */
        struct
        {
            /* References the function. */
            __Ast_Lvalue__ *__Function__;
            /* Stores the call arguments. */
            __Ast_Expression__ **__Arguments__;
            /* Stores the argument count. */
            size_t __Argument_Count__;
        } __Call__;
        /* Defines the struct structure. */
        struct
        {
            /* References the operand. */
            __Ast_Expression__ *__Operand__;
            /* References the target type. */
            __Ast_Type__ *__Target_Type__;
        } __Conversion__;
    } __As__;
};

#endif
