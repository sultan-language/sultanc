/* Declares canonical source path helpers. */

#ifndef SULTANC__SUPPORT_PATH_SOURCE_PATH_H__
#define SULTANC__SUPPORT_PATH_SOURCE_PATH_H__

#include "core/source.h"

/* Returns the source path directory. */
char *__Source_Path_Directory__(const char *__Path__);

/* Joins the path text. */
char *__Path_Join_Text__(const char *__Directory__, __Text_Slice__ __Relative__);

/* Canonicalizes the source path. */
char *__Source_Path_Canonical__(const char *__Path__);

#endif
