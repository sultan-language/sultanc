/* Recognizes SultanC source-file extensions. */

#include "support/path/filesystem.h"
#include "support/path/source_extension.h"

/* Checks whether the source path is sultan c. */
int __Source_Path_Is_SultanC__(const char *__Path__)
{
    return __Path_Has_Suffix__(__Path__, ".sn");
}
