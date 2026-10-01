/* Defines canonical resolved type facts. */

#ifndef SULTANC__KERNEL_TYPE_MODEL_H__
#define SULTANC__KERNEL_TYPE_MODEL_H__

#include "semantic/context.h"
#include "builtin.h"

/* Defines the resolved type kind values. */
typedef enum
{
    /* Represents the resolved type unknown value. */
    __Resolved_Type_Unknown__,
    /* Represents the resolved type boolean value. */
    __Resolved_Type_Boolean__,
    /* Represents the resolved type signed integer value. */
    __Resolved_Type_Signed_Integer__,
    /* Represents the resolved type unsigned integer value. */
    __Resolved_Type_Unsigned_Integer__,
    /* Represents the resolved type float value. */
    __Resolved_Type_Float__,
    /* Represents the resolved type character value. */
    __Resolved_Type_Character__,
    /* Represents the resolved type string value. */
    __Resolved_Type_String__,
    /* Represents the resolved type reference value. */
    __Resolved_Type_Reference__,
    /* Represents the resolved type vector value. */
    __Resolved_Type_Vector__,
    /* Represents the resolved type box value. */
    __Resolved_Type_Box__,
    /* Represents the resolved type option value. */
    __Resolved_Type_Option__,
    /* Represents the resolved type result value. */
    __Resolved_Type_Result__,
    /* Represents the resolved type struct value. */
    __Resolved_Type_Struct__,
    /* Represents the resolved type enum value. */
    __Resolved_Type_Enum__,
    /* Represents the resolved type void value. */
    __Resolved_Type_Void__
} __Resolved_Type_Kind__;

/* Defines the resolved type structure. */
typedef struct
{
    /* Stores the kind. */
    __Resolved_Type_Kind__ __Kind__;
    /* Stores the builtin. */
    __Type_Builtin_Id__ __Builtin__;
    /* Stores the bits. */
    unsigned __Bits__;
    /* References the AST. */
    __Ast_Type__ *__Ast__;
    /* References the named. */
    __Semantic_Type_Entry__ *__Named__;
    /* References the inner. */
    __Ast_Type__ *__Inner__;
    /* Tracks the mutable state. */
    int __Mutable__;
    /* Tracks the reference mutable state. */
    int __Reference_Mutable__;
} __Resolved_Type__;

#endif
