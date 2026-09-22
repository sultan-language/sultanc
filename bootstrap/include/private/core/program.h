#ifndef SULTANC__CORE_PROGRAM_H__
#define SULTANC__CORE_PROGRAM_H__

#include "core/diagnostic.h"
#include "frontend/parser/parser.h"
#include "support/containers/vector.h"

#include <stddef.h>

typedef struct
{
    char *__Path__;
    __Source_File__ *__Source__;
    __Parse_Result__ __Parse__;

    /* Canonical direct import edges. Entries are indexes into Program.__Units__. */
    __Vector__ __Imported_Unit_Indexes__;

    /* Loader-only cycle state: true while this canonical unit is being expanded. */
    int __Loading__;
} __Program_Unit__;

typedef struct
{
    __Vector__ __Units__;
    /* __Program_Unit__ */
    __Diagnostic__ __Diagnostic__;
    int __Failed__;
} __Program__;

/* Lifecycle: Init -> Load_Root -> Destroy. */
void __Program_Init__(__Program__ *__Program_State__);

void __Program_Destroy__(__Program__ *__Program_State__);

int __Program_Load_Root__(__Program__ *__Program_State__, const char *__Root_Path__);

size_t __Program_Unit_Count__(const __Program__ *__Program_State__);

__Program_Unit__ *__Program_Unit_At__(__Program__ *__Program_State__, size_t __Index__);

const __Program_Unit__ *__Program_Unit_At_Const__(const __Program__ *__Program_State__,
                                                  size_t __Index__);

const __Diagnostic__ *__Program_Diagnostic__(const __Program__ *__Program_State__);

#endif
