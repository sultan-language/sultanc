#include "core/source.h"

#include <string.h>

int __Text_Equals_Cstr__(__Text_Slice__ __Text__, const char *__Cstr__)
{
    size_t __Length__ = 0U;
    if (__Cstr__ == NULL)
    {
        return 0;
    }
    __Length__ = strlen(__Cstr__);
    return __Text__.__Length__ == __Length__ &&
           memcmp(__Text__.__Data__, __Cstr__, __Length__) == 0;
}
