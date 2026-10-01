/* Declares type conversion classification. */

#ifndef SULTANC__KERNEL_TYPE_CONVERSION_H__
#define SULTANC__KERNEL_TYPE_CONVERSION_H__

#include "type.h"

/* Defines the type conversion class values. */
typedef enum
{
    /* Represents the type conversion invalid value. */
    __Type_Conversion_Invalid__,
    /* Represents the type conversion implicit safe value. */
    __Type_Conversion_Implicit_Safe__,
    /* Represents the type conversion explicit safe value. */
    __Type_Conversion_Explicit_Safe__
} __Type_Conversion_Class__;

/* Returns the type integer literal fits. */
int __Type_Integer_Literal_Fits__(__Semantic_Context__ *__Context__,
                                  int64_t __Value__,
                                  __Ast_Type__ *__Target_Type__);

/* Classifies the type conversion. */
__Type_Conversion_Class__ __Type_Conversion_Classify__(__Semantic_Context__ *__Context__,
                                                       __Ast_Type__ *__Source__,
                                                       __Ast_Type__ *__Target__);

/* Compatibility wrapper for existing implicit assignment/call sites. */
int __Type_Conversion_Is_Safe__(__Semantic_Context__ *__Context__,
                                __Ast_Type__ *__Source__,
                                __Ast_Type__ *__Target__);

#endif
