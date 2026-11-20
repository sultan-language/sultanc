/* module.c: parsing and validating the module.
 *
 * Accepted: the integer MVP, sign-extension operators, f32/f64 loads, stores,
 * constants, arithmetic, comparisons and reinterpretation, bulk memory.copy
 * and memory.fill, one funcref table, one 32-bit memory, mutable and
 * immutable globals, active element and data segments, a start function, and
 * function imports from wasi_snapshot_preview1 whose names and signatures
 * match `wasi_imports` below. Anything else is rejected with a diagnostic.
 */
#include "module.h"

#include <stdlib.h>
#include <string.h>

#include "stream.h"

int is_value_type(unsigned t) { return t == T_I32 || t == T_I64 || t == T_F32 || t == T_F64; }
const char *ctype(unsigned t) {
    switch (t) {
        case T_I32: return "uint32_t";
        case T_I64: return "uint64_t";
        case T_F32: return "float";
        case T_F64: return "double";
        case T_NONE: return "void";
    }
    die("internal: bad value type %u", t);
    return "";
}
char tchar(unsigned t) {
    switch (t) {
        case T_I32: return 'i';
        case T_I64: return 'l';
        case T_F32: return 'f';
        case T_F64: return 'd';
    }
    die("internal: bad value type %u", t);
    return '?';
}
const char *tname(unsigned t) {
    switch (t) {
        case T_I32: return "i32";
        case T_I64: return "i64";
        case T_F32: return "f32";
        case T_F64: return "f64";
        case T_NONE: return "none";
    }
    return "?";
}

FuncType *types;
uint32_t num_types;
Func *funcs;
uint32_t num_funcs, num_imported;
Global *globals;
uint32_t num_globals;
int have_table;
uint32_t table_min;
int have_memory;
uint32_t memory_min, memory_max;
int memory_has_max;
ElemSeg *elems;
uint32_t num_elems;
DataSeg *datas;
uint32_t num_datas;
int have_start;
uint32_t start_func;
uint32_t entry_func;
static uint32_t table_max;
static int table_has_max, have_entry, have_datacount;
static uint32_t datacount;

/* The WASI Preview 1 functions the host (runtime/wasi.c) implements, with their
 * WebAssembly signatures: 'i' = i32, 'I' = i64, after ':' the result ('' =
 * none). These are every import that SultanC's std WASI provider declares. */
static const struct {
    const char *name, *sig;
} wasi_imports[] = {
    {"args_get", "ii:i"},
    {"args_sizes_get", "ii:i"},
    {"clock_time_get", "iIi:i"},
    {"environ_get", "ii:i"},
    {"environ_sizes_get", "ii:i"},
    {"fd_close", "i:i"},
    {"fd_datasync", "i:i"},
    {"fd_filestat_get", "ii:i"},
    {"fd_prestat_dir_name", "iii:i"},
    {"fd_prestat_get", "ii:i"},
    {"fd_read", "iiii:i"},
    {"fd_readdir", "iiiIi:i"},
    {"fd_seek", "iIii:i"},
    {"fd_sync", "i:i"},
    {"fd_tell", "ii:i"},
    {"fd_write", "iiii:i"},
    {"path_create_directory", "iii:i"},
    {"path_filestat_get", "iiiii:i"},
    {"path_open", "iiiiiIIii:i"},
    {"path_remove_directory", "iii:i"},
    {"path_rename", "iiiiii:i"},
    {"path_unlink_file", "iii:i"},
    {"poll_oneoff", "iiii:i"},
    {"proc_exit", "i:"},
    {"random_get", "ii:i"},
};

static void type_signature(const FuncType *t, char *buf, size_t cap) {
    size_t n = 0;
    uint32_t i;
    for (i = 0; i < t->nparams && n + 2 < cap; i++)
        buf[n++] = t->params[i] == T_I32 ? 'i' : t->params[i] == T_I64 ? 'I'
                 : t->params[i] == T_F32 ? 'f' : 'F';
    if (n + 2 < cap) buf[n++] = ':';
    if (t->result != T_NONE && n + 2 < cap)
        buf[n++] = t->result == T_I32 ? 'i' : t->result == T_I64 ? 'I'
                 : t->result == T_F32 ? 'f' : 'F';
    buf[n] = 0;
}

static void check_wasi_import(const char *module, const char *name, uint32_t type) {
    char sig[160];
    size_t i;
    if (strcmp(module, "wasi_snapshot_preview1") != 0)
        die("import %s.%s: only wasi_snapshot_preview1 imports are supported", module, name);
    if (type >= num_types) die("import %s: bad type index", name);
    type_signature(&types[type], sig, sizeof(sig));
    for (i = 0; i < sizeof(wasi_imports) / sizeof(wasi_imports[0]); i++) {
        if (strcmp(wasi_imports[i].name, name) == 0) {
            if (strcmp(wasi_imports[i].sig, sig) != 0)
                die("import %s has signature %s, the WASI host implements %s", name, sig,
                    wasi_imports[i].sig);
            return;
        }
    }
    die("import %s is not implemented by the WASI host (runtime/wasi.c)", name);
}

