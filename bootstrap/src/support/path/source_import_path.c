/* Resolves logical source import paths. */

#include "support/path/filesystem.h"
#include "support/path/source_path.h"
#include "support/path/source_import_path.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Explicit module sources (--module-source): the module as a path without
 * extension (dots become slashes) and the file it binds. */
typedef struct
{
    /* Owns the module path. */
    char *__Module__;
    /* Owns the bound file path. */
    char *__File__;
} __Module_Source_Binding__;

/* Stores the bindings. */
static __Module_Source_Binding__ *__Module_Source_Bindings__ = NULL;
/* Stores the binding count. */
static size_t __Module_Source_Binding_Count__ = 0U;

/* Copies a module spelling as a path: dots become slashes, a trailing .sn
 * is dropped. */
static char *__Module_Source_Key__(const char *__Data__, size_t __Length__)
{
    /* References the key. */
    char *__Key__;
    /* Tracks the index. */
    size_t __Index__;

    if (__Length__ >= 3U && memcmp(__Data__ + __Length__ - 3U, ".sn", 3U) == 0)
    {
        __Length__ -= 3U;
    }
    __Key__ = (char *)malloc(__Length__ + 1U);
    if (__Key__ == NULL)
    {
        return NULL;
    }
    for (__Index__ = 0U; __Index__ < __Length__; ++__Index__)
    {
        __Key__[__Index__] = __Data__[__Index__] == '.' ? '/' : __Data__[__Index__];
    }
    __Key__[__Length__] = '\0';
    return __Key__;
}

/* Binds a logical module to an explicit source file. */
int __Source_Path_Bind_Module_Source__(const char *__Binding__)
{
    /* References the separator. */
    const char *__Equals__ = strchr(__Binding__, '=');
    /* References the grown table. */
    __Module_Source_Binding__ *__Grown__;
    /* References the module key. */
    char *__Module__;
    /* References the file copy. */
    char *__File__;

    if (__Equals__ == NULL || __Equals__ == __Binding__ || __Equals__[1] == '\0')
    {
        return 0;
    }
    __Module__ = __Module_Source_Key__(__Binding__, (size_t)(__Equals__ - __Binding__));
    __File__ = (char *)malloc(strlen(__Equals__ + 1) + 1U);
    __Grown__ = (__Module_Source_Binding__ *)realloc(
        __Module_Source_Bindings__,
        (__Module_Source_Binding_Count__ + 1U) * sizeof(__Module_Source_Binding__));
    if (__Module__ == NULL || __File__ == NULL || __Grown__ == NULL)
    {
        free(__Module__);
        free(__File__);
        if (__Grown__ != NULL)
        {
            __Module_Source_Bindings__ = __Grown__;
        }
        return 0;
    }
    strcpy(__File__, __Equals__ + 1);
    __Module_Source_Bindings__ = __Grown__;
    __Module_Source_Bindings__[__Module_Source_Binding_Count__].__Module__ = __Module__;
    __Module_Source_Bindings__[__Module_Source_Binding_Count__].__File__ = __File__;
    ++__Module_Source_Binding_Count__;
    return 1;
}

/* The file bound to a logical import, or NULL. */
static char *__Module_Source_For_Import__(const char *__Project_Root__, __Text_Slice__ __Import__)
{
    /* References the import key. */
    char *__Key__;
    /* Tracks the index. */
    size_t __Index__;

    if (__Module_Source_Binding_Count__ == 0U)
    {
        return NULL;
    }
    __Key__ = __Module_Source_Key__(__Import__.__Data__, __Import__.__Length__);
    if (__Key__ == NULL)
    {
        return NULL;
    }
    for (__Index__ = 0U; __Index__ < __Module_Source_Binding_Count__; ++__Index__)
    {
        if (strcmp(__Key__, __Module_Source_Bindings__[__Index__].__Module__) == 0)
        {
            /* References the bound file. */
            const char *__File__ = __Module_Source_Bindings__[__Index__].__File__;
            /* Stores the file slice. */
            __Text_Slice__ __File_Text__ = {__File__, strlen(__File__)};
            /* References the copy of an absolute path. */
            char *__Copy__;
            free(__Key__);
            if (__File__[0] == '/' || __Project_Root__ == NULL)
            {
                __Copy__ = (char *)malloc(strlen(__File__) + 1U);
                if (__Copy__ != NULL)
                {
                    strcpy(__Copy__, __File__);
                }
                return __Copy__;
            }
            return __Path_Join_Text__(__Project_Root__, __File_Text__);
        }
    }
    free(__Key__);
    return NULL;
}

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
/* Reads the std package's localized names (canonical: library/Supax.toml,
 * [package.names] and [imports-alias]): the package's Arabic name and, for
 * each top-level std package, its Arabic spelling. Returns a malloc'd copy of
 * IMPORT with `<ar package>/<ar alias>` rewritten to `std/<en alias>`, or
 * NULL when IMPORT does not start with the package's Arabic name. */
