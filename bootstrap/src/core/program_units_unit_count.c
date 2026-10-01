/* Returns the number of loaded program units. */

#include "core/program.h"

/* Returns the program unit count. */
size_t __Program_Unit_Count__(const __Program__ *__Program_State__)
{
    return __Program_State__ == NULL ? 0U : __Program_State__->__Units__.__Count__;
}
