/* Defines bootstrap program state and loaded units. */

#ifndef SULTANC__CORE_PROGRAM_H__
#define SULTANC__CORE_PROGRAM_H__

#include "core/diagnostic.h"
#include "frontend/parser/parser.h"
#include "support/containers/vector.h"

#include <stddef.h>

/* Defines the program unit structure. */
typedef struct
{
    /* References the path. */
    char *__Path__;
    /* References the source. */
    __Source_File__ *__Source__;
    /* Stores the parse. */
    __Parse_Result__ __Parse__;

    /* Canonical direct import edges. Entries are indexes into Program.__Units__. */
    __Vector__ __Imported_Unit_Indexes__;

    /* Loader-only cycle state: true while this canonical unit is being expanded. */
    int __Loading__;
} __Program_Unit__;

/* Defines the program structure. */
typedef struct
{
    /* Stores the units. */
    __Vector__ __Units__;
    /* Stores loaded program units. */
    char *__Project_Root__;
    /* Stores the diagnostic. */
    __Diagnostic__ __Diagnostic__;
    /* Tracks the failed state. */
    int __Failed__;
} __Program__;

/* Lifecycle: Init -> Load_Root -> Destroy. */
void __Program_Init__(__Program__ *__Program_State__);

/* Releases the program. */
void __Program_Destroy__(__Program__ *__Program_State__);

/* Loads the program root. */
int __Program_Load_Root__(__Program__ *__Program_State__, const char *__Root_Path__);

/* Returns the program unit count. */
size_t __Program_Unit_Count__(const __Program__ *__Program_State__);

/* Returns the program unit at. */
__Program_Unit__ *__Program_Unit_At__(__Program__ *__Program_State__, size_t __Index__);

/* Returns the program unit at const. */
const __Program_Unit__ *__Program_Unit_At_Const__(const __Program__ *__Program_State__,
                                                  size_t __Index__);

/* Returns the program diagnostic. */
const __Diagnostic__ *__Program_Diagnostic__(const __Program__ *__Program_State__);

#endif
