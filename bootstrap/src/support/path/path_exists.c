#include "support/path/filesystem.h"

#include <stddef.h>
#include <sys/stat.h>

int __Path_Exists__(const char *__Path__)
{
    struct stat __Status__;

    return __Path__ != NULL && stat(__Path__, &__Status__) == 0 && S_ISREG(__Status__.st_mode);
}
