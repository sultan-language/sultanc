#include "core/program.h"
#include "support/containers/vector.h"

const __Program_Unit__ *__Program_Unit_At_Const__(const __Program__ *__Program_State__,
                                                  size_t __Index__)
{
    return __Program_State__ == NULL ? NULL
                                     : (const __Program_Unit__ *)__Vector_At_Const__(
                                           &__Program_State__->__Units__, __Index__);
}
