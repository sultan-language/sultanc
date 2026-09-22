#ifndef SULTANC__SUPPORT_PATH_SOURCE_PATH_H__
#define SULTANC__SUPPORT_PATH_SOURCE_PATH_H__

#include "core/source.h"

char *__Source_Path_Directory__(const char *__Path__);

char *__Path_Join_Text__(const char *__Directory__, __Text_Slice__ __Relative__);

char *__Source_Path_Canonical__(const char *__Path__);

#endif
