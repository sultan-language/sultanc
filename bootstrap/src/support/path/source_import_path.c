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

/* The standard library's libraries, nearest first: library/std sees
 * library/alloc and library/core at the same paths, as the self-hosted
 * compiler's layered roots do (compiler/driver/std_location.sn). A library
 * that is not there is skipped. */
static const char *const __Std_Libraries__[] = {"std", "alloc", "core"};
#define __STD_LIBRARY_COUNT__ 3

/* library/NAME under the project root, malloc'd. */
static char *__Std_Library_Base__(const char *__Project_Root__, int __Index__)
{
    char __Relative__[32];
    __Text_Slice__ __Relative_Text__;
    snprintf(__Relative__, sizeof(__Relative__), "library/%s", __Std_Libraries__[__Index__]);
    __Relative_Text__.__Data__ = __Relative__;
    __Relative_Text__.__Length__ = strlen(__Relative__);
    return __Path_Join_Text__(__Project_Root__, __Relative_Text__);
}

/* The library whose folder holds PATH, or NULL; malloc'd. */
static char *__Std_Library_Of__(const char *__Path__, const char *__Project_Root__)
{
    int __Index__;
    for (__Index__ = 0; __Index__ < __STD_LIBRARY_COUNT__; ++__Index__)
    {
        char *__Base__ = __Std_Library_Base__(__Project_Root__, __Index__);
        if (__Base__ != NULL && __Path_Is_Under__(__Path__, __Base__))
            return __Base__;
        free(__Base__);
    }
    return NULL;
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

    (void)__Std_Root_Text__;
    __Std_Base__ = __Std_Library_Of__(__Importer_Path__, __Project_Root__);
    if (__Std_Base__ != NULL)
    {
        return __Std_Base__;
    }
    return __Source_Path_Canonical__(__Project_Root__);
}
/* One `module NAME { ... };`, `module NAME = LIBRARY.a.b;` or `module NAME =
 * LIBRARY.a.b { ... };` line of a root file (`وحدة` for `module`), parsed in
 * place: the child's name, the re-export target (NULL for none) and the `ar:`
 * spelling (NULL for none). Returns 0 when the line is no such declaration. */
static int __Parse_Module_Line__(char *__Line__, char **__Name__, char **__Target__, char **__Arabic__)
{
    static const char __Module_En__[] = "module ";
    static const char __Module_Ar__[] = "\xd9\x88\xd8\xad\xd8\xaf\xd8\xa9 "; /* وحدة */
    char *__Cursor__ = __Line__;
    char *__End__;
    char *__Brace__;
    *__Name__ = NULL;
    *__Target__ = NULL;
    *__Arabic__ = NULL;
    while (*__Cursor__ == ' ' || *__Cursor__ == '\t')
        ++__Cursor__;
    if (strncmp(__Cursor__, __Module_En__, sizeof(__Module_En__) - 1U) == 0)
        __Cursor__ += sizeof(__Module_En__) - 1U;
    else if (strncmp(__Cursor__, __Module_Ar__, sizeof(__Module_Ar__) - 1U) == 0)
        __Cursor__ += sizeof(__Module_Ar__) - 1U;
    else
        return 0;
    while (*__Cursor__ == ' ')
        ++__Cursor__;
    __End__ = __Cursor__;
    while (*__End__ != '\0' && *__End__ != ' ' && *__End__ != '\t' && *__End__ != '=' &&
           *__End__ != '{' && *__End__ != ';')
        ++__End__;
    if (__End__ == __Cursor__)
        return 0;
    *__Name__ = __Cursor__;
    __Cursor__ = __End__;
    while (*__Cursor__ == ' ' || *__Cursor__ == '\t')
        ++__Cursor__;
    if (*__Cursor__ == '=')
    {
        *__End__ = '\0';
        ++__Cursor__;
        while (*__Cursor__ == ' ' || *__Cursor__ == '\t')
            ++__Cursor__;
        __End__ = __Cursor__;
        while (*__End__ != '\0' && *__End__ != ' ' && *__End__ != '\t' && *__End__ != '{' &&
               *__End__ != ';')
            ++__End__;
        if (__End__ == __Cursor__)
            return 0;
        *__Target__ = __Cursor__;
        __Cursor__ = __End__;
    }
    __Brace__ = strchr(__Cursor__, '{');
    *__End__ = '\0';
    if (__Brace__ != NULL)
    {
        char *__Ar__ = __Brace__;
        for (;;)
        {
            __Ar__ = strstr(__Ar__ + 1, "ar:");
            if (__Ar__ == NULL)
                break;
            if (__Ar__[-1] == '{' || __Ar__[-1] == ',' || __Ar__[-1] == ' ' || __Ar__[-1] == '\t')
                break;
        }
        if (__Ar__ != NULL)
        {
            __Ar__ += 3;
            while (*__Ar__ == ' ')
                ++__Ar__;
            __End__ = __Ar__;
            while (*__End__ != '\0' && *__End__ != ',' && *__End__ != '}' && *__End__ != ' ' &&
                   *__End__ != '\t')
                ++__End__;
            if (__End__ != __Ar__)
            {
                *__End__ = '\0';
                *__Arabic__ = __Ar__;
            }
        }
    }
    return 1;
}