static char *__Localized_Std_Import__(const char *__Project_Root__, __Text_Slice__ __Import__)
{
    static const char __Manifest_Name__[] = "library/Supax.toml";
    __Text_Slice__ __Manifest_Text__ = {__Manifest_Name__, sizeof(__Manifest_Name__) - 1U};
    char *__Manifest_Path__ = __Path_Join_Text__(__Project_Root__, __Manifest_Text__);
    FILE *__File__ = NULL;
    char __Line__[512];
    char __Package__[128] = "";
    char *__Result__ = NULL;
    size_t __First_End__ = 0U;
    size_t __Second_End__;
    int __In_Names__ = 0;
    int __In_Aliases__ = 0;
    if (__Manifest_Path__ == NULL)
        return NULL;
    __File__ = fopen(__Manifest_Path__, "rb");
    free(__Manifest_Path__);
    if (__File__ == NULL)
        return NULL;
    while (__First_End__ < __Import__.__Length__ && __Import__.__Data__[__First_End__] != '/')
        ++__First_End__;
    __Second_End__ = __First_End__ < __Import__.__Length__ ? __First_End__ + 1U : __First_End__;
    while (__Second_End__ < __Import__.__Length__ && __Import__.__Data__[__Second_End__] != '/')
        ++__Second_End__;
    while (fgets(__Line__, (int)sizeof(__Line__), __File__) != NULL)
    {
        char *__Ar__;
        char *__En__;
        char *__End__;
        if (__Line__[0] == '[')
        {
            __In_Names__ = strncmp(__Line__, "[package.names]", 15) == 0;
            __In_Aliases__ = strncmp(__Line__, "[imports-alias]", 15) == 0;
            continue;
        }
        __Ar__ = strstr(__Line__, "ar = \"");
        if (__Ar__ == NULL)
            continue;
        __Ar__ += 6;
        __End__ = strchr(__Ar__, '"');
        if (__End__ == NULL)
            continue;
        *__End__ = '\0';
        if (__In_Names__ && strncmp(__Line__, "ar", 2) == 0 && strlen(__Ar__) < sizeof(__Package__))
        {
            strcpy(__Package__, __Ar__);
            if (strlen(__Package__) != __First_End__ ||
                memcmp(__Package__, __Import__.__Data__, __First_End__) != 0)
                break;
            continue;
        }
        if (!__In_Aliases__ || __Package__[0] == '\0')
            continue;
        if (strlen(__Ar__) != __Second_End__ - __First_End__ - 1U ||
            memcmp(__Ar__, __Import__.__Data__ + __First_End__ + 1U, __Second_End__ - __First_End__ - 1U) != 0)
            continue;
        __En__ = strstr(__Line__, "en = \"");
        if (__En__ == NULL)
            continue;
        __En__ += 6;
        __End__ = strchr(__En__, '"');
        if (__End__ == NULL)
            continue;
        *__End__ = '\0';
        __Result__ = (char *)malloc(4U + strlen(__En__) + (__Import__.__Length__ - __Second_End__) + 1U);
        if (__Result__ != NULL)
        {
            size_t __Length__ = 0U;
            memcpy(__Result__, "std/", 4U);
            __Length__ = 4U;
            memcpy(__Result__ + __Length__, __En__, strlen(__En__));
            __Length__ += strlen(__En__);
            memcpy(__Result__ + __Length__, __Import__.__Data__ + __Second_End__,
                   __Import__.__Length__ - __Second_End__);
            __Length__ += __Import__.__Length__ - __Second_End__;
            __Result__[__Length__] = '\0';
        }
        break;
    }
    fclose(__File__);
    return __Result__;
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
        /* `المكتبة.أخطاء.failure`: the std package by its Arabic names. */
        char *__Localized__ = __Localized_Std_Import__(__Project_Root__, __Import__);
        if (__Localized__ != NULL)
        {
            __Text_Slice__ __Localized_Text__ = {__Localized__, strlen(__Localized__)};
            char *__Resolved__ = __Resolve_Logical_Import__(__Importer_Path__, __Project_Root__,
                                                            __Localized_Text__);
            free(__Localized__);
            return __Resolved__;
        }
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
        __Candidate__ = __Module_Source_For_Import__(__Project_Root__, __Import__);
        if (__Candidate__ != NULL)
        {
            return __Candidate__;
        }
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
