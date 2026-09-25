/* Declares filesystem path primitives. */

#ifndef SULTANC__SUPPORT_PATH_FILESYSTEM_H__
#define SULTANC__SUPPORT_PATH_FILESYSTEM_H__

/* Returns the path exists. */
int __Path_Exists__(const char *__Path__);

/* Checks whether the path has suffix. */
int __Path_Has_Suffix__(const char *__Path__, const char *__Suffix__);

#endif