static uint64_t read_const_expr(unsigned expect_type) {
    unsigned op = rd_u8();
    uint64_t bits = 0;
    switch (op) {
        case 0x41: if (expect_type != T_I32) die("constant type mismatch"); bits = rd_s32(); break;
        case 0x42: if (expect_type != T_I64) die("constant type mismatch"); bits = rd_s64(); break;
        case 0x43: {
            unsigned i;
            if (expect_type != T_F32) die("constant type mismatch");
            for (i = 0; i < 4; i++) bits |= (uint64_t)rd_u8() << (8 * i);
            break;
        }
        case 0x44: {
            unsigned i;
            if (expect_type != T_F64) die("constant type mismatch");
            for (i = 0; i < 8; i++) bits |= (uint64_t)rd_u8() << (8 * i);
            break;
        }
        default: die("unsupported constant expression opcode 0x%02x", op);
    }
    if (rd_u8() != 0x0b) die("constant expression not terminated by end");
    return bits;
}

static void read_limits(uint32_t *min, uint32_t *max, int *has_max) {
    unsigned flags = rd_u8();
    if (flags != 0 && flags != 1) die("unsupported limits flags 0x%02x (shared/64-bit?)", flags);
    *min = rd_u32();
    *has_max = flags == 1;
    *max = *has_max ? rd_u32() : 0;
    if (*has_max && *max < *min) die("limits maximum below minimum");
}

