/* Initializes and destroys bootstrap program state. */

#include "core/program.h"
#include "core/source.h"
#include "frontend/parser/parser.h"
#include "support/containers/vector.h"

#include <stdlib.h>
#include <string.h>

/* Initializes the program. */
void __Program_Init__(__Program__ *__Program_State__)
{
    memset(__Program_State__, 0, sizeof(*__Program_State__));
    __Vector_Init__(&__Program_State__->__Units__, sizeof(__Program_Unit__));
}

/* Releases the program. */
void __Program_Destroy__(__Program__ *__Program_State__)
{
    /* Tracks the index. */
    size_t __Index__ = 0U;
    if (__Program_State__ == NULL)
    {
        return;
    }
    for (__Index__ = 0U; __Index__ < __Program_State__->__Units__.__Count__; ++__Index__)
    {
        /* References the unit. */
        __Program_Unit__ *__Unit__ =
            (__Program_Unit__ *)__Vector_At__(&__Program_State__->__Units__, __Index__);
        if (__Unit__ == NULL)
        {
            continue;
        }
        __Vector_Destroy__(&__Unit__->__Imported_Unit_Indexes__);
        __Parse_Result_Destroy__(&__Unit__->__Parse__);
        if (__Unit__->__Source__ != NULL)
        {
            __Source_Destroy__(__Unit__->__Source__);
            free(__Unit__->__Source__);
            __Unit__->__Source__ = NULL;
        }
        free(__Unit__->__Path__);
    }
    __Vector_Destroy__(&__Program_State__->__Units__);
    free(__Program_State__->__Project_Root__);
    __Program_State__->__Project_Root__ = NULL;
    memset(__Program_State__, 0, sizeof(*__Program_State__));
}
