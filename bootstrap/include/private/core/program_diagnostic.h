#ifndef SULTANC_PRIVATE_CORE_PROGRAM_DIAGNOSTIC_H
#define SULTANC_PRIVATE_CORE_PROGRAM_DIAGNOSTIC_H

#include "program.h"

/* Starts the single structured program-loading diagnostic. */
__Diagnostic__ *__Program_Begin_Diagnostic__(__Program__ *__Program_State__, __Error_Id__ __Id__);

void __Program_Set_Diagnostic__(__Program__ *__Program_State__, __Error_Id__ __Id__);

void __Program_Adopt_Diagnostic__(__Program__ *__Program_State__,
                                  const __Diagnostic__ *__Diagnostic_State__);

#endif
