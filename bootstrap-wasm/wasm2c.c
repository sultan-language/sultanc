/* sultanc-wasm2c: translate the SultanC WASI seed (a WebAssembly module) to C.
 * Copyright 2026 SultanC contributors. MIT license (see repository LICENSE).
 *
 * The seed is translated ahead of time to plain C99, compiled with the host C
 * compiler and linked with a small WASI host (wasi.c), so building SultanC
 * from source needs only a C compiler.
 *
 * Contract
 * - Input: one WebAssembly 1.0 module as produced by SultanC's native wasm32
 *   backend and wasm linker. Accepted: the integer MVP, sign-extension
 *   operators, f32/f64 loads/stores/constants/arithmetic/comparisons/
 *   reinterpretation, bulk memory.copy/memory.fill, one funcref table, one
 *   32-bit memory, mutable/immutable globals, active element/data segments,
 *   a start function, and function imports from wasi_snapshot_preview1 whose
 *   names and signatures match the table below.
 * - Anything else is rejected with a diagnostic. Nothing is ever silently
 *   reinterpreted. Extending the accepted set is a reviewed change.
 * - Translation is deterministic: the same seed always gives the same C.
 *
 * Translation scheme
 * - The operand stack is resolved statically (WebAssembly validation makes
 *   every stack height and type known at each instruction). The value at
 *   height h with type T lives in the C local `<T>h` (i0, l3, f2, d5 ...), so
 *   a block's result is always in the slot at the block's entry height.
 * - Control flow becomes labels and gotos; loops are labels at their start.
 * - The translator type-checks every instruction it emits and fails on any
 *   mismatch, so a malformed seed or a translator bug cannot produce C that
 *   silently computes something else.
 * - Every memory access is bounds-checked and traps when out of range.
 * - Every call between module functions counts against a fixed call-depth
 *   limit (W2C_CALL_DEPTH_LIMIT) and checks the native stack the host gave the
 *   guest, so deep recursion traps ("call stack exhausted") instead of
 *   overflowing the native stack. The depth limit is the same on every host,
 *   so where a program traps does not depend on the C compiler's frame sizes
 *   (the native check is only a safety net for hosts with small stacks).
 *
 * Usage: sultanc-wasm2c [--shards=N] SEED.wasm[.zst] OUT_PREFIX
 *        sultanc-wasm2c --decompress FILE.zst OUT
 * A Zstandard-compressed seed (as committed) is decompressed in memory with
 * the in-tree decoder (zstd_decoder.h); a malformed frame is rejected with a
 * diagnostic, as is a malformed module.
 * Outputs: OUT_PREFIX.h, OUT_PREFIX-runtime.c, OUT_PREFIX-NNN.c (N shards).
 */
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zstd_decoder.h"

/* Calls between module functions allowed at once. The SultanC compiler needs
 * a few thousand on deeply nested sources; the limit keeps the native stack
 * use bounded (each level is one C frame of the translated function). */
#define W2C_CALL_DEPTH_LIMIT 100000
#define W2C_STRINGIFY_(x) #x
#define W2C_STRINGIFY(x) W2C_STRINGIFY_(x)

/* ---- diagnostics ------------------------------------------------------------ */

static const char *current_context = "module";

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "sultanc-wasm2c: error (%s): ", current_context);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) die("out of memory");
    return p;
}
static void *xcalloc(size_t count, size_t size) {
    void *p = calloc(count ? count : 1, size ? size : 1);
    if (!p) die("out of memory");
    return p;
}
static void *xrealloc(void *p, size_t n) {
    p = realloc(p, n ? n : 1);
    if (!p) die("out of memory");
    return p;
}

/* ---- input reader --------------------------------------------------------------- */

static const unsigned char *in_bytes;
static size_t in_size, in_pos;

static uint8_t rd_u8(void) {
    if (in_pos >= in_size) die("truncated module at offset %zu", in_pos);
    return in_bytes[in_pos++];
}
static uint64_t rd_leb(unsigned bits, int is_signed) {
    uint64_t result = 0;
    unsigned shift = 0;
    uint8_t byte;
    for (;;) {
        byte = rd_u8();
        if (shift >= bits + 7 || shift >= 64) die("LEB128 too long at offset %zu", in_pos);
        result |= (uint64_t)(byte & 0x7f) << shift;
        shift += 7;
        if (!(byte & 0x80)) break;
    }
    if (is_signed && shift < 64 && (byte & 0x40)) result |= ~UINT64_C(0) << shift;
    if (bits == 32) {
        if (is_signed) {
            int64_t v = (int64_t)result;
            if (v < INT32_MIN || v > INT32_MAX) die("s32 out of range at offset %zu", in_pos);
            return (uint64_t)(uint32_t)(int32_t)v;
        }
        if (result > UINT32_MAX) die("u32 out of range at offset %zu", in_pos);
    }
    return result;
}
static uint32_t rd_u32(void) { return (uint32_t)rd_leb(32, 0); }
static uint32_t rd_s32(void) { return (uint32_t)rd_leb(32, 1); }
static uint64_t rd_s64(void) { return rd_leb(64, 1); }
static int64_t rd_s33(void) {
    /* Block types: a signed 33-bit LEB (negative single-byte forms for value
     * types and the empty type, non-negative type indices). */
    uint64_t v = rd_leb(64, 1);
    return (int64_t)v;
}
static void rd_seek(size_t pos) {
    if (pos > in_size) die("offset %zu outside module", pos);
    in_pos = pos;
}
static char *rd_name(void) {
    uint32_t n = rd_u32();
    char *s;
    if ((size_t)n > in_size - in_pos) die("name runs past end of module");
    s = xmalloc((size_t)n + 1);
    memcpy(s, in_bytes + in_pos, n);
    s[n] = 0;
    in_pos += n;
    if (strlen(s) != n) die("name contains NUL");
    return s;
}

/* ---- module model ------------------------------------------------------------ */

enum { T_NONE = 0, T_I32 = 0x7f, T_I64 = 0x7e, T_F32 = 0x7d, T_F64 = 0x7c };

