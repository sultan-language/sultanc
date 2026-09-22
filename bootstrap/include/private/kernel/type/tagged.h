#ifndef SULTANC__KERNEL_TYPE_TAGGED_H__
#define SULTANC__KERNEL_TYPE_TAGGED_H__

#include "type.h"

typedef struct
{
    size_t __Tag__;
    size_t __Payload_Count__;
    __Ast_Type__ *__Payload_Type__;
} __Type_Tagged_Constructor__;

int __Type_Is_Builtin_Tagged__(__Ast_Type__ *__Type__);

size_t __Type_Tagged_Constructor_Count__(__Ast_Type__ *__Type__);

int __Type_Tagged_Constructor_At__(__Ast_Type__ *__Type__,
                                   size_t __Tag__,
                                   __Type_Tagged_Constructor__ *__Out_Constructor__);

int __Type_Tagged_Find_Constructor__(__Ast_Type__ *__Type__,
                                     __Text_Slice__ __Name__,
                                     __Type_Tagged_Constructor__ *__Out_Constructor__);

int __Type_Tagged_Name_Matches__(__Ast_Type__ *__Type__, __Text_Slice__ __Name__);

#endif
