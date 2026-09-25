/* Maps built-in type descriptors. */

#include "kernel/type/builtin.h"

#include <stddef.h>

/* Stores the type builtins. */
static const __Type_Builtin_Descriptor__ __Type_Builtins__[__Type_Builtin_Count__] = {
/* Expands the type builtin macro. */
#define SULTANC__TYPE_BUILTIN__(                                                                   \
    Id, Token, AstKind, ResolvedKind, Bits, Scalar, Signed, Stage1, Name)                          \
    [__Type_Builtin_##Id##__] = {__Type_Builtin_##Id##__,                                          \
                                 __Type_Builtin_Class_##ResolvedKind##__,                          \
                                 Bits##U,                                                          \
                                 Scalar,                                                           \
                                 Signed,                                                           \
                                 Name},
/* Expands the type machine macro. */
#define SULTANC__TYPE_MACHINE__(                                                                   \
    Id, Token, Machine, ResolvedKind, Bits, Scalar, Signed, Stage1, Name)                          \
    [__Type_Builtin_##Id##__] = {__Type_Builtin_##Id##__,                                          \
                                 __Type_Builtin_Class_##ResolvedKind##__,                          \
                                 Bits##U,                                                          \
                                 Scalar,                                                           \
                                 Signed,                                                           \
                                 Name},
#include "kernel/type/type_registry.def"
#undef SULTANC__TYPE_MACHINE__
#undef SULTANC__TYPE_BUILTIN__
};

/* Returns the type builtin descriptor. */
const __Type_Builtin_Descriptor__ *__Type_Builtin_Get_Descriptor__(__Type_Builtin_Id__ __Id__)
{
    if ((size_t)__Id__ >= (size_t)__Type_Builtin_Count__)
    {
        return NULL;
    }
    return &__Type_Builtins__[__Id__];
}