/* The declaration of a folder's root file for the child WORD (by its name or
 * its Arabic spelling): its name into NAME_OUT and its re-export target, or
 * "", into TARGET_OUT. Returns 0 when the root file declares no such child. */
static int __Std_Folder_Lookup__(const char *__Folder__, const char *__Root_File__,
                                 const char *__Word__, size_t __Word_Length__,
                                 char *__Name_Out__, size_t __Name_Capacity__,
                                 char *__Target_Out__, size_t __Target_Capacity__)
{
    __Text_Slice__ __Root_Text__ = {__Root_File__, strlen(__Root_File__)};
    char *__Root_Path__ = __Path_Join_Text__(__Folder__, __Root_Text__);
    FILE *__File__ = NULL;
    char __Line__[512];
    int __Found__ = 0;
    if (__Root_Path__ == NULL)
        return 0;
    __File__ = fopen(__Root_Path__, "rb");
    free(__Root_Path__);
    while (__File__ != NULL && !__Found__ && fgets(__Line__, (int)sizeof(__Line__), __File__) != NULL)
    {
        char *__Name__;
        char *__Target__;
        char *__Arabic__;
        int __By_Name__;
        int __By_Spelling__;
        if (!__Parse_Module_Line__(__Line__, &__Name__, &__Target__, &__Arabic__))
            continue;
        __By_Name__ = strlen(__Name__) == __Word_Length__ && memcmp(__Name__, __Word__, __Word_Length__) == 0;
        __By_Spelling__ = __Arabic__ != NULL && strlen(__Arabic__) == __Word_Length__ &&
                          memcmp(__Arabic__, __Word__, __Word_Length__) == 0;
        if (!__By_Name__ && !__By_Spelling__)
            continue;
        if (strlen(__Name__) < __Name_Capacity__ &&
            (__Target__ == NULL || strlen(__Target__) < __Target_Capacity__))
        {
            strcpy(__Name_Out__, __Name__);
            strcpy(__Target_Out__, __Target__ != NULL ? __Target__ : "");
            __Found__ = 1;
        }
    }
    if (__File__ != NULL)
        fclose(__File__);
    return __Found__;
}

/* The index of the library named NAME (std, alloc, core), or -1. */
static int __Std_Library_Index__(const char *__Name__, size_t __Length__)
{
    int __Index__;
    for (__Index__ = 0; __Index__ < __STD_LIBRARY_COUNT__; ++__Index__)
    {
        if (strlen(__Std_Libraries__[__Index__]) == __Length__ &&
            memcmp(__Std_Libraries__[__Index__], __Name__, __Length__) == 0)
            return __Index__;
    }
    return -1;
}

