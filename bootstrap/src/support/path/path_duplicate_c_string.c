#include "support/path/path.h"

#include <stdlib.h>
#include <string.h>

char *__Path_Duplicate_C_String__(const char *__Text__)
{
    size_t __Length__;
    char *__Copy__;

    if (__Text__ == NULL)
    {
        return NULL;
    }

    __Length__ = strlen(__Text__);
    __Copy__ = (char *)malloc(__Length__ + 1U);
    if (__Copy__ == NULL)
    {
        return NULL;
    }

    memcpy(__Copy__, __Text__, __Length__ + 1U);
    return __Copy__;
}