static int is_value_type(unsigned t) { return t == T_I32 || t == T_I64 || t == T_F32 || t == T_F64; }
static const char *ctype(unsigned t) {
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
static char tchar(unsigned t) {
    switch (t) {
        case T_I32: return 'i';
        case T_I64: return 'l';
        case T_F32: return 'f';
        case T_F64: return 'd';
    }
    die("internal: bad value type %u", t);
    return '?';
}
static const char *tname(unsigned t) {
    switch (t) {
        case T_I32: return "i32";
        case T_I64: return "i64";
        case T_F32: return "f32";
        case T_F64: return "f64";
        case T_NONE: return "none";
    }
    return "?";
}

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

static FuncType *types;
static uint32_t num_types;
static Func *funcs;
static uint32_t num_funcs, num_imported;
static Global *globals;
static uint32_t num_globals;
static int have_table;
static uint32_t table_min, table_max;
static int table_has_max;
static int have_memory;
static uint32_t memory_min, memory_max;
static int memory_has_max;
static ElemSeg *elems;
static uint32_t num_elems;
static DataSeg *datas;
static uint32_t num_datas;
static int have_start;
static uint32_t start_func;
static int have_entry;
static uint32_t entry_func;
static int have_datacount;
static uint32_t datacount;

/* The WASI Preview 1 functions the host (wasi.c) implements, with their
 * WebAssembly signatures: 'i' = i32, 'I' = i64, after ':' the result ('' =
 * none). These are every import that SultanC's std WASI provider declares. */
static const struct { const char *name, *sig; } wasi_imports[] = {
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
    die("import %s is not implemented by the WASI host (wasi.c)", name);
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

static void parse_module(void) {
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

typedef struct { char *s; size_t n, cap; } Buf;

static void buf_put(Buf *b, const char *s, size_t n) {
    if (b->n + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 4096;
        while (cap < b->n + n + 1) cap *= 2;
        b->s = xrealloc(b->s, cap);
        b->cap = cap;
    }
    memcpy(b->s + b->n, s, n);
    b->n += n;
    b->s[b->n] = 0;
}
static void bprintf(Buf *b, const char *fmt, ...) {
    char tmp[1024];
    int n;
    va_list ap;
    va_start(ap, fmt);
    n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0) die("internal: format failure");
    if ((size_t)n >= sizeof(tmp)) {
        char *big = xmalloc((size_t)n + 1);
        va_start(ap, fmt);
        vsnprintf(big, (size_t)n + 1, fmt, ap);
        va_end(ap);
        buf_put(b, big, (size_t)n);
        free(big);
    } else {
        buf_put(b, tmp, (size_t)n);
    }
}

static FILE *open_out(const char *prefix, const char *suffix) {
    size_t n = strlen(prefix) + strlen(suffix) + 1;
    char *path = xmalloc(n);
    FILE *f;
    snprintf(path, n, "%s%s", prefix, suffix);
    f = fopen(path, "wb");
    if (!f) die("cannot create %s", path);
    free(path);
    return f;
}
static void write_out(FILE *f, const Buf *b) {
    if (b->n && fwrite(b->s, 1, b->n, f) != b->n) die("write failed");
}
static void close_out(FILE *f) {
    if (fclose(f) != 0) die("write failed");
}

/* ---- function prototypes --------------------------------------------------------- */

static void func_c_name(uint32_t idx, char *buf, size_t cap) {
    if (funcs[idx].imported)
        snprintf(buf, cap, "wasi_snapshot_preview1_%s", funcs[idx].import_name);
    else
        snprintf(buf, cap, "w2c_f%" PRIu32, idx);
}

static void emit_prototype(Buf *b, uint32_t idx, int with_param_names) {
    const FuncType *t = &types[funcs[idx].type];
    char name[160];
    uint32_t i;
    func_c_name(idx, name, sizeof(name));
    bprintf(b, "%s %s(", ctype(t->result), name);
    if (!t->nparams) bprintf(b, "void");
    for (i = 0; i < t->nparams; i++) {
        if (with_param_names)
            bprintf(b, "%s%s p%" PRIu32, i ? ", " : "", ctype(t->params[i]), i);
        else
            bprintf(b, "%s%s", i ? ", " : "", ctype(t->params[i]));
    }
    bprintf(b, ")");
}

static void emit_fnptr_type(Buf *b, uint32_t type) {
    const FuncType *t = &types[type];
    uint32_t i;
    bprintf(b, "%s (*)(", ctype(t->result));
    if (!t->nparams) bprintf(b, "void");
    for (i = 0; i < t->nparams; i++) bprintf(b, "%s%s", i ? ", " : "", ctype(t->params[i]));
    bprintf(b, ")");
}

/* ---- function body translation --------------------------------------------------- */

enum { K_FUNC, K_BLOCK, K_LOOP, K_IF };

typedef struct {
    int kind;
    uint32_t id;
    uint32_t height;   /* operand stack height at entry */
    unsigned result;   /* T_NONE or a value type */
    int live_at_entry; /* was the instruction that opened it reachable */
    int had_else;
} Ctl;

/* Per-function translation state. */
static unsigned char *stack_types;
static uint32_t stack_height, stack_cap;
static Ctl *ctls;
static uint32_t ctl_depth, ctl_cap;
static int live;
static unsigned char *local_types;
static uint32_t num_locals;
static uint32_t max_height[4]; /* per type: one past the highest slot used */
static uint32_t next_label;

static int type_slot_index(unsigned t) {
    switch (t) {
        case T_I32: return 0;
        case T_I64: return 1;
        case T_F32: return 2;
        case T_F64: return 3;
    }
    die("internal: slot type");
    return 0;
}

static void push(unsigned t) {
    int k;
    if (stack_height == stack_cap) {
        stack_cap = stack_cap ? stack_cap * 2 : 64;
        stack_types = xrealloc(stack_types, stack_cap);
    }
    stack_types[stack_height++] = (unsigned char)t;
    k = type_slot_index(t);
    if (stack_height > max_height[k]) max_height[k] = stack_height;
}
static unsigned top_type(void) {
    if (stack_height <= ctls[ctl_depth - 1].height) die("operand stack underflow");
    return stack_types[stack_height - 1];
}
static uint32_t pop(unsigned expect) {
    unsigned t;
    if (stack_height <= ctls[ctl_depth - 1].height) die("operand stack underflow");
    t = stack_types[stack_height - 1];
    if (expect != T_NONE && t != expect) die("type mismatch: expected %s, found %s", tname(expect), tname(t));
    return --stack_height;
}

/* "x<h>" for the slot at height h, by its type. */
static const char *slot(uint32_t h) {
    static char bufs[8][32];
    static unsigned next;
    char *b = bufs[next++ % 8];
    if (h >= stack_height + 1 && h >= stack_cap) die("internal: slot index");
    snprintf(b, 32, "%c%" PRIu32, tchar(stack_types[h]), h);
    return b;
}
static const char *slot_typed(unsigned t, uint32_t h) {
    static char bufs[8][32];
    static unsigned next;
    char *b = bufs[next++ % 8];
    snprintf(b, 32, "%c%" PRIu32, tchar(t), h);
    return b;
}

static void push_ctl(int kind, unsigned result) {
    if (ctl_depth == ctl_cap) {
        ctl_cap = ctl_cap ? ctl_cap * 2 : 32;
        ctls = xrealloc(ctls, (size_t)ctl_cap * sizeof(*ctls));
    }
    ctls[ctl_depth].kind = kind;
    ctls[ctl_depth].id = next_label++;
    ctls[ctl_depth].height = stack_height;
    ctls[ctl_depth].result = result;
    ctls[ctl_depth].live_at_entry = live;
    ctls[ctl_depth].had_else = 0;
    ctl_depth++;
}

static unsigned read_block_type(void) {
    int64_t bt = rd_s33();
    if (bt == -64) return T_NONE;           /* 0x40 empty */
    if (bt == -1) return T_I32;             /* 0x7f */
    if (bt == -2) return T_I64;             /* 0x7e */
    if (bt == -3) return T_F32;             /* 0x7d */
    if (bt == -4) return T_F64;             /* 0x7c */
    if (bt >= 0 && (uint64_t)bt < num_types) {
        const FuncType *t = &types[bt];
        if (t->nparams) die("block types with parameters are not supported");
        return t->result;
    }
    die("unsupported block type %" PRId64, bt);
    return T_NONE;
}

/* Branch to control entry `target_index` (0 = outermost/function). For a block
 * result, the value on top is first copied into the slot at the target's
 * entry height. Emits nothing but the jump; the caller handles liveness. */
static void emit_branch(Buf *b, uint32_t target_index, const FuncType *ft) {
    Ctl *c = &ctls[target_index];
    if (c->kind == K_FUNC) {
        if (ft->result != T_NONE) {
            if (top_type() != ft->result) die("type mismatch at function-level branch");
            bprintf(b, "return %s;", slot(stack_height - 1));
        } else {
            bprintf(b, "return;");
        }
        return;
    }
    if (c->kind == K_LOOP) {
        bprintf(b, "goto L%" PRIu32 ";", c->id);
        return;
    }
    if (c->result != T_NONE) {
        if (top_type() != c->result) die("branch value type mismatch");
        if (stack_height - 1 != c->height)
            bprintf(b, "%s = %s;", slot_typed(c->result, c->height), slot(stack_height - 1));
    }
    bprintf(b, "goto B%" PRIu32 ";", c->id);
}

static uint32_t memarg(uint32_t max_align_log2) {
    uint32_t align = rd_u32();
    uint32_t offset = rd_u32();
    if (align > max_align_log2) die("memory access alignment larger than natural");
    if (!have_memory) die("memory access without a memory");
    return offset;
}

static void translate_body(Buf *out, uint32_t idx) {
    const FuncType *ft = &types[funcs[idx].type];
    Buf body = {0};
    uint32_t i, k;
    char name[64];

    snprintf(name, sizeof(name), "function %" PRIu32, idx);
    current_context = name;

    rd_seek(funcs[idx].body_start);
    /* Locals: parameters first, then declared groups. */
    num_locals = ft->nparams;
    local_types = xrealloc(local_types, (size_t)ft->nparams + 1);
    memcpy(local_types, ft->params, ft->nparams);
    {
        uint32_t groups = rd_u32();
        for (i = 0; i < groups; i++) {
            uint32_t count = rd_u32();
            unsigned t = rd_u8();
            if (!is_value_type(t)) die("unsupported local type 0x%02x", t);
            if ((uint64_t)num_locals + count > 50000) die("too many locals");
            local_types = xrealloc(local_types, (size_t)num_locals + count + 1);
            memset(local_types + num_locals, (int)t, count);
            num_locals += count;
        }
    }

    stack_height = 0;
    ctl_depth = 0;
    next_label = 0;
    live = 1;
    memset(max_height, 0, sizeof(max_height));
    push_ctl(K_FUNC, ft->result);

    while (ctl_depth > 0) {
        size_t at = in_pos;
        unsigned op;
        if (in_pos >= funcs[idx].body_end) die("body ends before its final end");
        op = rd_u8();

        /* Structured control instructions are tracked in dead code too. */
        switch (op) {
            case 0x02: /* block */
            case 0x03: /* loop */ {
                unsigned bt = read_block_type();
                push_ctl(op == 0x02 ? K_BLOCK : K_LOOP, bt);
                if (live && op == 0x03) bprintf(&body, "L%" PRIu32 ":;\n", ctls[ctl_depth - 1].id);
                continue;
            }
            case 0x04: /* if */ {
                unsigned bt = read_block_type();
                if (live) {
                    uint32_t c = pop(T_I32);
                    push_ctl(K_IF, bt);
                    bprintf(&body, "if (!%s) goto E%" PRIu32 ";\n", slot_typed(T_I32, c), ctls[ctl_depth - 1].id);
                } else {
                    push_ctl(K_IF, bt);
                }
                continue;
            }
            case 0x05: /* else */ {
                Ctl *c = &ctls[ctl_depth - 1];
                if (c->kind != K_IF || c->had_else) die("else without matching if at offset %zu", at);
                if (live) {
                    if (stack_height != c->height + (c->result != T_NONE))
                        die("if-branch leaves %u values, expected %u", stack_height - c->height,
                            (unsigned)(c->result != T_NONE));
                    if (c->result != T_NONE && stack_types[c->height] != c->result)
                        die("if-branch result type mismatch");
                    bprintf(&body, "goto B%" PRIu32 ";\n", c->id);
                }
                if (c->live_at_entry) bprintf(&body, "E%" PRIu32 ":;\n", c->id);
                c->had_else = 1;
                stack_height = c->height;
                live = c->live_at_entry;
                continue;
            }
            case 0x0b: /* end */ {
                Ctl c = ctls[ctl_depth - 1];
                if (live) {
                    if (stack_height != c.height + (c.result != T_NONE))
                        die("block leaves %u values, expected %u", stack_height - c.height,
                            (unsigned)(c.result != T_NONE));
                    if (c.result != T_NONE && stack_types[c.height] != c.result)
                        die("block result type mismatch");
                }
                if (c.kind == K_FUNC) {
                    if (live) {
                        if (ft->result != T_NONE) bprintf(&body, "return %s;\n", slot(0));
                    }
                    ctl_depth--;
                    break;
                }
                if (c.kind == K_IF && !c.had_else) {
                    if (c.result != T_NONE) die("if without else cannot produce a value");
                    if (c.live_at_entry) bprintf(&body, "E%" PRIu32 ":;\n", c.id);
                }
                if (c.live_at_entry && c.kind != K_LOOP) bprintf(&body, "B%" PRIu32 ":;\n", c.id);
                ctl_depth--;
                stack_height = c.height;
                live = c.live_at_entry;
                if (live && c.result != T_NONE) push(c.result);
                continue;
            }
            default:
                break;
        }
        if (ctl_depth == 0) break;

        if (!live) {
            /* Unreachable code: decode immediates only. */
            switch (op) {
                case 0x00: case 0x01: case 0x0f: case 0x1a: case 0x1b: break;
                case 0x0c: case 0x0d: (void)rd_u32(); break;
                case 0x0e: { uint32_t n = rd_u32(); for (i = 0; i <= n; i++) (void)rd_u32(); break; }
                case 0x10: (void)rd_u32(); break;
                case 0x11: (void)rd_u32(); (void)rd_u32(); break;
                case 0x1c: { uint32_t n = rd_u32(); for (i = 0; i < n; i++) (void)rd_u8(); break; }
                case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: (void)rd_u32(); break;
                case 0x3f: case 0x40: (void)rd_u8(); break;
                case 0x41: (void)rd_s32(); break;
                case 0x42: (void)rd_s64(); break;
                case 0x43: for (i = 0; i < 4; i++) (void)rd_u8(); break;
                case 0x44: for (i = 0; i < 8; i++) (void)rd_u8(); break;
                case 0xfc: {
                    uint32_t sub = rd_u32();
                    if (sub == 10) { (void)rd_u8(); (void)rd_u8(); }
                    else if (sub == 11) { (void)rd_u8(); }
                    else die("unsupported 0xfc %u instruction at offset %zu", sub, at);
                    break;
                }
                default:
                    if (op >= 0x28 && op <= 0x3e) { (void)rd_u32(); (void)rd_u32(); }
                    else if (op >= 0x45 && op <= 0xc4) { /* no immediates */ }
                    else die("unknown opcode 0x%02x at offset %zu", op, at);
            }
            continue;
        }

        switch (op) {
            case 0x00:
                bprintf(&body, "w2c_trap(\"unreachable executed\");\n");
                live = 0;
                break;
            case 0x01:
                break;
            case 0x0c: { /* br */
                uint32_t depth = rd_u32();
                if (depth >= ctl_depth) die("br depth out of range");
                emit_branch(&body, ctl_depth - 1 - depth, ft);
                bprintf(&body, "\n");
                live = 0;
                break;
            }
            case 0x0d: { /* br_if */
                uint32_t depth = rd_u32(), c;
                if (depth >= ctl_depth) die("br_if depth out of range");
                c = pop(T_I32);
                bprintf(&body, "if (%s) { ", slot_typed(T_I32, c));
                emit_branch(&body, ctl_depth - 1 - depth, ft);
                bprintf(&body, " }\n");
                break;
            }
            case 0x0e: { /* br_table */
                uint32_t n = rd_u32(), c, *targets;
                if (n > 1000000) die("br_table too large");
                targets = xmalloc(((size_t)n + 1) * sizeof(uint32_t));
                for (i = 0; i <= n; i++) {
                    targets[i] = rd_u32();
                    if (targets[i] >= ctl_depth) die("br_table depth out of range");
                }
                c = pop(T_I32);
                bprintf(&body, "switch (%s) {\n", slot_typed(T_I32, c));
                for (i = 0; i < n; i++) {
                    bprintf(&body, "case %" PRIu32 "u: ", i);
                    emit_branch(&body, ctl_depth - 1 - targets[i], ft);
                    bprintf(&body, "\n");
                }
                bprintf(&body, "default: ");
                emit_branch(&body, ctl_depth - 1 - targets[n], ft);
                bprintf(&body, "\n}\n");
                free(targets);
                live = 0;
                break;
            }
            case 0x0f: /* return */
                emit_branch(&body, 0, ft);
                bprintf(&body, "\n");
                live = 0;
                break;
            case 0x10: { /* call */
                uint32_t callee = rd_u32(), base;
                const FuncType *ct;
                char cname[160];
                if (callee >= num_funcs) die("call target out of range");
                ct = &types[funcs[callee].type];
                for (i = ct->nparams; i > 0; i--) pop(ct->params[i - 1]);
                base = stack_height;
                func_c_name(callee, cname, sizeof(cname));
                /* Imports (WASI host functions) do not recurse into the module. */
                if (!funcs[callee].imported) bprintf(&body, "w2c_enter();\n");
                if (ct->result != T_NONE) bprintf(&body, "%s = ", slot_typed(ct->result, base));
                bprintf(&body, "%s(", cname);
                for (i = 0; i < ct->nparams; i++)
                    bprintf(&body, "%s%s", i ? ", " : "", slot_typed(ct->params[i], base + i));
                bprintf(&body, ");\n");
                if (!funcs[callee].imported) bprintf(&body, "w2c_call_depth--;\n");
                if (ct->result != T_NONE) push(ct->result);
                break;
            }
            case 0x11: { /* call_indirect */
                uint32_t type = rd_u32(), table = rd_u32(), c, base;
                const FuncType *ct;
                if (table != 0 || !have_table) die("call_indirect without table 0");
                if (type >= num_types) die("call_indirect type out of range");
                ct = &types[type];
                c = pop(T_I32);
                for (i = ct->nparams; i > 0; i--) pop(ct->params[i - 1]);
                base = stack_height;
                bprintf(&body, "w2c_enter();\n");
                if (ct->result != T_NONE) bprintf(&body, "%s = ", slot_typed(ct->result, base));
                bprintf(&body, "((");
                emit_fnptr_type(&body, type);
                bprintf(&body, ")w2c_indirect(%s, %" PRIu32 "u))(", slot_typed(T_I32, c), types[type].canon);
                for (i = 0; i < ct->nparams; i++)
                    bprintf(&body, "%s%s", i ? ", " : "", slot_typed(ct->params[i], base + i));
                bprintf(&body, ");\n");
                bprintf(&body, "w2c_call_depth--;\n");
                if (ct->result != T_NONE) push(ct->result);
                break;
            }
            case 0x1a: /* drop */
                pop(T_NONE);
                break;
            case 0x1b: /* select */
            case 0x1c: { /* select t* */
                uint32_t c, b2, a;
                unsigned t;
                if (op == 0x1c) {
                    uint32_t n = rd_u32();
                    if (n != 1) die("typed select with %u types", n);
                    t = rd_u8();
                    if (!is_value_type(t)) die("typed select: bad type");
                } else {
                    t = T_NONE;
                }
                c = pop(T_I32);
                if (t == T_NONE) t = top_type();
                b2 = pop(t);
                a = pop(t);
                bprintf(&body, "if (!%s) %s = %s;\n", slot_typed(T_I32, c), slot_typed(t, a), slot_typed(t, b2));
                push(t);
                break;
            }
            case 0x20: { /* local.get */
                uint32_t l = rd_u32();
                if (l >= num_locals) die("local index out of range");
                push(local_types[l]);
                bprintf(&body, "%s = v%" PRIu32 ";\n", slot(stack_height - 1), l);
                break;
            }
            case 0x21: { /* local.set */
                uint32_t l = rd_u32(), s;
                if (l >= num_locals) die("local index out of range");
                s = pop(local_types[l]);
                bprintf(&body, "v%" PRIu32 " = %s;\n", l, slot_typed(local_types[l], s));
                break;
            }
            case 0x22: { /* local.tee */
                uint32_t l = rd_u32();
                if (l >= num_locals) die("local index out of range");
                if (top_type() != local_types[l]) die("local.tee type mismatch");
                bprintf(&body, "v%" PRIu32 " = %s;\n", l, slot(stack_height - 1));
                break;
            }
            case 0x23: { /* global.get */
                uint32_t g = rd_u32();
                if (g >= num_globals) die("global index out of range");
                push(globals[g].type);
                bprintf(&body, "%s = w2c_g%" PRIu32 ";\n", slot(stack_height - 1), g);
                break;
            }
            case 0x24: { /* global.set */
                uint32_t g = rd_u32(), s;
                if (g >= num_globals) die("global index out of range");
                if (!globals[g].mutable_) die("global.set on immutable global");
                s = pop(globals[g].type);
                bprintf(&body, "w2c_g%" PRIu32 " = %s;\n", g, slot_typed(globals[g].type, s));
                break;
            }
            case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e:
            case 0x2f: case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: {
                static const struct { const char *fn; unsigned result; unsigned align; } loads[] = {
                    {"w2c_load_i32", T_I32, 2}, {"w2c_load_i64", T_I64, 3},
                    {"w2c_load_f32", T_F32, 2}, {"w2c_load_f64", T_F64, 3},
                    {"w2c_load_i32_8s", T_I32, 0}, {"w2c_load_i32_8u", T_I32, 0},
                    {"w2c_load_i32_16s", T_I32, 1}, {"w2c_load_i32_16u", T_I32, 1},
                    {"w2c_load_i64_8s", T_I64, 0}, {"w2c_load_i64_8u", T_I64, 0},
                    {"w2c_load_i64_16s", T_I64, 1}, {"w2c_load_i64_16u", T_I64, 1},
                    {"w2c_load_i64_32s", T_I64, 2}, {"w2c_load_i64_32u", T_I64, 2}};
                unsigned li = op - 0x28;
                uint32_t off = memarg(loads[li].align), a = pop(T_I32);
                push(loads[li].result);
                bprintf(&body, "%s = %s(%s, %" PRIu32 "u);\n", slot(stack_height - 1), loads[li].fn,
                        slot_typed(T_I32, a), off);
                break;
            }
            case 0x36: case 0x37: case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c:
            case 0x3d: case 0x3e: {
                static const struct { const char *fn; unsigned value; unsigned align; } stores[] = {
                    {"w2c_store_i32", T_I32, 2}, {"w2c_store_i64", T_I64, 3},
                    {"w2c_store_f32", T_F32, 2}, {"w2c_store_f64", T_F64, 3},
                    {"w2c_store_i32_8", T_I32, 0}, {"w2c_store_i32_16", T_I32, 1},
                    {"w2c_store_i64_8", T_I64, 0}, {"w2c_store_i64_16", T_I64, 1},
                    {"w2c_store_i64_32", T_I64, 2}};
                unsigned si = op - 0x36;
                uint32_t off = memarg(stores[si].align), v = pop(stores[si].value), a = pop(T_I32);
                bprintf(&body, "%s(%s, %" PRIu32 "u, %s);\n", stores[si].fn, slot_typed(T_I32, a), off,
                        slot_typed(stores[si].value, v));
                break;
            }
            case 0x3f: /* memory.size */
                if (rd_u8() != 0 || !have_memory) die("memory.size on memory other than 0");
                push(T_I32);
                bprintf(&body, "%s = w2c_memory_pages();\n", slot(stack_height - 1));
                break;
            case 0x40: { /* memory.grow */
                uint32_t a;
                if (rd_u8() != 0 || !have_memory) die("memory.grow on memory other than 0");
                a = pop(T_I32);
                push(T_I32);
                bprintf(&body, "%s = w2c_memory_grow(%s);\n", slot(stack_height - 1), slot_typed(T_I32, a));
                break;
            }
            case 0x41:
                push(T_I32);
                bprintf(&body, "%s = UINT32_C(0x%08" PRIx32 ");\n", slot(stack_height - 1), rd_s32());
                break;
            case 0x42:
                push(T_I64);
                bprintf(&body, "%s = UINT64_C(0x%016" PRIx64 ");\n", slot(stack_height - 1), rd_s64());
                break;
            case 0x43: {
                uint32_t bits = 0;
                for (i = 0; i < 4; i++) bits |= (uint32_t)rd_u8() << (8 * i);
                push(T_F32);
                bprintf(&body, "%s = w2c_f32_from_bits(UINT32_C(0x%08" PRIx32 "));\n", slot(stack_height - 1), bits);
                break;
            }
            case 0x44: {
                uint64_t bits = 0;
                for (i = 0; i < 8; i++) bits |= (uint64_t)rd_u8() << (8 * i);
                push(T_F64);
                bprintf(&body, "%s = w2c_f64_from_bits(UINT64_C(0x%016" PRIx64 "));\n", slot(stack_height - 1), bits);
                break;
            }
            case 0x45: case 0x50: { /* eqz */
                unsigned t = op == 0x45 ? T_I32 : T_I64;
                uint32_t a = pop(t);
                push(T_I32);
                bprintf(&body, "%s = %s == 0;\n", slot(stack_height - 1), slot_typed(t, a));
                break;
            }
            default: {
                /* Binary comparisons: i32 0x46-0x4f, i64 0x51-0x5a, f32 0x5b-0x60, f64 0x61-0x66. */
                if ((op >= 0x46 && op <= 0x4f) || (op >= 0x51 && op <= 0x66)) {
                    static const char *int_cmp[10] = {"==", "!=", "<", "<", ">", ">", "<=", "<=", ">=", ">="};
                    static const char *flt_cmp[6] = {"==", "!=", "<", ">", "<=", ">="};
                    unsigned t;
                    uint32_t bv, av;
                    const char *cmp;
                    int is_signed = 0;
                    if (op <= 0x4f) { t = T_I32; cmp = int_cmp[op - 0x46]; is_signed = (op - 0x46) >= 2 && ((op - 0x46) % 2 == 0); }
                    else if (op <= 0x5a) { t = T_I64; cmp = int_cmp[op - 0x51]; is_signed = (op - 0x51) >= 2 && ((op - 0x51) % 2 == 0); }
                    else if (op <= 0x60) { t = T_F32; cmp = flt_cmp[op - 0x5b]; }
                    else { t = T_F64; cmp = flt_cmp[op - 0x61]; }
                    bv = pop(t);
                    av = pop(t);
                    push(T_I32);
                    if (is_signed) {
                        /* Signed order by flipping the sign bit: no implementation-defined conversions. */
                        if (t == T_I32)
                            bprintf(&body, "%s = (%s ^ UINT32_C(0x80000000)) %s (%s ^ UINT32_C(0x80000000));\n",
                                    slot(stack_height - 1), slot_typed(t, av), cmp, slot_typed(t, bv));
                        else
                            bprintf(&body, "%s = (%s ^ UINT64_C(0x8000000000000000)) %s (%s ^ UINT64_C(0x8000000000000000));\n",
                                    slot(stack_height - 1), slot_typed(t, av), cmp, slot_typed(t, bv));
                    } else {
                        bprintf(&body, "%s = %s %s %s;\n", slot(stack_height - 1), slot_typed(t, av), cmp,
                                slot_typed(t, bv));
                    }
                    break;
                }
                /* Integer unary: clz ctz popcnt. */
                if (op == 0x67 || op == 0x68 || op == 0x69 || op == 0x79 || op == 0x7a || op == 0x7b) {
                    static const char *names[3] = {"clz", "ctz", "popcnt"};
                    unsigned t = op <= 0x69 ? T_I32 : T_I64;
                    unsigned which = op <= 0x69 ? op - 0x67 : op - 0x79;
                    uint32_t a = pop(t);
                    push(t);
                    bprintf(&body, "%s = w2c_%s%s(%s);\n", slot(stack_height - 1), names[which],
                            t == T_I32 ? "32" : "64", slot_typed(t, a));
                    break;
                }
                /* Integer binary: i32 0x6a-0x78, i64 0x7c-0x8a. */
                if ((op >= 0x6a && op <= 0x78) || (op >= 0x7c && op <= 0x8a)) {
                    unsigned t = op <= 0x78 ? T_I32 : T_I64;
                    unsigned which = op <= 0x78 ? op - 0x6a : op - 0x7c;
                    const char *w = t == T_I32 ? "32" : "64";
                    uint32_t bv = pop(t), av = pop(t);
                    const char *r, *x, *y;
                    push(t);
                    r = slot(stack_height - 1);
                    x = slot_typed(t, av);
                    y = slot_typed(t, bv);
                    switch (which) {
                        case 0: bprintf(&body, "%s = %s + %s;\n", r, x, y); break;
                        case 1: bprintf(&body, "%s = %s - %s;\n", r, x, y); break;
                        case 2: bprintf(&body, "%s = w2c_mul%s(%s, %s);\n", r, w, x, y); break;
                        case 3: bprintf(&body, "%s = w2c_div_s%s(%s, %s);\n", r, w, x, y); break;
                        case 4: bprintf(&body, "%s = w2c_div_u%s(%s, %s);\n", r, w, x, y); break;
                        case 5: bprintf(&body, "%s = w2c_rem_s%s(%s, %s);\n", r, w, x, y); break;
                        case 6: bprintf(&body, "%s = w2c_rem_u%s(%s, %s);\n", r, w, x, y); break;
                        case 7: bprintf(&body, "%s = %s & %s;\n", r, x, y); break;
                        case 8: bprintf(&body, "%s = %s | %s;\n", r, x, y); break;
                        case 9: bprintf(&body, "%s = %s ^ %s;\n", r, x, y); break;
                        case 10: bprintf(&body, "%s = %s << (%s & %s);\n", r, x, y, t == T_I32 ? "31u" : "63u"); break;
                        case 11: bprintf(&body, "%s = w2c_shr_s%s(%s, %s);\n", r, w, x, y); break;
                        case 12: bprintf(&body, "%s = %s >> (%s & %s);\n", r, x, y, t == T_I32 ? "31u" : "63u"); break;
                        case 13: bprintf(&body, "%s = w2c_rotl%s(%s, %s);\n", r, w, x, y); break;
                        case 14: bprintf(&body, "%s = w2c_rotr%s(%s, %s);\n", r, w, x, y); break;
                    }
                    break;
                }
                /* Float unary/binary: f32 0x8b-0x98, f64 0x99-0xa6. Only operations
                 * whose IEEE semantics C gives exactly (no libm) are accepted. */
                if (op >= 0x8b && op <= 0xa6) {
                    unsigned t = op <= 0x98 ? T_F32 : T_F64;
                    unsigned which = op <= 0x98 ? op - 0x8b : op - 0x99;
                    const char *w = t == T_F32 ? "32" : "64";
                    if (which <= 6) { /* abs neg ceil floor trunc nearest sqrt */
                        uint32_t a;
                        if (which >= 2) die("float operation 0x%02x needs libm; not supported", op);
                        a = pop(t);
                        push(t);
                        bprintf(&body, "%s = w2c_f%s_%s(%s);\n", slot(stack_height - 1), w,
                                which == 0 ? "abs" : "neg", slot_typed(t, a));
                    } else {
                        static const char *ops[4] = {"+", "-", "*", "/"};
                        uint32_t bv = pop(t), av = pop(t);
                        push(t);
                        if (which <= 10)
                            bprintf(&body, "%s = %s %s %s;\n", slot(stack_height - 1), slot_typed(t, av),
                                    ops[which - 7], slot_typed(t, bv));
                        else if (which == 13)
                            bprintf(&body, "%s = w2c_f%s_copysign(%s, %s);\n", slot(stack_height - 1), w,
                                    slot_typed(t, av), slot_typed(t, bv));
                        else
                            die("float min/max (0x%02x) not supported", op);
                    }
                    break;
                }
                /* Conversions and sign extension. */
                {
                    unsigned from = T_NONE, to = T_NONE;
                    const char *fmt = NULL;
                    switch (op) {
                        case 0xa7: from = T_I64; to = T_I32; fmt = "(uint32_t)%s"; break;
                        case 0xac: from = T_I32; to = T_I64; fmt = "(uint64_t)(%s ^ UINT32_C(0x80000000)) - UINT64_C(0x80000000)"; break;
                        case 0xad: from = T_I32; to = T_I64; fmt = "(uint64_t)%s"; break;
                        case 0xbc: from = T_F32; to = T_I32; fmt = "w2c_f32_bits(%s)"; break;
                        case 0xbd: from = T_F64; to = T_I64; fmt = "w2c_f64_bits(%s)"; break;
                        case 0xbe: from = T_I32; to = T_F32; fmt = "w2c_f32_from_bits(%s)"; break;
                        case 0xbf: from = T_I64; to = T_F64; fmt = "w2c_f64_from_bits(%s)"; break;
                        case 0xc0: from = T_I32; to = T_I32; fmt = "((%s & 0xffu) ^ 0x80u) - 0x80u"; break;
                        case 0xc1: from = T_I32; to = T_I32; fmt = "((%s & 0xffffu) ^ 0x8000u) - 0x8000u"; break;
                        case 0xc2: from = T_I64; to = T_I64; fmt = "((%s & 0xffu) ^ 0x80u) - UINT64_C(0x80)"; break;
                        case 0xc3: from = T_I64; to = T_I64; fmt = "((%s & 0xffffu) ^ 0x8000u) - UINT64_C(0x8000)"; break;
                        case 0xc4: from = T_I64; to = T_I64; fmt = "((%s & UINT64_C(0xffffffff)) ^ UINT64_C(0x80000000)) - UINT64_C(0x80000000)"; break;
                        default: break;
                    }
                    if (fmt) {
                        uint32_t a = pop(from);
                        push(to);
                        bprintf(&body, "%s = ", slot(stack_height - 1));
                        bprintf(&body, fmt, slot_typed(from, a));
                        bprintf(&body, ";\n");
                        break;
                    }
                }
                if (op >= 0xa8 && op <= 0xbb)
                    die("float/int conversion 0x%02x is not supported (SultanC MSIR has none)", op);
                if (op == 0xfc) {
                    uint32_t sub = rd_u32();
                    if (sub == 10) { /* memory.copy dst src n */
                        uint32_t n, s, d;
                        if (rd_u8() != 0 || rd_u8() != 0 || !have_memory) die("memory.copy on memory other than 0");
                        n = pop(T_I32); s = pop(T_I32); d = pop(T_I32);
                        bprintf(&body, "w2c_memory_copy(%s, %s, %s);\n", slot_typed(T_I32, d), slot_typed(T_I32, s),
                                slot_typed(T_I32, n));
                    } else if (sub == 11) { /* memory.fill dst val n */
                        uint32_t n, v, d;
                        if (rd_u8() != 0 || !have_memory) die("memory.fill on memory other than 0");
                        n = pop(T_I32); v = pop(T_I32); d = pop(T_I32);
                        bprintf(&body, "w2c_memory_fill(%s, %s, %s);\n", slot_typed(T_I32, d), slot_typed(T_I32, v),
                                slot_typed(T_I32, n));
                    } else {
                        die("unsupported 0xfc %u instruction at offset %zu", sub, at);
                    }
                    break;
                }
                die("unknown or unsupported opcode 0x%02x at offset %zu", op, at);
            }
        }
    }
    if (in_pos != funcs[idx].body_end) die("instructions after the final end");

    /* Function header: parameters become locals v0.., stack slots declared by use. */
    emit_prototype(out, idx, 1);
    bprintf(out, " {\n");
    for (i = 0; i < num_locals; i++) {
        if (i < ft->nparams)
            bprintf(out, "%s v%" PRIu32 " = p%" PRIu32 ";\n", ctype(local_types[i]), i, i);
        else
            bprintf(out, "%s v%" PRIu32 " = 0;\n", ctype(local_types[i]), i);
    }
    for (k = 0; k < 4; k++) {
        static const unsigned ts[4] = {T_I32, T_I64, T_F32, T_F64};
        if (!max_height[k]) continue;
        bprintf(out, "%s", ctype(ts[k]));
        for (i = 0; i < max_height[k]; i++) bprintf(out, "%s %c%" PRIu32, i ? "," : "", tchar(ts[k]), i);
        bprintf(out, ";\n");
    }
    buf_put(out, body.s ? body.s : "", body.n);
    bprintf(out, "}\n\n");
    free(body.s);
    current_context = "module";
}

/* ---- header and runtime ------------------------------------------------------------ */

/* Emitted at the top of the generated header, one piece per line (each under
 * the 4095-character string limit C99 guarantees). */
static const char *const runtime_helpers[] = {
    "#if defined(__GNUC__)\n",
    "#pragma GCC diagnostic ignored \"-Wpragmas\"\n",
    "#pragma GCC diagnostic ignored \"-Wunknown-warning-option\"\n",
    "#pragma GCC diagnostic ignored \"-Wunused-label\"\n",
    "#pragma GCC diagnostic ignored \"-Wunused-variable\"\n",
    "#pragma GCC diagnostic ignored \"-Wunused-but-set-variable\"\n",
    "#pragma GCC diagnostic ignored \"-Wmaybe-uninitialized\"\n",
    "#pragma GCC diagnostic ignored \"-Wuninitialized\"\n",
    "#endif\n",
    "#include <stdint.h>\n#include <stddef.h>\n#include <string.h>\n\n",
    "#define W2C_CALL_DEPTH_LIMIT " W2C_STRINGIFY(W2C_CALL_DEPTH_LIMIT) "u\n",
    "#if defined(_MSC_VER) && _MSC_VER < 1900 && !defined(inline)\n#define inline __inline /* C99 inline for older MSVC */\n#endif\n",
    "#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__\n",
    "#define W2C_BIG_ENDIAN 1\n#else\n#define W2C_BIG_ENDIAN 0\n#endif\n\n",
    "extern uint8_t *w2c_memory;\nextern uint64_t w2c_memory_size;\n",
    "/* w2c_trap never returns: every function body ends in a trap tail, and a\n"
    " * non-void one would otherwise warn (-Wreturn-type) on that path. */\n",
    "#if defined(__GNUC__) || defined(__clang__) || defined(__TINYC__)\n#define W2C_NORETURN __attribute__((noreturn))\n",
    "#elif defined(_MSC_VER)\n#define W2C_NORETURN __declspec(noreturn)\n#else\n#define W2C_NORETURN\n#endif\n",
    "W2C_NORETURN void w2c_trap(const char *why);\n",
    "uint32_t w2c_memory_pages(void);\nuint32_t w2c_memory_grow(uint32_t pages);\n",
    "void w2c_memory_copy(uint32_t dst, uint32_t src, uint32_t n);\n",
    "void w2c_memory_fill(uint32_t dst, uint32_t value, uint32_t n);\n",
    "typedef void (*w2c_funcref)(void);\n",
    "w2c_funcref w2c_indirect(uint32_t index, uint32_t type);\n\n",
    "/* Call depth and native stack bounds (see w2c_limit_native_stack). */\n",
    "extern uint32_t w2c_call_depth;\nextern uintptr_t w2c_stack_low, w2c_stack_high;\n",
    "void w2c_call_exhausted(void);\n",
    "void w2c_limit_native_stack(size_t budget);\n",
    "static inline void w2c_enter(void) {\n",
    "    char probe;\n",
    "    uintptr_t sp = (uintptr_t)&probe;\n",
    "    if (++w2c_call_depth > W2C_CALL_DEPTH_LIMIT || sp < w2c_stack_low || sp > w2c_stack_high) w2c_call_exhausted();\n}\n",
    "static inline uint8_t *w2c_addr(uint32_t a, uint32_t off, uint32_t n) {\n",
    "    uint64_t e = (uint64_t)a + off;\n",
    "    if (e + n > w2c_memory_size) w2c_trap(\"out-of-bounds memory access\");\n",
    "    return w2c_memory + e;\n}\n",
    "static inline uint16_t w2c_le16(uint16_t x) { return W2C_BIG_ENDIAN ? (uint16_t)(x << 8 | x >> 8) : x; }\n",
    "static inline uint32_t w2c_le32(uint32_t x) { return W2C_BIG_ENDIAN ? ((x & 0xffu) << 24 | (x & 0xff00u) << 8 | (x >> 8 & 0xff00u) | x >> 24) : x; }\n",
    "static inline uint64_t w2c_le64(uint64_t x) { return W2C_BIG_ENDIAN ? ((uint64_t)w2c_le32((uint32_t)x) << 32 | w2c_le32((uint32_t)(x >> 32))) : x; }\n",
    "static inline uint32_t w2c_load_i32(uint32_t a, uint32_t o) { uint32_t v; memcpy(&v, w2c_addr(a, o, 4), 4); return w2c_le32(v); }\n",
    "static inline uint64_t w2c_load_i64(uint32_t a, uint32_t o) { uint64_t v; memcpy(&v, w2c_addr(a, o, 8), 8); return w2c_le64(v); }\n",
    "static inline float w2c_f32_from_bits(uint32_t b) { float f; memcpy(&f, &b, 4); return f; }\n",
    "static inline double w2c_f64_from_bits(uint64_t b) { double d; memcpy(&d, &b, 8); return d; }\n",
    "static inline uint32_t w2c_f32_bits(float f) { uint32_t b; memcpy(&b, &f, 4); return b; }\n",
    "static inline uint64_t w2c_f64_bits(double d) { uint64_t b; memcpy(&b, &d, 8); return b; }\n",
    "static inline float w2c_load_f32(uint32_t a, uint32_t o) { return w2c_f32_from_bits(w2c_load_i32(a, o)); }\n",
    "static inline double w2c_load_f64(uint32_t a, uint32_t o) { return w2c_f64_from_bits(w2c_load_i64(a, o)); }\n",
    "static inline uint32_t w2c_load_i32_8u(uint32_t a, uint32_t o) { return *w2c_addr(a, o, 1); }\n",
    "static inline uint32_t w2c_load_i32_8s(uint32_t a, uint32_t o) { return (w2c_load_i32_8u(a, o) ^ 0x80u) - 0x80u; }\n",
    "static inline uint32_t w2c_load_i32_16u(uint32_t a, uint32_t o) { uint16_t v; memcpy(&v, w2c_addr(a, o, 2), 2); return w2c_le16(v); }\n",
    "static inline uint32_t w2c_load_i32_16s(uint32_t a, uint32_t o) { return (w2c_load_i32_16u(a, o) ^ 0x8000u) - 0x8000u; }\n",
    "static inline uint64_t w2c_load_i64_8u(uint32_t a, uint32_t o) { return w2c_load_i32_8u(a, o); }\n",
    "static inline uint64_t w2c_load_i64_8s(uint32_t a, uint32_t o) { return ((uint64_t)w2c_load_i32_8u(a, o) ^ 0x80u) - UINT64_C(0x80); }\n",
    "static inline uint64_t w2c_load_i64_16u(uint32_t a, uint32_t o) { return w2c_load_i32_16u(a, o); }\n",
    "static inline uint64_t w2c_load_i64_16s(uint32_t a, uint32_t o) { return ((uint64_t)w2c_load_i32_16u(a, o) ^ 0x8000u) - UINT64_C(0x8000); }\n",
    "static inline uint64_t w2c_load_i64_32u(uint32_t a, uint32_t o) { return w2c_load_i32(a, o); }\n",
    "static inline uint64_t w2c_load_i64_32s(uint32_t a, uint32_t o) { return ((uint64_t)w2c_load_i32(a, o) ^ UINT64_C(0x80000000)) - UINT64_C(0x80000000); }\n",
    "static inline void w2c_store_i32(uint32_t a, uint32_t o, uint32_t v) { v = w2c_le32(v); memcpy(w2c_addr(a, o, 4), &v, 4); }\n",
    "static inline void w2c_store_i64(uint32_t a, uint32_t o, uint64_t v) { v = w2c_le64(v); memcpy(w2c_addr(a, o, 8), &v, 8); }\n",
    "static inline void w2c_store_f32(uint32_t a, uint32_t o, float v) { w2c_store_i32(a, o, w2c_f32_bits(v)); }\n",
    "static inline void w2c_store_f64(uint32_t a, uint32_t o, double v) { w2c_store_i64(a, o, w2c_f64_bits(v)); }\n",
    "static inline void w2c_store_i32_8(uint32_t a, uint32_t o, uint32_t v) { *w2c_addr(a, o, 1) = (uint8_t)v; }\n",
    "static inline void w2c_store_i32_16(uint32_t a, uint32_t o, uint32_t v) { uint16_t x = w2c_le16((uint16_t)v); memcpy(w2c_addr(a, o, 2), &x, 2); }\n",
    "static inline void w2c_store_i64_8(uint32_t a, uint32_t o, uint64_t v) { w2c_store_i32_8(a, o, (uint32_t)v); }\n",
    "static inline void w2c_store_i64_16(uint32_t a, uint32_t o, uint64_t v) { w2c_store_i32_16(a, o, (uint32_t)v); }\n",
    "static inline void w2c_store_i64_32(uint32_t a, uint32_t o, uint64_t v) { w2c_store_i32(a, o, (uint32_t)v); }\n",
    /* Multiplication in unsigned arithmetic at least as wide as int: no signed overflow. */
    "static inline uint32_t w2c_mul32(uint32_t a, uint32_t b) { return (uint32_t)((uint64_t)a * b); }\n",
    "static inline uint64_t w2c_mul64(uint64_t a, uint64_t b) { return a * b; }\n",
    "static inline uint32_t w2c_div_u32(uint32_t a, uint32_t b) { if (!b) w2c_trap(\"integer divide by zero\"); return a / b; }\n",
    "static inline uint64_t w2c_div_u64(uint64_t a, uint64_t b) { if (!b) w2c_trap(\"integer divide by zero\"); return a / b; }\n",
    "static inline uint32_t w2c_rem_u32(uint32_t a, uint32_t b) { if (!b) w2c_trap(\"integer divide by zero\"); return a % b; }\n",
    "static inline uint64_t w2c_rem_u64(uint64_t a, uint64_t b) { if (!b) w2c_trap(\"integer divide by zero\"); return a % b; }\n",
    /* Signed division on magnitudes, so no signed overflow or implementation-defined conversion. */
    "static inline uint32_t w2c_div_s32(uint32_t a, uint32_t b) {\n",
    "    uint32_t na = a >> 31, nb = b >> 31, ma = na ? 0u - a : a, mb = nb ? 0u - b : b, q;\n",
    "    if (!b) w2c_trap(\"integer divide by zero\");\n",
    "    if (a == UINT32_C(0x80000000) && b == UINT32_C(0xffffffff)) w2c_trap(\"integer overflow\");\n",
    "    q = ma / mb; return (na ^ nb) ? 0u - q : q;\n}\n",
    "static inline uint64_t w2c_div_s64(uint64_t a, uint64_t b) {\n",
    "    uint64_t na = a >> 63, nb = b >> 63, ma = na ? 0u - a : a, mb = nb ? 0u - b : b, q;\n",
    "    if (!b) w2c_trap(\"integer divide by zero\");\n",
    "    if (a == UINT64_C(0x8000000000000000) && b == UINT64_C(0xffffffffffffffff)) w2c_trap(\"integer overflow\");\n",
    "    q = ma / mb; return (na ^ nb) ? 0u - q : q;\n}\n",
    "static inline uint32_t w2c_rem_s32(uint32_t a, uint32_t b) {\n",
    "    uint32_t na = a >> 31, ma = na ? 0u - a : a, mb = (b >> 31) ? 0u - b : b, r;\n",
    "    if (!b) w2c_trap(\"integer divide by zero\");\n",
    "    r = ma % mb; return na ? 0u - r : r;\n}\n",
    "static inline uint64_t w2c_rem_s64(uint64_t a, uint64_t b) {\n",
    "    uint64_t na = a >> 63, ma = na ? 0u - a : a, mb = (b >> 63) ? 0u - b : b, r;\n",
    "    if (!b) w2c_trap(\"integer divide by zero\");\n",
    "    r = ma % mb; return na ? 0u - r : r;\n}\n",
    "static inline uint32_t w2c_shr_s32(uint32_t x, uint32_t n) { uint32_t s = 0u - (x >> 31); n &= 31u; return ((x ^ s) >> n) ^ s; }\n",
    "static inline uint64_t w2c_shr_s64(uint64_t x, uint64_t n) { uint64_t s = 0u - (x >> 63); n &= 63u; return ((x ^ s) >> n) ^ s; }\n",
    "static inline uint32_t w2c_rotl32(uint32_t x, uint32_t n) { n &= 31u; return n ? (x << n) | (x >> (32u - n)) : x; }\n",
    "static inline uint32_t w2c_rotr32(uint32_t x, uint32_t n) { n &= 31u; return n ? (x >> n) | (x << (32u - n)) : x; }\n",
    "static inline uint64_t w2c_rotl64(uint64_t x, uint64_t n) { n &= 63u; return n ? (x << n) | (x >> (64u - n)) : x; }\n",
    "static inline uint64_t w2c_rotr64(uint64_t x, uint64_t n) { n &= 63u; return n ? (x >> n) | (x << (64u - n)) : x; }\n",
    "static inline uint32_t w2c_clz32(uint32_t x) { uint32_t n = 0; if (!x) return 32; while (!(x & UINT32_C(0x80000000))) { x <<= 1; n++; } return n; }\n",
    "static inline uint64_t w2c_clz64(uint64_t x) { uint64_t n = 0; if (!x) return 64; while (!(x & UINT64_C(0x8000000000000000))) { x <<= 1; n++; } return n; }\n",
    "static inline uint32_t w2c_ctz32(uint32_t x) { uint32_t n = 0; if (!x) return 32; while (!(x & 1u)) { x >>= 1; n++; } return n; }\n",
    "static inline uint64_t w2c_ctz64(uint64_t x) { uint64_t n = 0; if (!x) return 64; while (!(x & 1u)) { x >>= 1; n++; } return n; }\n",
    "static inline uint32_t w2c_popcnt32(uint32_t x) { uint32_t n = 0; while (x) { x &= x - 1; n++; } return n; }\n",
    "static inline uint64_t w2c_popcnt64(uint64_t x) { uint64_t n = 0; while (x) { x &= x - 1; n++; } return n; }\n",
    "static inline float w2c_f32_abs(float x) { return w2c_f32_from_bits(w2c_f32_bits(x) & UINT32_C(0x7fffffff)); }\n",
    "static inline float w2c_f32_neg(float x) { return w2c_f32_from_bits(w2c_f32_bits(x) ^ UINT32_C(0x80000000)); }\n",
    "static inline float w2c_f32_copysign(float x, float y) { return w2c_f32_from_bits((w2c_f32_bits(x) & UINT32_C(0x7fffffff)) | (w2c_f32_bits(y) & UINT32_C(0x80000000))); }\n",
    "static inline double w2c_f64_abs(double x) { return w2c_f64_from_bits(w2c_f64_bits(x) & UINT64_C(0x7fffffffffffffff)); }\n",
    "static inline double w2c_f64_neg(double x) { return w2c_f64_from_bits(w2c_f64_bits(x) ^ UINT64_C(0x8000000000000000)); }\n",
    "static inline double w2c_f64_copysign(double x, double y) { return w2c_f64_from_bits((w2c_f64_bits(x) & UINT64_C(0x7fffffffffffffff)) | (w2c_f64_bits(y) & UINT64_C(0x8000000000000000))); }\n",
    NULL};

static void emit_header(const char *prefix) {
    Buf b = {0};
    FILE *f = open_out(prefix, ".h");
    uint32_t i;
    bprintf(&b, "/* Generated by sultanc-wasm2c from the SultanC WASI seed. Do not edit. */\n");
    bprintf(&b, "#ifndef SULTANC_WASM2C_H\n#define SULTANC_WASM2C_H\n");
    for (i = 0; runtime_helpers[i]; i++) buf_put(&b, runtime_helpers[i], strlen(runtime_helpers[i]));
    bprintf(&b, "\n/* Globals. */\n");
    for (i = 0; i < num_globals; i++) bprintf(&b, "extern %s w2c_g%" PRIu32 ";\n", ctype(globals[i].type), i);
    bprintf(&b, "\n/* Functions (imports are provided by wasi.c). */\n");
    for (i = 0; i < num_funcs; i++) {
        emit_prototype(&b, i, 0);
        bprintf(&b, ";\n");
    }
    bprintf(&b, "\nvoid w2c_run(void);\n#endif\n");
    write_out(f, &b);
    close_out(f);
    free(b.s);
}

static void emit_runtime(const char *prefix) {
    Buf b = {0};
    FILE *f = open_out(prefix, "-runtime.c");
    uint32_t i, j;
    bprintf(&b, "/* Generated by sultanc-wasm2c. Do not edit. */\n");
    bprintf(&b, "#include \"%s.h\"\n", strrchr(prefix, '/') ? strrchr(prefix, '/') + 1 : prefix);
    bprintf(&b, "#include <stdio.h>\n#include <stdlib.h>\n\n");
    bprintf(&b, "uint8_t *w2c_memory;\nuint64_t w2c_memory_size;\n");
    bprintf(&b, "static const uint32_t w2c_memory_max_pages = %" PRIu32 "u;\n",
            memory_has_max ? memory_max : 65536u);
    for (i = 0; i < num_globals; i++) {
        switch (globals[i].type) {
            case T_I32: bprintf(&b, "uint32_t w2c_g%" PRIu32 " = UINT32_C(0x%08" PRIx32 ");\n", i, (uint32_t)globals[i].init_bits); break;
            case T_I64: bprintf(&b, "uint64_t w2c_g%" PRIu32 " = UINT64_C(0x%016" PRIx64 ");\n", i, globals[i].init_bits); break;
            /* Float globals: initialized from bits at startup (constant-expression rules). */
            case T_F32: bprintf(&b, "float w2c_g%" PRIu32 ";\n", i); break;
            case T_F64: bprintf(&b, "double w2c_g%" PRIu32 ";\n", i); break;
        }
    }
    bprintf(&b,
            "\nW2C_NORETURN void w2c_trap(const char *why) {\n"
            "    fflush(stdout);\n"
            "    fprintf(stderr, \"sultanc (wasm bootstrap): trap: %%s\\n\", why);\n"
            "    abort();\n}\n"
            "uint32_t w2c_call_depth;\n"
            "uintptr_t w2c_stack_low = 0, w2c_stack_high = UINTPTR_MAX;\n"
            "void w2c_call_exhausted(void) {\n"
            "    if (w2c_call_depth > W2C_CALL_DEPTH_LIMIT)\n"
            "        w2c_trap(\"call stack exhausted (more than " W2C_STRINGIFY(W2C_CALL_DEPTH_LIMIT) " nested calls)\");\n"
            "    w2c_trap(\"call stack exhausted (native stack)\");\n}\n"
            "/* The host calls this on the thread that runs w2c_run, with the stack it\n"
            " * may use from here (bytes). The check keeps a reserve below the bound for\n"
            " * the frame of the callee, the WASI host functions and the trap report. */\n"
            "void w2c_limit_native_stack(size_t budget) {\n"
            "    char here;\n"
            "    uintptr_t base = (uintptr_t)&here, reserve = budget / 8 < 1048576u ? budget / 8 : 1048576u;\n"
            "    uintptr_t room = budget > reserve ? (uintptr_t)(budget - reserve) : 0;\n"
            "    w2c_stack_low = base > room ? base - room : 0;\n"
            "    w2c_stack_high = UINTPTR_MAX - base > room ? base + room : UINTPTR_MAX;\n}\n"
            "uint32_t w2c_memory_pages(void) { return (uint32_t)(w2c_memory_size / 65536u); }\n"
            "uint32_t w2c_memory_grow(uint32_t pages) {\n"
            "    uint64_t old = w2c_memory_size / 65536u, next = old + pages;\n"
            "    uint8_t *p;\n"
            "    if (next > w2c_memory_max_pages || next > 65536u) return UINT32_C(0xffffffff);\n"
            "    if (!pages) return (uint32_t)old;\n"
            "    if ((uint64_t)(size_t)(next * 65536u) != next * 65536u) return UINT32_C(0xffffffff);\n"
            "    p = (uint8_t *)realloc(w2c_memory, (size_t)(next * 65536u));\n"
            "    if (!p) return UINT32_C(0xffffffff);\n"
            "    memset(p + w2c_memory_size, 0, (size_t)(pages * UINT64_C(65536)));\n"
            "    w2c_memory = p;\n    w2c_memory_size = next * 65536u;\n"
            "    return (uint32_t)old;\n}\n"
            "void w2c_memory_copy(uint32_t d, uint32_t s, uint32_t n) {\n"
            "    if ((uint64_t)d + n > w2c_memory_size || (uint64_t)s + n > w2c_memory_size) w2c_trap(\"out-of-bounds memory.copy\");\n"
            "    if (n) memmove(w2c_memory + d, w2c_memory + s, n);\n}\n"
            "void w2c_memory_fill(uint32_t d, uint32_t v, uint32_t n) {\n"
            "    if ((uint64_t)d + n > w2c_memory_size) w2c_trap(\"out-of-bounds memory.fill\");\n"
            "    if (n) memset(w2c_memory + d, (int)(v & 0xffu), n);\n}\n");

    /* Indirect-call table: function pointers and canonical type ids. */
    if (have_table) {
        uint32_t *entries = xcalloc(table_min ? table_min : 1, sizeof(uint32_t));
        for (i = 0; i < num_elems; i++)
            for (j = 0; j < elems[i].count; j++) entries[elems[i].offset + j] = elems[i].funcs[j] + 1;
        bprintf(&b, "\nstatic const struct { w2c_funcref fn; uint32_t type; } w2c_table[%" PRIu32 "] = {\n",
                table_min ? table_min : 1);
        for (i = 0; i < (table_min ? table_min : 1); i++) {
            if (i < table_min && entries[i]) {
                char cname[160];
                func_c_name(entries[i] - 1, cname, sizeof(cname));
                bprintf(&b, "{(w2c_funcref)%s, %" PRIu32 "u},\n", cname, types[funcs[entries[i] - 1].type].canon);
            } else {
                bprintf(&b, "{0, 0},\n");
            }
        }
        bprintf(&b, "};\n");
        bprintf(&b,
                "w2c_funcref w2c_indirect(uint32_t index, uint32_t type) {\n"
                "    if (index >= %" PRIu32 "u || !w2c_table[index].fn) w2c_trap(\"indirect call to null or out-of-range table entry\");\n"
                "    if (w2c_table[index].type != type) w2c_trap(\"indirect call signature mismatch\");\n"
                "    return w2c_table[index].fn;\n}\n",
                table_min);
        free(entries);
    } else {
        bprintf(&b, "w2c_funcref w2c_indirect(uint32_t index, uint32_t type) { (void)index; (void)type; w2c_trap(\"no table\"); return 0; }\n");
    }

    /* Data segments. */
    for (i = 0; i < num_datas; i++) {
        bprintf(&b, "\nstatic const unsigned char w2c_data%" PRIu32 "[%" PRIu32 "] = {", i,
                datas[i].size ? datas[i].size : 1);
        for (j = 0; j < datas[i].size; j++) {
            if (j % 24 == 0) bprintf(&b, "\n");
            bprintf(&b, "%u,", in_bytes[datas[i].pos + j]);
        }
        if (!datas[i].size) bprintf(&b, "0");
        bprintf(&b, "\n};\n");
    }

    bprintf(&b, "\nvoid w2c_run(void) {\n");
    bprintf(&b, "    {\n        const uint32_t one = 1;\n        unsigned char first;\n"
                "        memcpy(&first, &one, 1);\n"
                "        if ((first == 1) == (W2C_BIG_ENDIAN != 0)) w2c_trap(\"host byte order detection disagrees with W2C_BIG_ENDIAN\");\n    }\n");
    if (have_memory) {
        bprintf(&b, "    w2c_memory_size = (uint64_t)%" PRIu32 "u * 65536u;\n", memory_min);
        bprintf(&b, "    w2c_memory = (uint8_t *)calloc(1, (size_t)(w2c_memory_size ? w2c_memory_size : 1));\n");
        bprintf(&b, "    if (!w2c_memory) w2c_trap(\"cannot allocate linear memory\");\n");
    }
    for (i = 0; i < num_globals; i++) {
        if (globals[i].type == T_F32)
            bprintf(&b, "    w2c_g%" PRIu32 " = w2c_f32_from_bits(UINT32_C(0x%08" PRIx32 "));\n", i, (uint32_t)globals[i].init_bits);
        else if (globals[i].type == T_F64)
            bprintf(&b, "    w2c_g%" PRIu32 " = w2c_f64_from_bits(UINT64_C(0x%016" PRIx64 "));\n", i, globals[i].init_bits);
    }
    for (i = 0; i < num_datas; i++)
        if (datas[i].size)
            bprintf(&b, "    memcpy(w2c_memory + %" PRIu32 "u, w2c_data%" PRIu32 ", %" PRIu32 "u);\n", datas[i].offset, i,
                    datas[i].size);
    if (have_start) {
        char cname[160];
        func_c_name(start_func, cname, sizeof(cname));
        bprintf(&b, "    %s();\n", cname);
    }
    {
        char cname[160];
        func_c_name(entry_func, cname, sizeof(cname));
        bprintf(&b, "    %s();\n", cname);
    }
    bprintf(&b, "}\n");
    write_out(f, &b);
    close_out(f);
    free(b.s);
}

static void emit_shards(const char *prefix, unsigned shards) {
    /* Contiguous function ranges balanced by body size: deterministic. */
    uint64_t total = 0, acc = 0;
    uint32_t i = num_imported;
    unsigned s;
    const char *base = strrchr(prefix, '/') ? strrchr(prefix, '/') + 1 : prefix;
    for (s = 0; s < num_funcs; s++)
        if (!funcs[s].imported) total += funcs[s].body_end - funcs[s].body_start;
    for (s = 0; s < shards; s++) {
        char suffix[32];
        Buf b = {0};
        FILE *f;
        uint64_t limit = total * (s + 1) / shards;
        snprintf(suffix, sizeof(suffix), "-%03u.c", s);
        f = open_out(prefix, suffix);
        bprintf(&b, "/* Generated by sultanc-wasm2c: translation unit %u of %u. Do not edit. */\n", s + 1, shards);
        bprintf(&b, "#include \"%s.h\"\n\n", base);
        while (i < num_funcs && (acc < limit || s == shards - 1)) {
            acc += funcs[i].body_end - funcs[i].body_start;
            translate_body(&b, i);
            i++;
        }
        write_out(f, &b);
        close_out(f);
        free(b.s);
    }
    if (i != num_funcs) die("internal: not every function was emitted");
}

/* Reads a whole file. A Zstandard-compressed file is decompressed when
 * `decompress` is set. */
static unsigned char *load_file(const char *path, size_t *len, int decompress) {
    FILE *f = fopen(path, "rb");
    long size = 0;
    unsigned char *bytes;
    if (!f) die("cannot open %s", path);
    if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0)
        die("cannot determine size of %s", path);
    if ((unsigned long)size > 512ul * 1024 * 1024) die("%s is larger than 512 MiB", path);
    bytes = xmalloc((size_t)size);
    if (fread(bytes, 1, (size_t)size, f) != (size_t)size) die("cannot read %s", path);
    fclose(f);
    *len = (size_t)size;
    if (decompress && zstd_is_compressed(bytes, *len)) {
        unsigned char *out;
        size_t out_len;
        char err[200];
        if (!zstd_decompress(bytes, *len, &out, &out_len, err, sizeof(err)))
            die("cannot decompress %s: %s", path, err);
        free(bytes);
        bytes = out;
        *len = out_len;
    }
    return bytes;
}

int main(int argc, char **argv) {
    const char *seed = NULL, *prefix = NULL;
    unsigned shards = 64;
    int i;
    size_t size;
    unsigned char *bytes;

    /* Utility mode for bootstrap.c and the seed check. */
    if (argc == 4 && strcmp(argv[1], "--decompress") == 0) {
        FILE *out;
        bytes = load_file(argv[2], &size, 1);
        out = fopen(argv[3], "wb");
        if (!out || (size && fwrite(bytes, 1, size, out) != size) || fclose(out) != 0)
            die("cannot write %s", argv[3]);
        return 0;
    }
    for (i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--shards=", 9) == 0) {
            shards = (unsigned)strtoul(argv[i] + 9, NULL, 10);
            if (shards < 1 || shards > 999) die("--shards must be 1..999");
        } else if (!seed) seed = argv[i];
        else if (!prefix) prefix = argv[i];
        else die("unexpected argument %s", argv[i]);
    }
    if (!seed || !prefix) {
        fprintf(stderr, "usage: sultanc-wasm2c [--shards=N] SEED.wasm[.zst] OUT_PREFIX\n"
                        "       sultanc-wasm2c --decompress FILE.zst OUT\n");
        return 2;
    }
    bytes = load_file(seed, &size, 1);
    in_bytes = bytes;
    in_size = size;
    in_pos = 0;
    parse_module();
    emit_header(prefix);
    emit_runtime(prefix);
    emit_shards(prefix, shards);
    fprintf(stderr, "sultanc-wasm2c: %u functions (%u imported), %u types, %u globals, %u data segments -> %u units\n",
            num_funcs, num_imported, num_types, num_globals, num_datas, shards);
    return 0;
}
