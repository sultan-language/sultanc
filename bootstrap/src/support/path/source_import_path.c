#include "support/path/filesystem.h"
#include "support/path/source_path.h"
#include "support/path/source_import_path.h"

#include <stdlib.h>

char *__Source_Path_Resolve_Import__(const char *__Importer_Path__, __Text_Slice__ __Import__)
{
    char *__Directory__;
    char *__Candidate__;

    if (__Importer_Path__ == NULL || __Import__.__Data__ == NULL || __Import__.__Length__ == 0U ||
        __Import__.__Data__[0] == '/')
    {
        return NULL;
    }

    __Directory__ = __Source_Path_Directory__(__Importer_Path__);
    if (__Directory__ == NULL)
    {
        return NULL;
    }
    __Candidate__ = __Path_Join_Text__(__Directory__, __Import__);
    free(__Directory__);
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
