/* Maps diagnostic IDs to stable catalog metadata. */

#include "diagnostics/catalog/catalog.h"

#include <stddef.h>

/* Stores the diagnostic definitions. */
static const __Diagnostic_Definition__ __Diagnostic_Definitions__[__Error_Id_Count__] = {
/* Expands the sultanc diagnostic macro. */
#define SULTANC_DIAGNOSTIC(__Id__, __Code__, __Severity__, __Message_Key__)                        \
    [__Id__] = {__Id__, __Code__, #__Id__, __Severity__, __Message_Key__},
#include "core/diagnostic_catalog.def"
#undef SULTANC_DIAGNOSTIC
};

/* Returns the diagnostic definition for. */
const __Diagnostic_Definition__ *__Diagnostic_Definition_For__(__Error_Id__ __Id__)
{
    if ((size_t)__Id__ >= (size_t)__Error_Id_Count__)
    {
        return NULL;
    }
    return &__Diagnostic_Definitions__[__Id__];
}

/* Returns the diagnostic code. */
const char *__Diagnostic_Code__(__Error_Id__ __Id__)
{
    /* References the definition. */
    const __Diagnostic_Definition__ *__Definition__ = __Diagnostic_Definition_For__(__Id__);

    if (__Definition__ == NULL)
    {
        return "SULTANC-E????";
    }
    return __Definition__->__Code__;
}

/* Returns the diagnostic name. */
const char *__Diagnostic_Name__(__Error_Id__ __Id__)
{
    /* References the definition. */
    const __Diagnostic_Definition__ *__Definition__ = __Diagnostic_Definition_For__(__Id__);

    if (__Definition__ == NULL)
    {
        return "__Unknown_Diagnostic__";
    }
    return __Definition__->__Name__;
}

/* Returns the diagnostic severity of. */
__Diagnostic_Severity__ __Diagnostic_Severity_Of__(__Error_Id__ __Id__)
{
    /* References the definition. */
    const __Diagnostic_Definition__ *__Definition__ = __Diagnostic_Definition_For__(__Id__);

    if (__Definition__ == NULL)
    {
        return __Diagnostic_Severity_Error__;
    }
    return __Definition__->__Severity__;
}

/* Returns the diagnostic default message key. */
__Diagnostic_Wording_Key__ __Diagnostic_Default_Message_Key__(__Error_Id__ __Id__)
{
    /* References the definition. */
    const __Diagnostic_Definition__ *__Definition__ = __Diagnostic_Definition_For__(__Id__);

    if (__Definition__ == NULL)
    {
        return __Diag_Word_E1100__;
    }
    return __Definition__->__Default_Message_Key__;
}
