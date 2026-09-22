#include "support/path/source_path.h"
#include "core/program.h"
#include "core/program_loading.h"
#include "core/program_diagnostic.h"
#include "support/path/source_extension.h"
#include "support/path/source_import_path.h"

#include <stdlib.h>
#include <string.h>

static size_t __Program_Find_Path_Index__(const __Program__ *__Program_State__,
                                          const char *__Canonical_Path__)
{
    size_t __Index__;

    for (__Index__ = 0U; __Index__ < __Program_State__->__Units__.__Count__; ++__Index__)
    {
        const __Program_Unit__ *__Unit__ =
            (const __Program_Unit__ *)__Vector_At_Const__(&__Program_State__->__Units__, __Index__);
        if (__Unit__ != NULL && __Unit__->__Path__ != NULL &&
            strcmp(__Unit__->__Path__, __Canonical_Path__) == 0)
        {
            return __Index__;
        }
    }
    return SIZE_MAX;
}

static int __Program_Unit_Has_Import_Index__(const __Program_Unit__ *__Unit__,
                                             size_t __Imported_Index__)
{
    size_t __Index__;

    if (__Unit__ == NULL)
    {
        return 0;
    }
    for (__Index__ = 0U; __Index__ < __Unit__->__Imported_Unit_Indexes__.__Count__; ++__Index__)
    {
        const size_t *__Existing__ =
            (const size_t *)__Vector_At_Const__(&__Unit__->__Imported_Unit_Indexes__, __Index__);
        if (__Existing__ != NULL && *__Existing__ == __Imported_Index__)
        {
            return 1;
        }
    }
    return 0;
}

