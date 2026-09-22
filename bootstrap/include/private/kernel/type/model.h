#ifndef SULTANC__KERNEL_TYPE_MODEL_H__
#define SULTANC__KERNEL_TYPE_MODEL_H__

#include "semantic/context.h"
#include "builtin.h"

typedef enum
{
    __Resolved_Type_Unknown__,
    __Resolved_Type_Boolean__,
    __Resolved_Type_Signed_Integer__,
    __Resolved_Type_Unsigned_Integer__,
    __Resolved_Type_Float__,
    __Resolved_Type_Character__,
    __Resolved_Type_String__,
    __Resolved_Type_Reference__,
    __Resolved_Type_Vector__,
    __Resolved_Type_Box__,
    __Resolved_Type_Option__,
    __Resolved_Type_Result__,
    __Resolved_Type_Struct__,
    __Resolved_Type_Enum__,
    __Resolved_Type_Void__
} __Resolved_Type_Kind__;

typedef struct
{
    __Resolved_Type_Kind__ __Kind__;
    __Type_Builtin_Id__ __Builtin__;
    unsigned __Bits__;
    __Ast_Type__ *__Ast__;
    __Semantic_Type_Entry__ *__Named__;
    __Ast_Type__ *__Inner__;
    int __Mutable__;
    int __Reference_Mutable__;
} __Resolved_Type__;

#endif