/* The index of the library whose folder holds PATH, or -1. */
static int __Std_Library_Index_Of__(const char *__Path__, const char *__Project_Root__)
{
    int __Index__;
    for (__Index__ = 0; __Index__ < __STD_LIBRARY_COUNT__; ++__Index__)
    {
        char *__Base__ = __Std_Library_Base__(__Project_Root__, __Index__);
        int __Under__ = __Base__ != NULL && __Path_Is_Under__(__Path__, __Base__);
        free(__Base__);
        if (__Under__)
            return __Index__;
    }
    return -1;
}

#define __STD_MAX_COMPONENTS__ 64
#define __STD_MAX_REEXPORTS__ 32

/* The file RELATIVE (a/b/c.sn, its components canonical names or Arabic
 * spellings) of the standard library LIBRARY, walked folder by folder as
 * compiler/frontend/module_loading/module_spellings.sn walks it: a component
 * is replaced by the child its spelling names in the folder's root file, and
 * a child the root file re-exports (`module compare = core.text.compare;`)
 * continues in the re-exported library at the re-exported path. Only the
 * root files of the library being walked are read: nothing reaches another
 * library unless a root file re-exports it. Returns a malloc'd path, or NULL
 * on a re-export that names no library or does not end. */
static char *__Std_Resolve_Relative__(const char *__Project_Root__, int __Library__,
                                      __Text_Slice__ __Relative__)
{
    char *__Parts__[__STD_MAX_COMPONENTS__];
    size_t __Count__ = 0U;
    size_t __Start__ = 0U;
    size_t __Index__;
    int __Has_Suffix__ = 0;
    int __Hops__ = 0;
    int __Failed__ = 0;
    char *__Folder__ = NULL;
    char *__Result__ = NULL;
    size_t __Length__;
    while (__Start__ <= __Relative__.__Length__ && __Count__ < __STD_MAX_COMPONENTS__)
    {
        size_t __End__ = __Start__;
        while (__End__ < __Relative__.__Length__ && __Relative__.__Data__[__End__] != '/')
            ++__End__;
        __Parts__[__Count__] = (char *)malloc(__End__ - __Start__ + 1U);
        if (__Parts__[__Count__] == NULL)
        {
            __Failed__ = 1;
            break;
        }
        memcpy(__Parts__[__Count__], __Relative__.__Data__ + __Start__, __End__ - __Start__);
        __Parts__[__Count__][__End__ - __Start__] = '\0';
        ++__Count__;
        if (__End__ >= __Relative__.__Length__)
            break;
        __Start__ = __End__ + 1U;
    }
    if (!__Failed__ && __Count__ > 0U)
    {
        char *__Last__ = __Parts__[__Count__ - 1U];
        size_t __Last_Length__ = strlen(__Last__);
        if (__Last_Length__ > 3U && strcmp(__Last__ + __Last_Length__ - 3U, ".sn") == 0)
        {
            __Has_Suffix__ = 1;
            __Last__[__Last_Length__ - 3U] = '\0';
        }
    }
    __Index__ = 0U;
    while (!__Failed__ && __Index__ < __Count__)
    {
        char __Root_File__[160];
        char __Name__[128];
        char __Target__[256];
        if (__Folder__ == NULL)
        {
            __Folder__ = __Std_Library_Base__(__Project_Root__, __Library__);
            if (__Folder__ == NULL)
            {
                __Failed__ = 1;
                break;
            }
            snprintf(__Root_File__, sizeof(__Root_File__), "%s.sn", __Std_Libraries__[__Library__]);
        }
        else
        {
            const char *__Slash__ = strrchr(__Folder__, '/');
            snprintf(__Root_File__, sizeof(__Root_File__), "%s.sn", __Slash__ != NULL ? __Slash__ + 1 : __Folder__);
        }
        if (__Std_Folder_Lookup__(__Folder__, __Root_File__, __Parts__[__Index__], strlen(__Parts__[__Index__]),
                                  __Name__, sizeof(__Name__), __Target__, sizeof(__Target__)))
        {
            if (__Target__[0] != '\0')
            {
                /* LIBRARY.a.b, then the components after this one. */
                char *__New_Parts__[__STD_MAX_COMPONENTS__];
                size_t __New_Count__ = 0U;
                char *__Cursor__ = __Target__;
                char *__Dot__ = strchr(__Cursor__, '.');
                size_t __Rest__;
                int __Target_Library__;
                if (++__Hops__ > __STD_MAX_REEXPORTS__ || __Dot__ == NULL)
                {
                    __Failed__ = 1;
                    break;
                }
                __Target_Library__ = __Std_Library_Index__(__Cursor__, (size_t)(__Dot__ - __Cursor__));
                /* A library sees itself and the libraries beneath it only. */
                if (__Target_Library__ < __Library__)
                {
                    __Failed__ = 1;
                    break;
                }
                __Cursor__ = __Dot__ + 1;
                while (__New_Count__ < __STD_MAX_COMPONENTS__)
                {
                    size_t __Part_Length__;
                    __Dot__ = strchr(__Cursor__, '.');
                    __Part_Length__ = __Dot__ != NULL ? (size_t)(__Dot__ - __Cursor__) : strlen(__Cursor__);
                    __New_Parts__[__New_Count__] = (char *)malloc(__Part_Length__ + 1U);
                    if (__New_Parts__[__New_Count__] == NULL)
                        break;
                    memcpy(__New_Parts__[__New_Count__], __Cursor__, __Part_Length__);
                    __New_Parts__[__New_Count__][__Part_Length__] = '\0';
                    ++__New_Count__;
                    if (__Dot__ == NULL)
                        break;
                    __Cursor__ = __Dot__ + 1;
                }
                for (__Rest__ = __Index__ + 1U; __Rest__ < __Count__ && __New_Count__ < __STD_MAX_COMPONENTS__; ++__Rest__)
                {
                    __New_Parts__[__New_Count__++] = __Parts__[__Rest__];
                    __Parts__[__Rest__] = NULL;
                }
                for (__Rest__ = 0U; __Rest__ < __Count__; ++__Rest__)
                    free(__Parts__[__Rest__]);
                memcpy(__Parts__, __New_Parts__, __New_Count__ * sizeof(*__Parts__));
                __Count__ = __New_Count__;
                __Library__ = __Target_Library__;
                free(__Folder__);
                __Folder__ = NULL;
                __Index__ = 0U;
                continue;
            }
            free(__Parts__[__Index__]);
            __Parts__[__Index__] = (char *)malloc(strlen(__Name__) + 1U);
            if (__Parts__[__Index__] == NULL)
            {
                __Failed__ = 1;
                break;
            }
            strcpy(__Parts__[__Index__], __Name__);
        }
        {
            __Text_Slice__ __Part_Text__ = {__Parts__[__Index__], strlen(__Parts__[__Index__])};
            char *__Next__ = __Path_Join_Text__(__Folder__, __Part_Text__);
            free(__Folder__);
            __Folder__ = __Next__;
            if (__Folder__ == NULL)
                __Failed__ = 1;
        }
        ++__Index__;
    }
    free(__Folder__);
    if (!__Failed__)
    {
        char *__Base__ = __Std_Library_Base__(__Project_Root__, __Library__);
        __Length__ = 0U;
        for (__Index__ = 0U; __Index__ < __Count__; ++__Index__)
            __Length__ += strlen(__Parts__[__Index__]) + 1U;
        __Result__ = __Base__ != NULL ? (char *)malloc(strlen(__Base__) + __Length__ + 8U) : NULL;
        if (__Result__ != NULL)
        {
            strcpy(__Result__, __Base__);
            for (__Index__ = 0U; __Index__ < __Count__; ++__Index__)
            {
                strcat(__Result__, "/");
                strcat(__Result__, __Parts__[__Index__]);
            }
            if (__Has_Suffix__)
                strcat(__Result__, ".sn");
        }
        free(__Base__);
    }
    for (__Index__ = 0U; __Index__ < __Count__; ++__Index__)
        free(__Parts__[__Index__]);
    return __Result__;
}

