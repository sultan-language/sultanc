/* Checks whether a filesystem path exists. */

#include "support/path/filesystem.h"

#include <stddef.h>
#include <sys/stat.h>

/* MSVC's <sys/stat.h> has the mode bits but not the POSIX test macro. */
#if !defined(S_ISREG)
#if defined(_S_IFMT) && defined(_S_IFREG)
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#else
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#endif

/* Returns the path exists. */
int __Path_Exists__(const char *__Path__)
{
    /* Stores the status. */
    struct stat __Status__;

    return __Path__ != NULL && stat(__Path__, &__Status__) == 0 && S_ISREG(__Status__.st_mode);
}
