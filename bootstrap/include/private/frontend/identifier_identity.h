#ifndef SULTANC__FRONTEND_IDENTIFIER_IDENTITY_H__
#define SULTANC__FRONTEND_IDENTIFIER_IDENTITY_H__

#include "core/source.h"

#include <stdint.h>

int __Identifier_Identity_Ignores_Scalar__(uint32_t __Scalar__);

int __Identifier_Identity_Has_Content__(__Text_Slice__ __Text__);

int __Identifier_Identity_Equals__(__Text_Slice__ __Left__, __Text_Slice__ __Right__);

int __Identifier_Identity_Equals_Cstr__(__Text_Slice__ __Left__, const char *__Right__);

#endif