int __Program_Load_File__(__Program__ *__Program_State__, const char *__Path__)
{
    __Program_Unit__ __Unit__;
    char *__Canonical_Path__ = NULL;
    size_t __Unit_Index__;
    size_t __Import_Index__;

    __Canonical_Path__ = __Source_Path_Canonical__(__Path__);
    if (__Canonical_Path__ == NULL)
    {
        __Program_Set_Diagnostic__(__Program_State__, __E1100_Internal_Context_Error__);
        return 0;
    }
    if (__Program_Has_Path__(__Program_State__, __Canonical_Path__))
    {
        free(__Canonical_Path__);
        return 1;
    }

    memset(&__Unit__, 0, sizeof(__Unit__));
    __Unit__.__Path__ = __Canonical_Path__;
    __Unit__.__Loading__ = 1;
    __Vector_Init__(&__Unit__.__Imported_Unit_Indexes__, sizeof(size_t));
    __Unit__.__Source__ = (__Source_File__ *)malloc(sizeof(*__Unit__.__Source__));
    if (__Unit__.__Source__ == NULL)
    {
        __Vector_Destroy__(&__Unit__.__Imported_Unit_Indexes__);
        free(__Unit__.__Path__);
        __Program_Set_Diagnostic__(__Program_State__, __E1100_Internal_Context_Error__);
        return 0;
    }
    if (!__Source_Load__(__Unit__.__Path__, __Unit__.__Source__))
    {
        __Diagnostic__ *__Diagnostic_State__ =
            __Program_Begin_Diagnostic__(__Program_State__, __E0103_Input_File_Not_Found__);
        if (__Diagnostic_State__ != NULL)
        {
            __Diagnostic_Set_Message_Key__(__Diagnostic_State__,
                                           __Diag_Word_Input_File_Not_Found_Path__);
            __Diagnostic_Set_Argument_Cstr__(
                __Diagnostic_State__, __Diagnostic_Argument_Path__, __Path__);
        }
        __Vector_Destroy__(&__Unit__.__Imported_Unit_Indexes__);
        free(__Unit__.__Source__);
        free(__Unit__.__Path__);
        return 0;
    }
    if (!__Parser_Parse_Module__(__Unit__.__Source__, &__Unit__.__Parse__))
    {
        __Program_Adopt_Diagnostic__(__Program_State__, &__Unit__.__Parse__.__Diagnostic__);
        __Parse_Result_Destroy__(&__Unit__.__Parse__);
        __Source_Destroy__(__Unit__.__Source__);
        free(__Unit__.__Source__);
        __Vector_Destroy__(&__Unit__.__Imported_Unit_Indexes__);
        free(__Unit__.__Path__);
        return 0;
    }
    if (__Vector_Push__(&__Program_State__->__Units__, &__Unit__) == NULL)
    {
        __Parse_Result_Destroy__(&__Unit__.__Parse__);
        __Source_Destroy__(__Unit__.__Source__);
        free(__Unit__.__Source__);
        __Vector_Destroy__(&__Unit__.__Imported_Unit_Indexes__);
        free(__Unit__.__Path__);
        __Program_Set_Diagnostic__(__Program_State__, __E1100_Internal_Context_Error__);
        return 0;
    }
    __Unit_Index__ = __Program_State__->__Units__.__Count__ - 1U;

    for (__Import_Index__ = 0U;; ++__Import_Index__)
    {
        __Program_Unit__ *__Importer__ = __Program_Unit_At__(__Program_State__, __Unit_Index__);
        __Text_Slice__ __Import__;
        char *__Resolved__;
        char *__Resolved_Canonical__;
        size_t __Imported_Index__;

        if (__Importer__ == NULL ||
            __Import_Index__ >= __Importer__->__Parse__.__Module__.__Import_Count__)
        {
            break;
        }
        __Import__ = __Importer__->__Parse__.__Module__.__Imports__[__Import_Index__];
        __Resolved__ = __Source_Path_Resolve_Import__(__Importer__->__Path__, __Import__);
        if (__Resolved__ == NULL)
        {
            __Diagnostic__ *__Diagnostic_State__ =
                __Program_Begin_Diagnostic__(__Program_State__, __E0201_Source_Module_Not_Found__);
            if (__Diagnostic_State__ != NULL)
            {
                __Diagnostic_Set_Message_Key__(__Diagnostic_State__,
                                               __Diag_Word_Source_Module_Not_Found_Path__);
                __Diagnostic_Set_Argument_Text__(
                    __Diagnostic_State__, __Diagnostic_Argument_Path__, __Import__);
            }
            return 0;
        }
        if (!__Source_Path_Is_SultanC__(__Resolved__))
        {
            __Diagnostic__ *__Diagnostic_State__ = __Program_Begin_Diagnostic__(
                __Program_State__, __E0104_Unrecognized_Input_File_Type__);
            if (__Diagnostic_State__ != NULL)
            {
                __Diagnostic_Set_Message_Key__(__Diagnostic_State__,
                                               __Diag_Word_Unrecognized_Input_Type_Path__);
                __Diagnostic_Set_Argument_Cstr__(
                    __Diagnostic_State__, __Diagnostic_Argument_Path__, __Resolved__);
            }
            free(__Resolved__);
            return 0;
        }

        __Resolved_Canonical__ = __Source_Path_Canonical__(__Resolved__);
        if (__Resolved_Canonical__ == NULL)
        {
            free(__Resolved__);
            __Program_Set_Diagnostic__(__Program_State__, __E1100_Internal_Context_Error__);
            return 0;
        }
        __Imported_Index__ = __Program_Find_Path_Index__(__Program_State__, __Resolved_Canonical__);
        if (__Imported_Index__ != SIZE_MAX)
        {
            const __Program_Unit__ *__Imported_Unit__ =
                __Program_Unit_At_Const__(__Program_State__, __Imported_Index__);
            if (__Imported_Unit__ != NULL && __Imported_Unit__->__Loading__)
            {
                __Program_Set_Diagnostic__(__Program_State__, __E0311_Import_Cycle__);
                free(__Resolved_Canonical__);
                free(__Resolved__);
                return 0;
            }
        }
        else
        {
            if (!__Program_Load_File__(__Program_State__, __Resolved_Canonical__))
            {
                free(__Resolved_Canonical__);
                free(__Resolved__);
                return 0;
            }
            __Imported_Index__ =
                __Program_Find_Path_Index__(__Program_State__, __Resolved_Canonical__);
        }
        free(__Resolved_Canonical__);
        free(__Resolved__);
        if (__Imported_Index__ == SIZE_MAX)
        {
            __Program_Set_Diagnostic__(__Program_State__, __E1100_Internal_Context_Error__);
            return 0;
        }

        __Importer__ = __Program_Unit_At__(__Program_State__, __Unit_Index__);
        if (__Importer__ == NULL)
        {
            __Program_Set_Diagnostic__(__Program_State__, __E1100_Internal_Context_Error__);
            return 0;
        }
        if (!__Program_Unit_Has_Import_Index__(__Importer__, __Imported_Index__) &&
            __Vector_Push__(&__Importer__->__Imported_Unit_Indexes__, &__Imported_Index__) == NULL)
        {
            __Program_Set_Diagnostic__(__Program_State__, __E1100_Internal_Context_Error__);
            return 0;
        }
    }
    {
        __Program_Unit__ *__Loaded_Unit__ = __Program_Unit_At__(__Program_State__, __Unit_Index__);
        if (__Loaded_Unit__ == NULL)
        {
            __Program_Set_Diagnostic__(__Program_State__, __E1100_Internal_Context_Error__);
            return 0;
        }
        __Loaded_Unit__->__Loading__ = 0;
    }
    return 1;
}
