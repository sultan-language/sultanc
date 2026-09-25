/* Duplicates path strings. */

#include "support/path/path.h"

#include <stdlib.h>
#include <string.h>

/* Duplicates the path c string. */
char *__Path_Duplicate_C_String__(const char *__Text__)
{
    /* Stores the length. */
    size_t __Length__;
    /* References the copy. */
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
