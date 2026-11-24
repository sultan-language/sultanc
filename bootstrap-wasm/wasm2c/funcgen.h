/* funcgen.h: translating one function body to C. */
#ifndef W2C_FUNCGEN_H
#define W2C_FUNCGEN_H

#include "common.h"

/* The C name of function `idx`: w2c_f<idx>, or the WASI host function. */
void func_c_name(uint32_t idx, char *buf, size_t cap);
/* `result name(params)`; parameters are named v0.. when `named`. */
void emit_prototype(Buf *b, uint32_t idx, int named);
/* The C type of a pointer to a function of type `type`. */
void emit_fnptr_type(Buf *b, uint32_t type);

/* Appends the C definition of module function `idx` to `out`. */
void translate_function(Buf *out, uint32_t idx);

#endif
