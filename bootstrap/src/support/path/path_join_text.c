#include "support/path/source_path.h"

#include <stdlib.h>
#include <string.h>

char *__Path_Join_Text__(const char *__Directory__, __Text_Slice__ __Relative__)
{
    size_t __Directory_Length__;
    size_t __Slash__;
    size_t __Length__;
    char *__Out__;

    __Directory_Length__ = strlen(__Directory__);
    __Slash__ =
        (__Directory_Length__ > 0U && __Directory__[__Directory_Length__ - 1U] != '/') ? 1U : 0U;
    __Length__ = __Directory_Length__ + __Slash__ + __Relative__.__Length__;

    __Out__ = (char *)malloc(__Length__ + 1U);
    if (__Out__ == NULL)
    {
        return NULL;
    }

    memcpy(__Out__, __Directory__, __Directory_Length__);
    if (__Slash__ != 0U)
    {
        __Out__[__Directory_Length__] = '/';
    }
    memcpy(
        __Out__ + __Directory_Length__ + __Slash__, __Relative__.__Data__, __Relative__.__Length__);
    __Out__[__Length__] = '\0';
    return __Out__;
}
