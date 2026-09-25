/* Declares source import path resolution. */

#ifndef SULTANC_PRIVATE_SUPPORT_PATH_SOURCE_IMPORT_PATH_H
/* Defines the sultanc private support path source import path h macro. */
#define SULTANC_PRIVATE_SUPPORT_PATH_SOURCE_IMPORT_PATH_H

#include "core/source.h"

/* Resolves the source path import. */
char *__Source_Path_Resolve_Import__(const char *__Importer_Path__,
                                     const char *__Project_Root__,
                                     int __Logical__,
                                     __Text_Slice__ __Import__);

#endif
