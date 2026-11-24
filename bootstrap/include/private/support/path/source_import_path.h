/* Declares source import path resolution. */

#ifndef SULTANC_PRIVATE_SUPPORT_PATH_SOURCE_IMPORT_PATH_H
/* Defines the sultanc private support path source import path h macro. */
#define SULTANC_PRIVATE_SUPPORT_PATH_SOURCE_IMPORT_PATH_H

#include "core/source.h"

/* Resolves the source path import. */
char *__Source_Path_Resolve_Import__(const char *__Importer_Path__,
                                     const char *__Project_Root__,
                                     int __Logical__,
                                     __Text_Slice__ __Import__,
                                     int *__Escapes_Root__);

/* Binds a logical module to an explicit source file (--module-source=
 * <module>=<file>, as the self-hosted compiler's option): a logical import of
 * <module> resolves to <file>, relative to the project root unless absolute.
 * Answers 0 when the binding is malformed or cannot be stored. */
int __Source_Path_Bind_Module_Source__(const char *__Binding__);

#endif
