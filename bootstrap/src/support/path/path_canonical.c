/* Builds canonical source paths. */

#if !defined(_WIN32)
/* Defines the xopen source macro. */
#define _XOPEN_SOURCE 700
#endif

#include <stdlib.h>
#include "support/path/source_path.h"
#include "support/path/path.h"

/* Canonicalizes the source path. */
char *__Source_Path_Canonical__(const char *__Path__)
{
#if defined(__APPLE__) || defined(__unix__)
    /* References the resolved. */
    char *__Resolved__;

    __Resolved__ = realpath(__Path__, NULL);
    if (__Resolved__ != NULL)
    {
        return __Resolved__;
    }
#endif
    return __Path_Duplicate_C_String__(__Path__);
}
