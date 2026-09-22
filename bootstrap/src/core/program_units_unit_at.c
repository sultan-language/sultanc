#include "core/program.h"
#include "support/containers/vector.h"

__Program_Unit__ *__Program_Unit_At__(__Program__ *__Program_State__, size_t __Index__)
{
    return __Program_State__ == NULL
               ? NULL
               : (__Program_Unit__ *)__Vector_At__(&__Program_State__->__Units__, __Index__);
}
