#ifndef SULTANC__KERNEL_TYPE_CONVERSION_H__
#define SULTANC__KERNEL_TYPE_CONVERSION_H__

#include "type.h"

typedef enum
{
    __Type_Conversion_Invalid__,
    __Type_Conversion_Implicit_Safe__,
    __Type_Conversion_Explicit_Safe__
} __Type_Conversion_Class__;

int __Type_Integer_Literal_Fits__(__Semantic_Context__ *__Context__,
                                  int64_t __Value__,
                                  __Ast_Type__ *__Target_Type__);

__Type_Conversion_Class__ __Type_Conversion_Classify__(__Semantic_Context__ *__Context__,
                                                       __Ast_Type__ *__Source__,
                                                       __Ast_Type__ *__Target__);

/* Compatibility wrapper for existing implicit assignment/call sites. */
int __Type_Conversion_Is_Safe__(__Semantic_Context__ *__Context__,
                                __Ast_Type__ *__Source__,
                                __Ast_Type__ *__Target__);

#endif
