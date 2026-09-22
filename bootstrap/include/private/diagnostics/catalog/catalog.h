#ifndef SULTANC__DIAGNOSTICS_CATALOG_CATALOG_H__
#define SULTANC__DIAGNOSTICS_CATALOG_CATALOG_H__

#include "core/diagnostic.h"

typedef struct
{
    __Error_Id__ __Id__;
    const char *__Code__;
    const char *__Name__;
    __Diagnostic_Severity__ __Severity__;
    __Diagnostic_Wording_Key__ __Default_Message_Key__;
} __Diagnostic_Definition__;

const __Diagnostic_Definition__ *__Diagnostic_Definition_For__(__Error_Id__ __Id__);

#endif
