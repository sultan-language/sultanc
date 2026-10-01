/* Returns the directory portion of a source path. */

#include "support/path/source_path.h"
#include "support/path/path.h"

#include <stdlib.h>
#include <string.h>

/* Returns the source path directory. */
char *__Source_Path_Directory__(const char *__Path__)
{
    /* References the slash. */
    const char *__Slash__;
    /* Stores the length. */
    size_t __Length__;
    /* References the directory. */
    char *__Directory__;

    if (__Path__ == NULL || __Path__[0] == '\0')
    {
        return __Path_Duplicate_C_String__(".");
    }

    __Slash__ = strrchr(__Path__, '/');
    if (__Slash__ == NULL)
    {
        return __Path_Duplicate_C_String__(".");
    }

    __Length__ = (size_t)(__Slash__ - __Path__);
    if (__Length__ == 0U)
    {
        __Length__ = 1U;
    }

    __Directory__ = (char *)malloc(__Length__ + 1U);
    if (__Directory__ == NULL)
    {
        return NULL;
    }

    memcpy(__Directory__, __Path__, __Length__);
    __Directory__[__Length__] = '\0';
    return __Directory__;
}
