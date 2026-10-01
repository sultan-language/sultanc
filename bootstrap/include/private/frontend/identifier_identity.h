/* Declares semantic identifier identity helpers. */

#ifndef SULTANC__FRONTEND_IDENTIFIER_IDENTITY_H__
#define SULTANC__FRONTEND_IDENTIFIER_IDENTITY_H__

#include "core/source.h"

#include <stdint.h>

/* Returns the identifier identity ignores scalar. */
int __Identifier_Identity_Ignores_Scalar__(uint32_t __Scalar__);

/* Checks whether the identifier identity has content. */
int __Identifier_Identity_Has_Content__(__Text_Slice__ __Text__);

/* Compares the identifier identity. */
int __Identifier_Identity_Equals__(__Text_Slice__ __Left__, __Text_Slice__ __Right__);

/* Compares the identifier identity C string. */
int __Identifier_Identity_Equals_Cstr__(__Text_Slice__ __Left__, const char *__Right__);

#endif
