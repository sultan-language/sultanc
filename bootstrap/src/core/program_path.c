#include "core/program_loading.h"
#include "support/containers/vector.h"

#include <string.h>

int __Program_Has_Path__(const __Program__ *__Program_State__, const char *__Path__)
{
    size_t __Index__ = 0U;
    for (__Index__ = 0U; __Index__ < __Program_State__->__Units__.__Count__; ++__Index__)
    {
        const __Program_Unit__ *__Unit__ =
            (const __Program_Unit__ *)__Vector_At_Const__(&__Program_State__->__Units__, __Index__);
        if (__Unit__ != NULL && strcmp(__Unit__->__Path__, __Path__) == 0)
        {
            return 1;
        }
    }
    return 0;
}
