/* GENERATED FILE — DO NOT EDIT.
 * Owner: tools/generate/registry/type_registry.def
 */
#ifndef SULTANC__KERNEL_TYPE_CONTEXTUAL_GENERATED_H__
#define SULTANC__KERNEL_TYPE_CONTEXTUAL_GENERATED_H__

#include "core/source.h"

static inline int __Type_Contextual_Name_Is_OPTION__(__Text_Slice__ __Name__)
{
    return __Text_Equals_Cstr__(__Name__, "option") ||
           __Text_Equals_Cstr__(__Name__, "اختيار");
}

static inline int __Type_Contextual_Name_Is_RESULT__(__Text_Slice__ __Name__)
{
    return __Text_Equals_Cstr__(__Name__, "result") ||
           __Text_Equals_Cstr__(__Name__, "نتيجة");
}

static inline int __Type_Contextual_Name_Is_VECTOR__(__Text_Slice__ __Name__)
{
    return __Text_Equals_Cstr__(__Name__, "vector") ||
           __Text_Equals_Cstr__(__Name__, "متجه");
}

#endif
