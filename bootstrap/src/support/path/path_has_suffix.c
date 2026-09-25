/* Checks path suffixes. */

#include "support/path/filesystem.h"

#include <string.h>

/* Checks whether the path has suffix. */
int __Path_Has_Suffix__(const char *__Path__, const char *__Suffix__)
{
    /* Stores the path length. */
    size_t __Path_Length__;
    /* Stores the suffix length. */
    size_t __Suffix_Length__;

    if (__Path__ == NULL || __Suffix__ == NULL)
    {
        return 0;
    }

    __Path_Length__ = strlen(__Path__);
    __Suffix_Length__ = strlen(__Suffix__);
    if (__Suffix_Length__ > __Path_Length__)
    {
        return 0;
    }

    return memcmp(__Path__ + __Path_Length__ - __Suffix_Length__, __Suffix__, __Suffix_Length__) ==
           0;
}
