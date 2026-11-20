/* output.h: the generated files.
 *
 *   PREFIX.h          declarations shared by the units: globals and every
 *                     function's prototype (includes w2c.h)
 *   PREFIX-module.c   the module's data: memory size, globals, the
 *                     indirect-call table, data segments, w2c_run
 *   PREFIX-N.c        the functions, N = 1..units, in index order, split
 *                     where the code size is balanced
 */
#ifndef W2C_OUTPUT_H
#define W2C_OUTPUT_H

#include "common.h"

void write_header(const char *prefix);
void write_module_unit(const char *prefix);
void write_function_units(const char *prefix, unsigned units);

#endif
