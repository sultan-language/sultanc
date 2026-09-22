#include "support/path/filesystem.h"
#include "support/path/source_extension.h"

int __Source_Path_Is_SultanC__(const char *__Path__)
{
    return __Path_Has_Suffix__(__Path__, ".sn");
}
