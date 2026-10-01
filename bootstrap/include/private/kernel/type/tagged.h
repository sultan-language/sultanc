/* Declares tagged-type constructor queries. */

#ifndef SULTANC__KERNEL_TYPE_TAGGED_H__
#define SULTANC__KERNEL_TYPE_TAGGED_H__

#include "type.h"

/* Defines the type tagged constructor structure. */
typedef struct
{
    /* Stores the tag. */
    size_t __Tag__;
    /* Stores the payload count. */
    size_t __Payload_Count__;
    /* References the payload type. */
    __Ast_Type__ *__Payload_Type__;
} __Type_Tagged_Constructor__;

/* Checks whether the type is builtin tagged. */
int __Type_Is_Builtin_Tagged__(__Ast_Type__ *__Type__);

/* Returns the type tagged constructor count. */
size_t __Type_Tagged_Constructor_Count__(__Ast_Type__ *__Type__);

/* Returns the type tagged constructor at. */
int __Type_Tagged_Constructor_At__(__Ast_Type__ *__Type__,
                                   size_t __Tag__,
                                   __Type_Tagged_Constructor__ *__Out_Constructor__);

/* Finds the type tagged constructor. */
int __Type_Tagged_Find_Constructor__(__Ast_Type__ *__Type__,
                                     __Text_Slice__ __Name__,
                                     __Type_Tagged_Constructor__ *__Out_Constructor__);

/* Checks the type tagged name. */
int __Type_Tagged_Name_Matches__(__Ast_Type__ *__Type__, __Text_Slice__ __Name__);

#endif
