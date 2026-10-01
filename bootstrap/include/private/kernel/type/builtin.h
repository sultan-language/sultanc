/* Declares built-in type descriptors. */

#ifndef SULTANC__KERNEL_TYPE_BUILTIN_H__
#define SULTANC__KERNEL_TYPE_BUILTIN_H__

/* Defines the type builtin ID values. */
typedef enum
{
/* Expands the type builtin macro. */
#define SULTANC__TYPE_BUILTIN__(                                                                   \
    Id, Token, AstKind, ResolvedKind, Bits, Scalar, Signed, Stage1, Name)                          \
    __Type_Builtin_##Id##__,
/* Expands the type machine macro. */
#define SULTANC__TYPE_MACHINE__(                                                                   \
    Id, Token, Machine, ResolvedKind, Bits, Scalar, Signed, Stage1, Name)                          \
    __Type_Builtin_##Id##__,
#include "type_registry.def"
#undef SULTANC__TYPE_MACHINE__
#undef SULTANC__TYPE_BUILTIN__
    /* Represents the type builtin count value. */
    __Type_Builtin_Count__,
    /* Represents the type builtin invalid value. */
    __Type_Builtin_Invalid__ = __Type_Builtin_Count__
} __Type_Builtin_Id__;

/* Defines the type builtin class values. */
typedef enum
{
    /* Represents the type builtin class unknown value. */
    __Type_Builtin_Class_Unknown__,
    /* Represents the type builtin class boolean value. */
    __Type_Builtin_Class_Boolean__,
    /* Represents the type builtin class signed integer value. */
    __Type_Builtin_Class_Signed_Integer__,
    /* Represents the type builtin class unsigned integer value. */
    __Type_Builtin_Class_Unsigned_Integer__,
    /* Represents the type builtin class float value. */
    __Type_Builtin_Class_Float__,
    /* Represents the type builtin class character value. */
    __Type_Builtin_Class_Character__,
    /* Represents the type builtin class string value. */
    __Type_Builtin_Class_String__,
    /* Represents the type builtin class void value. */
    __Type_Builtin_Class_Void__
} __Type_Builtin_Class__;

/* Defines the type builtin descriptor structure. */
typedef struct
{
    /* Stores the ID. */
    __Type_Builtin_Id__ __Id__;
    /* Stores the class. */
    __Type_Builtin_Class__ __Class__;
    /* Stores the bits. */
    unsigned __Bits__;
    /* Stores the scalar. */
    int __Scalar__;
    /* Stores the signed. */
    int __Signed__;
    /* References the diagnostic name. */
    const char *__Diagnostic_Name__;
} __Type_Builtin_Descriptor__;

/* Returns the type builtin descriptor. */
const __Type_Builtin_Descriptor__ *__Type_Builtin_Get_Descriptor__(__Type_Builtin_Id__ __Id__);

#endif
