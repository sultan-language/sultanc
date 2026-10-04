/* Resolves logical source import paths. */

#include "support/path/filesystem.h"
#include "support/path/source_path.h"
#include "support/path/source_import_path.h"

#include <stdint.h>
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
    if (__Root_Length__ == 1U && __Root__[0] == '/')
    {
        return __Path__[0] == '/';
    }
    return strncmp(__Path__, __Root__, __Root_Length__) == 0 &&
           (__Path__[__Root_Length__] == '/' || __Path__[__Root_Length__] == '\0');
}

/* Counts path components below a configured root. */
static size_t __Path_Depth_Below_Root__(const char *__Path__, const char *__Root__)
{
    /* References the relative path. */
    const char *__Relative__;
    /* Stores the depth. */
    size_t __Depth__ = 0U;

    if (!__Path_Is_Under__(__Path__, __Root__))
    {
        return SIZE_MAX;
    }
    __Relative__ = __Path__ + strlen(__Root__);
    if (*__Relative__ == '/')
    {
        ++__Relative__;
    }
    while (*__Relative__ != '\0')
    {
        while (*__Relative__ == '/')
        {
            ++__Relative__;
        }
        if (*__Relative__ == '\0')
        {
            break;
        }
        ++__Depth__;
        while (*__Relative__ != '\0' && *__Relative__ != '/')
        {
            ++__Relative__;
        }
    }
    return __Depth__;
}

/* Checks whether a relative spelling escapes its configured logical root. */
static int __Relative_Import_Escapes_Root__(const char *__Importer_Path__,
                                            const char *__Root__,
                                            __Text_Slice__ __Import__)
{
    /* References the importer directory. */
    char *__Directory__;
    /* Stores the remaining depth. */
    size_t __Depth__;
    /* Tracks the import cursor. */
    size_t __Index__ = 0U;

    __Directory__ = __Source_Path_Directory__(__Importer_Path__);
    if (__Directory__ == NULL)
    {
        return 1;
    }
    __Depth__ = __Path_Depth_Below_Root__(__Directory__, __Root__);
    free(__Directory__);
    if (__Depth__ == SIZE_MAX)
    {
        return 1;
    }

    while (__Index__ < __Import__.__Length__)
    {
        /* Stores the component start. */
        size_t __Start__;
        /* Stores the component length. */
        size_t __Length__;

        while (__Index__ < __Import__.__Length__ && __Import__.__Data__[__Index__] == '/')
        {
            ++__Index__;
        }
        __Start__ = __Index__;
        while (__Index__ < __Import__.__Length__ && __Import__.__Data__[__Index__] != '/')
        {
            ++__Index__;
        }
        __Length__ = __Index__ - __Start__;
        if (__Length__ == 0U ||
            (__Length__ == 1U && __Import__.__Data__[__Start__] == '.'))
        {
            continue;
        }
        if (__Length__ == 2U && __Import__.__Data__[__Start__] == '.' &&
            __Import__.__Data__[__Start__ + 1U] == '.')
        {
            if (__Depth__ == 0U)
            {
                return 1;
            }
            --__Depth__;
            continue;
        }
        ++__Depth__;
    }
    return 0;
}

/* Returns the configured root that owns a relative import. */
static char *__Relative_Import_Root__(const char *__Importer_Path__, const char *__Project_Root__)
{
    /* Stores the std root. */
    static const char __Std_Root__[] = "library/std";
    /* Stores the std root text. */
    __Text_Slice__ __Std_Root_Text__ = {__Std_Root__, sizeof(__Std_Root__) - 1U};
    /* References the std root path. */
    char *__Std_Base__;

    __Std_Base__ = __Path_Join_Text__(__Project_Root__, __Std_Root_Text__);
    if (__Std_Base__ != NULL && __Path_Is_Under__(__Importer_Path__, __Std_Base__))
    {
        return __Std_Base__;
    }
    free(__Std_Base__);
    return __Source_Path_Canonical__(__Project_Root__);
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
                                     __Text_Slice__ __Import__,
                                     int *__Escapes_Root__)
{
    /* References the importer directory. */
    char *__Directory__ = NULL;
    /* References the candidate path. */
    char *__Candidate__ = NULL;
    /* References the configured relative-import root. */
    char *__Relative_Root__ = NULL;

    if (__Escapes_Root__ != NULL)
    {
        *__Escapes_Root__ = 0;
    }
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
        __Relative_Root__ = __Relative_Import_Root__(__Importer_Path__, __Project_Root__);
        if (__Relative_Root__ == NULL)
        {
            return NULL;
        }
        if (__Relative_Import_Escapes_Root__(__Importer_Path__, __Relative_Root__, __Import__))
        {
            free(__Relative_Root__);
            if (__Escapes_Root__ != NULL)
            {
                *__Escapes_Root__ = 1;
            }
            return NULL;
        }
        __Directory__ = __Source_Path_Directory__(__Importer_Path__);
        if (__Directory__ == NULL)
        {
            free(__Relative_Root__);
            return NULL;
        }
        __Candidate__ = __Path_Join_Text__(__Directory__, __Import__);
        free(__Directory__);
    }

    if (__Candidate__ == NULL)
    {
        free(__Relative_Root__);
        return NULL;
    }
    if (!__Path_Exists__(__Candidate__))
    {
        free(__Relative_Root__);
        free(__Candidate__);
        return NULL;
    }
    if (!__Logical__ && __Relative_Root__ != NULL)
    {
        /* References the canonical candidate. */
        char *__Canonical_Candidate__ = __Source_Path_Canonical__(__Candidate__);
        if (__Canonical_Candidate__ == NULL ||
            !__Path_Is_Under__(__Canonical_Candidate__, __Relative_Root__))
        {
            free(__Canonical_Candidate__);
            free(__Relative_Root__);
            free(__Candidate__);
            if (__Escapes_Root__ != NULL)
            {
                *__Escapes_Root__ = 1;
            }
            return NULL;
        }
        free(__Canonical_Candidate__);
    }
    free(__Relative_Root__);
    return __Candidate__;
}
