/* Sets the current program diagnostic. */

#include "core/program_diagnostic.h"

/* Begins the program diagnostic. */
__Diagnostic__ *__Program_Begin_Diagnostic__(__Program__ *__Program_State__, __Error_Id__ __Id__)
{
    if (__Program_State__ == NULL)
    {
        return NULL;
    }

    __Diagnostic_Begin_Without_Source__(&__Program_State__->__Diagnostic__, __Id__);
    __Program_State__->__Failed__ = 1;
    return &__Program_State__->__Diagnostic__;
}

/* Sets the program diagnostic. */
void __Program_Set_Diagnostic__(__Program__ *__Program_State__, __Error_Id__ __Id__)
{
    (void)__Program_Begin_Diagnostic__(__Program_State__, __Id__);
}
