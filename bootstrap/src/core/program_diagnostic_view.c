#include "core/program.h"

const __Diagnostic__ *__Program_Diagnostic__(const __Program__ *__Program_State__)
{
    return (__Program_State__ == NULL || !__Program_State__->__Failed__)
               ? NULL
               : &__Program_State__->__Diagnostic__;
}