/* Resolves the logical import. */
static char *__Resolve_Logical_Import__(const char *__Importer_Path__,
                                        const char *__Project_Root__,
                                        __Text_Slice__ __Import__)
{
    static const char __Std_Ar__[] = "\xd8\xa7\xd9\x84\xd9\x85\xd9\x83\xd8\xaa\xd8\xa8\xd8\xa9"; /* المكتبة */
    size_t __First_End__ = 0U;
    int __Library__;

    if (__Project_Root__ == NULL)
    {
        return NULL;
    }
    /* `std/...` or `المكتبة/...`: a path of std (canonical `std`, Arabic
     * `المكتبة`, compiler/driver/std_location.sn), its components canonical
     * names or the Arabic spellings std's root files declare. */
    while (__First_End__ < __Import__.__Length__ && __Import__.__Data__[__First_End__] != '/')
        ++__First_End__;
    if (__First_End__ < __Import__.__Length__ &&
        ((__First_End__ == 3U && memcmp(__Import__.__Data__, "std", 3U) == 0) ||
         (__First_End__ == sizeof(__Std_Ar__) - 1U && memcmp(__Import__.__Data__, __Std_Ar__, __First_End__) == 0)))
    {
        __Text_Slice__ __Rest__;
        __Rest__.__Data__ = __Import__.__Data__ + __First_End__ + 1U;
        __Rest__.__Length__ = __Import__.__Length__ - __First_End__ - 1U;
        return __Std_Resolve_Relative__(__Project_Root__, 0, __Rest__);
    }
    /* `alloc/...` (`التخصيص/...`) or `core/...` (`الأساس/...`): a path of
     * that library, as std's above (an alloc module importing a core module
     * of the same path, `الأساس.ترميز.ثنائي`). */
    if (__First_End__ < __Import__.__Length__)
    {
        static const char __Alloc_Ar__[] =
            "\xd8\xa7\xd9\x84\xd8\xaa\xd8\xae\xd8\xb5\xd9\x8a\xd8\xb5"; /* التخصيص */
        static const char __Core_Ar__[] = "\xd8\xa7\xd9\x84\xd8\xa3\xd8\xb3\xd8\xa7\xd8\xb3"; /* الأساس */
        int __Named__ = -1;

        if ((__First_End__ == 5U && memcmp(__Import__.__Data__, "alloc", 5U) == 0) ||
            (__First_End__ == sizeof(__Alloc_Ar__) - 1U &&
             memcmp(__Import__.__Data__, __Alloc_Ar__, __First_End__) == 0))
            __Named__ = 1;
        else if ((__First_End__ == 4U && memcmp(__Import__.__Data__, "core", 4U) == 0) ||
                 (__First_End__ == sizeof(__Core_Ar__) - 1U &&
                  memcmp(__Import__.__Data__, __Core_Ar__, __First_End__) == 0))
            __Named__ = 2;
        if (__Named__ >= 0)
        {
            __Text_Slice__ __Rest__;
            __Rest__.__Data__ = __Import__.__Data__ + __First_End__ + 1U;
            __Rest__.__Length__ = __Import__.__Length__ - __First_End__ - 1U;
            return __Std_Resolve_Relative__(__Project_Root__, __Named__, __Rest__);
        }
    }
    /* Root-local logical imports inside the standard library (std, alloc,
     * core) are paths of the importer's own library (`نصوص.أعداد` is
     * `text.number`), followed through its root files' re-exports. */
    __Library__ = __Std_Library_Index_Of__(__Importer_Path__, __Project_Root__);
    if (__Library__ >= 0)
        return __Std_Resolve_Relative__(__Project_Root__, __Library__, __Import__);
    return __Path_Join_Text__(__Project_Root__, __Import__);
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
