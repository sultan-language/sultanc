/* module.h: the WebAssembly module model (what the translator accepts) and
 * its parser. */
#ifndef W2C_MODULE_H
#define W2C_MODULE_H

#include "common.h"

/* Value types, by their binary encoding. */
enum { T_NONE = 0, T_I32 = 0x7f, T_I64 = 0x7e, T_F32 = 0x7d, T_F64 = 0x7c };

int is_value_type(unsigned t);
const char *ctype(unsigned t); /* "uint32_t", ... "void" */
char tchar(unsigned t);        /* the stack-slot prefix: i l f d */
const char *tname(unsigned t); /* "i32", ... */

typedef struct {
    uint32_t nparams;
    unsigned char *params;
    unsigned result; /* T_NONE or a value type */
    uint32_t canon;  /* smallest index with a structurally equal signature */
} FuncType;

typedef struct {
    uint32_t type;
    int imported;
    char *import_name; /* WASI function name for imports */
    size_t body_start, body_end;
} Func;

typedef struct {
    unsigned type;
    int mutable_;
    uint64_t init_bits; /* constant initializer, as raw bits */
} Global;

typedef struct {
    uint32_t offset;
    uint32_t count;
    uint32_t *funcs;
} ElemSeg;

typedef struct {
    uint32_t offset;
    uint32_t size;
    size_t pos;
} DataSeg;

extern FuncType *types;
extern uint32_t num_types;
extern Func *funcs;
extern uint32_t num_funcs, num_imported;
extern Global *globals;
extern uint32_t num_globals;
extern int have_table;
extern uint32_t table_min;
extern int have_memory;
extern uint32_t memory_min, memory_max;
extern int memory_has_max;
extern ElemSeg *elems;
extern uint32_t num_elems;
extern DataSeg *datas;
extern uint32_t num_datas;
extern int have_start;
extern uint32_t start_func;
extern uint32_t entry_func;

/* Parses and validates the module in the input stream (stream.h). */
void parse_module(void);

#endif
