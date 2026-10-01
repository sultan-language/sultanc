/* Declares diagnostic catalog lookups. */

#ifndef SULTANC__DIAGNOSTICS_CATALOG_CATALOG_H__
#define SULTANC__DIAGNOSTICS_CATALOG_CATALOG_H__

#include "core/diagnostic.h"

/* Defines the diagnostic definition structure. */
typedef struct
{
    /* Stores the ID. */
    __Error_Id__ __Id__;
    /* References the code. */
    const char *__Code__;
    /* References the name. */
    const char *__Name__;
    /* Stores the severity. */
    __Diagnostic_Severity__ __Severity__;
    /* Stores the default message key. */
    __Diagnostic_Wording_Key__ __Default_Message_Key__;
} __Diagnostic_Definition__;

/* Returns the diagnostic definition for. */
const __Diagnostic_Definition__ *__Diagnostic_Definition_For__(__Error_Id__ __Id__);

#endif
