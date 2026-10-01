/* Resolves logical source import paths. */

#include "support/path/filesystem.h"
#include "support/path/source_path.h"
#include "support/path/source_import_path.h"

#include <stdlib.h>
#include <string.h>

/* Checks whether the import has prefix. */
static int __Import_Has_Prefix__(__Text_Slice__ __Import__, const char *__Prefix__)
{
    /* Stores the length. */
    size_t __Length__ = strlen(__Prefix__);
    return __Import__.__Length__ > __Length__ &&
           memcmp(__Import__.__Data__, __Prefix__, __Length__) == 0;
}

/* Checks whether a path is rooted under another path. */
static int __Path_Is_Under__(const char *__Path__, const char *__Root__)
{
    /* Stores the root length. */
    size_t __Root_Length__;

    if (__Path__ == NULL || __Root__ == NULL)
    {
        return 0;
    }
    __Root_Length__ = strlen(__Root__);
    return strncmp(__Path__, __Root__, __Root_Length__) == 0 &&
           (__Path__[__Root_Length__] == '/' || __Path__[__Root_Length__] == '\0');
}

/* Resolves the logical import. */
static char *__Resolve_Logical_Import__(const char *__Importer_Path__,
                                        const char *__Project_Root__,
                                        __Text_Slice__ __Import__)
{
    /* Stores the std prefix. */
    static const char __Std_Prefix__[] = "std/";
    /* Stores the std root. */
    static const char __Std_Root__[] = "library/std";
    /* Stores the relative import. */
    __Text_Slice__ __Relative__ = __Import__;
    /* References the base path. */
    char *__Base__ = NULL;
    /* References the candidate path. */
    char *__Candidate__ = NULL;

    if (__Project_Root__ == NULL)
    {
        return NULL;
    }

    {
        /* Stores the std root text. */
        __Text_Slice__ __Std_Root_Text__ = {__Std_Root__, sizeof(__Std_Root__) - 1U};
        __Base__ = __Path_Join_Text__(__Project_Root__, __Std_Root_Text__);
        if (__Base__ == NULL)
        {
            return NULL;
        }

        if (__Import_Has_Prefix__(__Import__, __Std_Prefix__))
        {
            __Relative__.__Data__ += sizeof(__Std_Prefix__) - 1U;
            __Relative__.__Length__ -= sizeof(__Std_Prefix__) - 1U;
            __Candidate__ = __Path_Join_Text__(__Base__, __Relative__);
            free(__Base__);
            return __Candidate__;
        }

        /* Root-local logical imports inside std stay rooted at library/std. */
        if (__Path_Is_Under__(__Importer_Path__, __Base__))
        {
            __Candidate__ = __Path_Join_Text__(__Base__, __Relative__);
            free(__Base__);
            return __Candidate__;
        }
        free(__Base__);
    }

    return __Path_Join_Text__(__Project_Root__, __Relative__);
}

/* Resolves the source path import. */
char *__Source_Path_Resolve_Import__(const char *__Importer_Path__,
                                     const char *__Project_Root__,
                                     int __Logical__,
                                     __Text_Slice__ __Import__)
{
    /* References the importer directory. */
    char *__Directory__ = NULL;
    /* References the candidate path. */
    char *__Candidate__ = NULL;

    if (__Importer_Path__ == NULL || __Import__.__Data__ == NULL || __Import__.__Length__ == 0U ||
        __Import__.__Data__[0] == '/')
    {
        return NULL;
    }

    if (__Logical__)
    {
        __Candidate__ = __Resolve_Logical_Import__(__Importer_Path__, __Project_Root__, __Import__);
    }
    else
    {
        __Directory__ = __Source_Path_Directory__(__Importer_Path__);
        if (__Directory__ == NULL)
        {
            return NULL;
        }
        __Candidate__ = __Path_Join_Text__(__Directory__, __Import__);
        free(__Directory__);
    }

    if (__Candidate__ == NULL)
    {
        return NULL;
    }
    if (!__Path_Exists__(__Candidate__))
    {
        free(__Candidate__);
        return NULL;
    }
    return __Candidate__;
}
