#ifndef SULTANC__KERNEL_TYPE_BUILTIN_H__
#define SULTANC__KERNEL_TYPE_BUILTIN_H__

typedef enum
{
#define SULTANC__TYPE_BUILTIN__(                                                                   \
    Id, Token, AstKind, ResolvedKind, Bits, Scalar, Signed, Stage1, Name)                          \
    __Type_Builtin_##Id##__,
#define SULTANC__TYPE_MACHINE__(                                                                   \
    Id, Token, Machine, ResolvedKind, Bits, Scalar, Signed, Stage1, Name)                          \
    __Type_Builtin_##Id##__,
#include "type_registry.def"
#undef SULTANC__TYPE_MACHINE__
#undef SULTANC__TYPE_BUILTIN__
    __Type_Builtin_Count__,
    __Type_Builtin_Invalid__ = __Type_Builtin_Count__
} __Type_Builtin_Id__;

typedef enum
{
    __Type_Builtin_Class_Unknown__,
    __Type_Builtin_Class_Boolean__,
    __Type_Builtin_Class_Signed_Integer__,
    __Type_Builtin_Class_Unsigned_Integer__,
    __Type_Builtin_Class_Float__,
    __Type_Builtin_Class_Character__,
    __Type_Builtin_Class_String__,
    __Type_Builtin_Class_Void__
} __Type_Builtin_Class__;

typedef struct
{
    __Type_Builtin_Id__ __Id__;
    __Type_Builtin_Class__ __Class__;
    unsigned __Bits__;
    int __Scalar__;
    int __Signed__;
    const char *__Diagnostic_Name__;
} __Type_Builtin_Descriptor__;

const __Type_Builtin_Descriptor__ *__Type_Builtin_Get_Descriptor__(__Type_Builtin_Id__ __Id__);

#endif