void parse_module(void) {
    int last_id = 0;
    size_t code_pos = 0;
    uint32_t declared_funcs = 0, i, j;
    int have_code = 0;

    if (in_size < 8 || memcmp(in_bytes, "\0asm\1\0\0\0", 8) != 0)
        die("not a WebAssembly 1.0 binary module");
    rd_seek(8);
    while (in_pos < in_size) {
        unsigned id = rd_u8();
        uint32_t len = rd_u32();
        size_t start = in_pos, end = start + (size_t)len;
        if (end > in_size) die("section %u runs past end of module", id);
        if (id != 0) {
            /* Section order: 1..11, with datacount (12) between import-related
             * sections and code; enforce the standard order. */
            static const int order[13] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 10};
            if (id > 12) die("unsupported section id %u", id);
            if (order[id] <= last_id) die("section %u out of order or duplicated", id);
            last_id = order[id];
        }
        switch (id) {
            case 0: /* custom (name, producers, debug): no semantics */
                break;
            case 1:
                num_types = rd_u32();
                types = xcalloc(num_types, sizeof(*types));
                for (i = 0; i < num_types; i++) {
                    FuncType *t = &types[i];
                    uint32_t nres;
                    if (rd_u8() != 0x60) die("type %u is not a function type", i);
                    t->nparams = rd_u32();
                    if (t->nparams > 1000) die("type %u has too many parameters", i);
                    t->params = xmalloc(t->nparams);
                    for (j = 0; j < t->nparams; j++) {
                        t->params[j] = rd_u8();
                        if (!is_value_type(t->params[j])) die("type %u: unsupported parameter type", i);
                    }
                    nres = rd_u32();
                    if (nres > 1) die("type %u: multiple results are not supported", i);
                    t->result = nres ? rd_u8() : T_NONE;
                    if (nres && !is_value_type(t->result)) die("type %u: unsupported result type", i);
                    t->canon = i;
                    for (j = 0; j < i; j++) {
                        if (types[j].nparams == t->nparams && types[j].result == t->result &&
                            memcmp(types[j].params, t->params, t->nparams) == 0) {
                            t->canon = types[j].canon;
                            break;
                        }
                    }
                }
                break;
            case 2: {
                uint32_t n = rd_u32();
                funcs = xrealloc(funcs, (size_t)n * sizeof(*funcs));
                for (i = 0; i < n; i++) {
                    char *module = rd_name();
                    char *name = rd_name();
                    unsigned kind = rd_u8();
                    if (kind != 0) die("import %s.%s: only function imports are supported", module, name);
                    memset(&funcs[num_funcs], 0, sizeof(Func));
                    funcs[num_funcs].type = rd_u32();
                    funcs[num_funcs].imported = 1;
                    funcs[num_funcs].import_name = name;
                    check_wasi_import(module, name, funcs[num_funcs].type);
                    free(module);
                    num_funcs++;
                }
                num_imported = num_funcs;
                break;
            }
            case 3:
                declared_funcs = rd_u32();
                if (declared_funcs > 10000000) die("too many functions");
                funcs = xrealloc(funcs, ((size_t)num_funcs + declared_funcs) * sizeof(*funcs));
                for (i = 0; i < declared_funcs; i++) {
                    memset(&funcs[num_funcs], 0, sizeof(Func));
                    funcs[num_funcs].type = rd_u32();
                    if (funcs[num_funcs].type >= num_types) die("function %u: bad type index", num_funcs);
                    num_funcs++;
                }
                break;
            case 4: {
                uint32_t n = rd_u32();
                if (n > 1) die("more than one table");
                if (n == 1) {
                    if (rd_u8() != 0x70) die("table element type is not funcref");
                    read_limits(&table_min, &table_max, &table_has_max);
                    have_table = 1;
                }
                break;
            }
            case 5: {
                uint32_t n = rd_u32();
                if (n > 1) die("more than one memory");
                if (n == 1) {
                    read_limits(&memory_min, &memory_max, &memory_has_max);
                    if (memory_min > 65536 || (memory_has_max && memory_max > 65536))
                        die("memory limits exceed 4 GiB");
                    have_memory = 1;
                }
                break;
            }
            case 6:
                num_globals = rd_u32();
                globals = xcalloc(num_globals, sizeof(*globals));
                for (i = 0; i < num_globals; i++) {
                    unsigned mut;
                    globals[i].type = rd_u8();
                    if (!is_value_type(globals[i].type)) die("global %u: unsupported type", i);
                    mut = rd_u8();
                    if (mut > 1) die("global %u: bad mutability", i);
                    globals[i].mutable_ = (int)mut;
                    globals[i].init_bits = read_const_expr(globals[i].type);
                }
                break;
            case 7: {
                uint32_t n = rd_u32();
                for (i = 0; i < n; i++) {
                    char *name = rd_name();
                    unsigned kind = rd_u8();
                    uint32_t index = rd_u32();
                    if (strcmp(name, "_start") == 0) {
                        if (kind != 0) die("_start is not a function");
                        have_entry = 1;
                        entry_func = index;
                    }
                    /* Other exports (memory, __indirect_function_table, ...)
                     * have no meaning for a translated command. */
                    free(name);
                }
                break;
            }
            case 8:
                have_start = 1;
                start_func = rd_u32();
                break;
            case 9:
                num_elems = rd_u32();
                elems = xcalloc(num_elems, sizeof(*elems));
                for (i = 0; i < num_elems; i++) {
                    uint32_t flags = rd_u32();
                    if (flags != 0) die("element segment %u: only active table-0 segments are supported", i);
                    elems[i].offset = (uint32_t)read_const_expr(T_I32);
                    elems[i].count = rd_u32();
                    elems[i].funcs = xmalloc((size_t)elems[i].count * sizeof(uint32_t));
                    for (j = 0; j < elems[i].count; j++) elems[i].funcs[j] = rd_u32();
                }
                break;
            case 10:
                have_code = 1;
                code_pos = in_pos;
                break;
            case 11:
                num_datas = rd_u32();
                datas = xcalloc(num_datas, sizeof(*datas));
                for (i = 0; i < num_datas; i++) {
                    uint32_t flags = rd_u32();
                    if (flags != 0) die("data segment %u: only active memory-0 segments are supported", i);
                    datas[i].offset = (uint32_t)read_const_expr(T_I32);
                    datas[i].size = rd_u32();
                    datas[i].pos = in_pos;
                    if ((size_t)datas[i].size > in_size - in_pos) die("data segment %u runs past end", i);
                    in_pos += datas[i].size;
                }
                break;
            case 12:
                have_datacount = 1;
                datacount = rd_u32();
                break;
        }
        if (in_pos > end) die("section %u overruns its declared size", id);
        if (id != 0 && id != 10 && in_pos != end) die("section %u has trailing bytes", id);
        rd_seek(end);
    }

    /* Cross-section validation. */
    if (num_funcs != num_imported + declared_funcs) die("internal: function count");
    if (!have_code && declared_funcs) die("function section without code section");
    if (have_code) {
        uint32_t count;
        rd_seek(code_pos);
        count = rd_u32();
        if (count != declared_funcs) die("code section has %u bodies for %u functions", count, declared_funcs);
        for (i = num_imported; i < num_funcs; i++) {
            uint32_t size = rd_u32();
            if ((size_t)size > in_size - in_pos) die("function %u body runs past end", i);
            funcs[i].body_start = in_pos;
            funcs[i].body_end = in_pos + size;
            in_pos += size;
        }
    }
    if (!have_entry) die("module does not export _start");
    if (entry_func >= num_funcs) die("_start index out of range");
    if (types[funcs[entry_func].type].nparams || types[funcs[entry_func].type].result != T_NONE)
        die("_start must have type [] -> []");
    if (have_start) {
        if (start_func >= num_funcs) die("start function out of range");
        if (types[funcs[start_func].type].nparams || types[funcs[start_func].type].result != T_NONE)
            die("start function must have type [] -> []");
    }
    if (have_datacount && datacount != num_datas) die("data count section disagrees with data section");
    for (i = 0; i < num_elems; i++) {
        if (!have_table) die("element segment without a table");
        if ((uint64_t)elems[i].offset + elems[i].count > table_min)
            die("element segment %u exceeds the table's initial size", i);
        for (j = 0; j < elems[i].count; j++)
            if (elems[i].funcs[j] >= num_funcs) die("element segment %u: bad function index", i);
    }
    for (i = 0; i < num_datas; i++) {
        if (!have_memory) die("data segment without a memory");
        if ((uint64_t)datas[i].offset + datas[i].size > (uint64_t)memory_min * 65536)
            die("data segment %u exceeds the memory's initial size", i);
    }
}

/* ---- output buffer ------------------------------------------------------------- */
