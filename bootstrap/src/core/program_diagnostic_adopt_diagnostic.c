#include "core/program_diagnostic.h"

void __Program_Adopt_Diagnostic__(__Program__ *__Program_State__,
                                  const __Diagnostic__ *__Diagnostic_State__)
{
    if (__Program_State__ == NULL || __Diagnostic_State__ == NULL)
    {
        return;
    }

    __Diagnostic_Copy__(&__Program_State__->__Diagnostic__, __Diagnostic_State__);
    __Program_State__->__Failed__ = 1;
}
