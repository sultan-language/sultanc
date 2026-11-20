/* funcgen.c: one WebAssembly function body to C.
 *
 * The operand stack is resolved statically: validation makes the height and
 * type of every stack value known at each instruction. The value at height h
 * with type T has a C local, its slot `<T>h` (i0, l3, f2, d5 ...), and a
 * block's result always goes to the slot at the block's entry height.
 *
 * A value is not written to its slot until it has to be. A constant, a local
 * read or an operation on such values stays a C expression on the stack and
 * becomes part of the instruction that consumes it, so
 *     local.get 3; i32.const 16; i32.add; local.set 4
 * is the single statement `v4 = v3 + 16u;`. The rules that keep this exact:
 *   - An expression that reads local L is written to its slot before L
 *     changes (local.set, local.tee, or a call result stored to L).
 *   - An expression that can trap or reads memory or a global ("impure":
 *     loads, global.get, division) is evaluated before any later side effect
 *     (store, call, global.set, memory operations, branches), and at most one
 *     is pending at a time, so traps happen in the order the code gives.
 *   - Every pending expression that reads a local, a slot, memory or a
 *     global is written to its slot when a block, loop or if starts, so the
 *     value is the same on every path through it.
 *   - Float arithmetic is one operation per statement (no contraction or
 *     excess precision across WebAssembly instructions).
 * Integer operations and conversions on constants are folded (not division,
 * which can trap).
 * Comparisons are C int expressions (0 or 1); they are used as conditions
 * directly, and converted with (uint32_t) where they meet other operators.
 *
 * Control flow: a block is a label after its end (if a branch targets it), a
 * loop is a labeled compound statement, and branches are gotos. An if is C's
 * if/else, with three shapes that keep nesting shallow: an else-branch that is
 * only another if is `else if`; a then-branch that never falls through is
 * closed before the else-branch, which follows unnested; and an else-branch
 * that never falls through and is the shorter one is written first, as a
 * guard `if (!cond) { ... }`, with the then-branch after it. Indentation
 * shows the ifs and loops. A first pass over the body finds the targeted loop
 * labels and the else-if and guard shapes; the second writes the C. Every
 * translated function counts its frame (w2c_enter, w2c_leave in w2c.h).
 *
 * Every instruction is type-checked as it is translated, and anything
 * unexpected stops the translation with a diagnostic, so a malformed seed or
 * a translator bug cannot produce C that silently computes something else.
 */
#include "funcgen.h"

#include <stdlib.h>
#include <string.h>

#include "module.h"
#include "stream.h"

#define INDENT_WIDTH 4
/* Nesting deeper than this is written at this depth (large switches nest
 * one block per case). */
#define MAX_INDENT 12
/* A longer expression goes to its slot instead of growing. */
#define MAX_TEXT 96
/* Declarations wrap after this column. */
#define WRAP_COLUMN 100

/* ---- names ----------------------------------------------------------------------- */

void func_c_name(uint32_t idx, char *buf, size_t cap) {
    if (funcs[idx].imported)
        snprintf(buf, cap, "wasi_snapshot_preview1_%s", funcs[idx].import_name);
    else
        snprintf(buf, cap, "w2c_f%" PRIu32, idx);
}

void emit_prototype(Buf *b, uint32_t idx, int named) {
    const FuncType *t = &types[funcs[idx].type];
    char name[160];
    uint32_t i;
    func_c_name(idx, name, sizeof(name));
    bprintf(b, "%s %s(", ctype(t->result), name);
    if (!t->nparams) buf_puts(b, "void");
    for (i = 0; i < t->nparams; i++) {
        if (named)
            bprintf(b, "%s%s v%" PRIu32, i ? ", " : "", ctype(t->params[i]), i);
        else
            bprintf(b, "%s%s", i ? ", " : "", ctype(t->params[i]));
    }
    buf_puts(b, ")");
}

void emit_fnptr_type(Buf *b, uint32_t type) {
    const FuncType *t = &types[type];
    uint32_t i;
    bprintf(b, "%s (*)(", ctype(t->result));
    if (!t->nparams) buf_puts(b, "void");
    for (i = 0; i < t->nparams; i++) bprintf(b, "%s%s", i ? ", " : "", ctype(t->params[i]));
    buf_puts(b, ")");
}

/* ---- translation state ---------------------------------------------------------- */

enum { V_SLOT, V_PURE, V_IMPURE };

/* A value on the operand stack. */
typedef struct {
    unsigned char type;
    unsigned char kind;      /* V_SLOT: in its slot; otherwise `text` is pending */
    unsigned char atom;      /* `text` needs no parentheses as an operand */
    unsigned char is_bool;   /* `text` is a C comparison (an int, 0 or 1) */
    unsigned char reads_local;
    unsigned char reads_slot; /* `text` reads this value's own slot */
    unsigned char is_const;
    uint64_t bits; /* is_const: the value */
    char *text;
    char *negated; /* is_bool: the opposite comparison */
} Val;

enum { K_FUNC, K_BLOCK, K_LOOP, K_IF };

typedef struct {
    int kind;
    uint32_t id;
    uint32_t height;   /* operand stack height at entry */
    unsigned result;   /* T_NONE or a value type */
    int live_at_entry; /* was the instruction that opened it reachable */
    int had_else;
    int then_live; /* if: the then-branch reaches its end */
    int used;      /* a branch targets its label */
    int braced;    /* its C compound statement is open */
    int chained;   /* if: written as `else if` of its parent's else-branch */
    int flat_else; /* if: the then-branch never falls through, so the
                    * else-branch follows the if unnested */
    size_t then_pos;   /* if: output position after `if (...) {` */
    size_t then_size;  /* if: output size of the then-branch */
    size_t else_start; /* if: output position where the else-branch starts */
    /* if: the else-branch never falls through, so it is written first as a
     * guard, `if (!cond) { else-branch }`, and the then-branch follows. */
    int guard;
    char *guard_cond;
    Buf *then_buf;   /* the then-branch, written after the guard */
    Buf *saved_body; /* the output the guard goes to */
    /* First pass, for `else if`: */
    int chain_candidate;   /* if: opened first thing in an else-branch, nothing written before it */
    size_t else_pos;       /* if: output position after `} else {` */
    uint32_t last_child;   /* the control entry that ended last inside it (UINT32_MAX: none) */
    size_t last_child_end; /* output position after that one's end */
} Ctl;

static const FuncType *ft;
static Val *stack;
static uint32_t height, stack_cap;
static Ctl *ctls;
static uint32_t ctl_depth, ctl_cap;
static int live;
static unsigned char *local_types, *local_used;
static uint32_t num_locals;
static unsigned char *slot_used[4]; /* per type, per height */
static uint32_t next_label;
/* From the first pass, by control id: */
#define F_LOOP_USED 1 /* a branch targets the loop */
#define F_ELSE_IF 2   /* the if is the only content of its parent's else-branch */
#define F_GUARD 4     /* the else-branch does not fall through and is the shorter */
static unsigned char *ctl_flags;
static uint32_t ctl_flags_cap;
static int final_pass;
static Buf *body;
static unsigned indent;

static int type_index(unsigned t) {
    switch (t) {
        case T_I32: return 0;
        case T_I64: return 1;
        case T_F32: return 2;
        case T_F64: return 3;
    }
    die("internal: slot type");
    return 0;
}

/* One statement line at the current indentation. */
static void line(const char *fmt, ...) W2C_PRINTF(1, 2);
static void line(const char *fmt, ...) {
    char tmp[512];
    int n;
    va_list ap;
    buf_spaces(body, (size_t)(indent < MAX_INDENT ? indent : MAX_INDENT) * INDENT_WIDTH);
    va_start(ap, fmt);
    n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0) die("internal: format failure");
    if ((size_t)n < sizeof(tmp)) {
        buf_put(body, tmp, (size_t)n);
    } else {
        char *big = xmalloc((size_t)n + 1);
        va_start(ap, fmt);
        vsnprintf(big, (size_t)n + 1, fmt, ap);
        va_end(ap);
        buf_put(body, big, (size_t)n);
        free(big);
    }
    buf_put(body, "\n", 1);
}

/* Room for a value at height h. */
static void reserve_height(uint32_t h) {
    uint32_t k, cap;
    if (h < stack_cap) return;
    cap = stack_cap ? stack_cap : 64;
    while (cap <= h) cap *= 2;
    stack = xrealloc(stack, (size_t)cap * sizeof(*stack));
    for (k = 0; k < 4; k++) {
        slot_used[k] = xrealloc(slot_used[k], cap);
        memset(slot_used[k] + stack_cap, 0, cap - stack_cap);
    }
    stack_cap = cap;
}

static void grow_stack(void) { reserve_height(height); }

/* The slot of type t at height h, as a new string. */
static char *slot_name(unsigned t, uint32_t h) {
    reserve_height(h);
    slot_used[type_index(t)][h] = 1;
    return xprintf("%c%" PRIu32, tchar(t), h);
}

static void val_clear(Val *v) {
    free(v->text);
    free(v->negated);
    v->text = v->negated = NULL;
}

static void truncate_stack(uint32_t h) {
    while (height > h) val_clear(&stack[--height]);
}

/* ---- the operand stack ------------------------------------------------------------ */

static void push_slot(unsigned t) {
    Val *v;
    grow_stack();
    v = &stack[height++];
    memset(v, 0, sizeof(*v));
    v->type = (unsigned char)t;
    v->kind = V_SLOT;
    v->atom = 1;
}

/* Writes the value at height h to its slot. */
static void materialize(uint32_t h) {
    Val *v = &stack[h];
    char *s;
    if (v->kind == V_SLOT) return;
    s = slot_name(v->type, h);
    line("%s = %s;", s, v->text);
    free(s);
    val_clear(v);
    v->kind = V_SLOT;
    v->atom = 1;
    v->is_bool = v->reads_local = v->reads_slot = v->is_const = 0;
}

/* Evaluates the pending impure value, if any, so a side effect can follow. */
static void flush_impure(uint32_t below) {
    uint32_t h;
    for (h = 0; h < below; h++)
        if (stack[h].kind == V_IMPURE) materialize(h);
}

/* Before a block, loop or if: every pending value that is not a constant. */
static void flush_for_block(void) {
    uint32_t h;
    for (h = 0; h < height; h++) {
        Val *v = &stack[h];
        if (v->kind == V_IMPURE || (v->kind == V_PURE && (v->reads_local || v->reads_slot))) materialize(h);
    }
}

/* Does `s` read local `l` (the token v<l>)? */
static int text_reads_local(const char *s, uint32_t l) {
    const char *p;
    for (p = s; (p = strchr(p, 'v')) != NULL; p++) {
        char *end;
        unsigned long n;
        if (p > s && (p[-1] == '_' || (p[-1] >= 'a' && p[-1] <= 'z') || (p[-1] >= 'A' && p[-1] <= 'Z') ||
                      (p[-1] >= '0' && p[-1] <= '9')))
            continue;
        if (p[1] < '0' || p[1] > '9') continue;
        n = strtoul(p + 1, &end, 10);
        if (n == l) return 1;
    }
    return 0;
}

/* Before local l changes: every pending value below `below` that reads it. */
static void flush_local(uint32_t l, uint32_t below) {
    uint32_t h;
    for (h = 0; h < below; h++)
        if (stack[h].kind != V_SLOT && stack[h].reads_local && text_reads_local(stack[h].text, l)) materialize(h);
}

static Val pop_val(unsigned expect) {
    Val v;
    if (height <= ctls[ctl_depth - 1].height) die("operand stack underflow");
    v = stack[height - 1];
    if (expect != T_NONE && v.type != expect) die("type mismatch: expected %s, found %s", tname(expect), tname(v.type));
    height--;
    return v;
}

static unsigned top_type(void) {
    if (height <= ctls[ctl_depth - 1].height) die("operand stack underflow");
    return stack[height - 1].type;
}

/* The text of value v (at height h) as a function argument, assignment
 * source or return value. */
static char *arg_text(const Val *v, uint32_t h) {
    return v->kind == V_SLOT ? slot_name(v->type, h) : xstrdup(v->text);
}

/* As an operand of a C operator: parenthesized unless atomic, and a
 * comparison converted to uint32_t. */
static char *operand_text(const Val *v, uint32_t h) {
    if (v->kind == V_SLOT) return slot_name(v->type, h);
    if (v->is_bool) return xprintf("(uint32_t)(%s)", v->text);
    return v->atom ? xstrdup(v->text) : xprintf("(%s)", v->text);
}

/* As a condition. */
static char *cond_text(const Val *v, uint32_t h) { return arg_text(v, h); }

static int kind_of(int a, int b) { return a == V_IMPURE || b == V_IMPURE ? V_IMPURE : V_PURE; }

/* Pushes a computed value. `bottom` is the operand at the result's own
 * height (or NULL), whose slot the text may read. `upper_slot` says the text
 * reads a slot above the result; such a value, or a long one, goes to its
 * slot at once. Takes ownership of `text`. */
static void push_expr(unsigned t, int kind, int atom, char *text, const Val *bottom, int reads_local, int upper_slot) {
    Val *v;
    if (kind == V_IMPURE) flush_impure(height); /* an older pending value is evaluated first */
    grow_stack();
    v = &stack[height++];
    memset(v, 0, sizeof(*v));
    v->type = (unsigned char)t;
    v->kind = (unsigned char)kind;
    v->atom = (unsigned char)atom;
    v->text = text;
    v->reads_local = (unsigned char)reads_local;
    v->reads_slot = (unsigned char)(bottom && (bottom->kind == V_SLOT || bottom->reads_slot));
    if (upper_slot || strlen(text) > MAX_TEXT) materialize(height - 1);
}

static int reads_upper_slot(const Val *v) { return v->kind == V_SLOT || v->reads_slot; }

static void push_bool(char *text, char *negated, int kind, const Val *bottom, int reads_local, int upper_slot) {
    push_expr(T_I32, kind, 0, text, bottom, reads_local, upper_slot);
    if (stack[height - 1].kind == V_SLOT) {
        free(negated);
    } else {
        stack[height - 1].is_bool = 1;
        stack[height - 1].negated = negated;
    }
}

static char *const_text(unsigned t, uint64_t bits) {
    if (t == T_I32) {
        uint32_t v = (uint32_t)bits;
        return v <= 0xffffu ? xprintf("%" PRIu32 "u", v) : xprintf("0x%" PRIx32 "u", v);
    }
    return bits <= 0xffffu ? xprintf("UINT64_C(%" PRIu64 ")", bits) : xprintf("UINT64_C(0x%" PRIx64 ")", bits);
}

static void push_const(unsigned t, uint64_t bits) {
    char *text;
    switch (t) {
        case T_I32:
        case T_I64: text = const_text(t, bits); break;
        case T_F32: text = xprintf("w2c_f32_from_bits(0x%08" PRIx32 "u)", (uint32_t)bits); break;
        default: text = xprintf("w2c_f64_from_bits(UINT64_C(0x%016" PRIx64 "))", bits); break;
    }
    push_expr(t, V_PURE, 1, text, NULL, 0, 0);
    if (stack[height - 1].kind != V_SLOT) {
        stack[height - 1].is_const = 1;
        stack[height - 1].bits = bits;
    }
}

static void finish_pop(Val *v) { val_clear(v); }

/* ---- control ----------------------------------------------------------------------- */

static void push_ctl(int kind, unsigned result) {
    Ctl *c;
    if (ctl_depth == ctl_cap) {
        ctl_cap = ctl_cap ? ctl_cap * 2 : 32;
        ctls = xrealloc(ctls, (size_t)ctl_cap * sizeof(*ctls));
    }
    c = &ctls[ctl_depth++];
    memset(c, 0, sizeof(*c));
    c->kind = kind;
    c->id = next_label++;
    c->height = height;
    c->result = result;
    c->live_at_entry = live;
    c->last_child = UINT32_MAX;
}

static unsigned read_block_type(void) {
    int64_t bt = rd_s33();
    if (bt == -64) return T_NONE; /* 0x40 empty */
    if (bt == -1) return T_I32;   /* 0x7f */
    if (bt == -2) return T_I64;   /* 0x7e */
    if (bt == -3) return T_F32;   /* 0x7d */
    if (bt == -4) return T_F64;   /* 0x7c */
    if (bt >= 0 && (uint64_t)bt < num_types) {
        const FuncType *t = &types[bt];
        if (t->nparams) die("block types with parameters are not supported");
        return t->result;
    }
    die("unsupported block type %" PRId64, bt);
    return T_NONE;
}

static void set_flag(uint32_t id, unsigned flag) {
    if (final_pass) return;
    if (id >= ctl_flags_cap) {
        uint32_t cap = ctl_flags_cap ? ctl_flags_cap : 256;
        while (cap <= id) cap *= 2;
        ctl_flags = xrealloc(ctl_flags, cap);
        memset(ctl_flags + ctl_flags_cap, 0, cap - ctl_flags_cap);
        ctl_flags_cap = cap;
    }
    ctl_flags[id] |= (unsigned char)flag;
}

static int has_flag(uint32_t id, unsigned flag) { return id < ctl_flags_cap && (ctl_flags[id] & flag); }

/* The statements of a branch to control entry `target` (0 = the function).
 * A block result (the value on top) is first copied to the slot at the
 * target's entry height. Writes them to `out` separated by spaces. */
static void branch_text(Buf *out, uint32_t target) {
    Ctl *c = &ctls[target];
    if (c->kind == K_FUNC) {
        if (c->result != T_NONE) {
            char *v;
            if (top_type() != c->result) die("type mismatch at function-level branch");
            v = arg_text(&stack[height - 1], height - 1);
            bprintf(out, "w2c_leave(); return %s;", v);
            free(v);
        } else {
            buf_puts(out, "w2c_leave(); return;");
        }
        return;
    }
    c->used = 1;
    if (c->kind == K_LOOP) {
        set_flag(c->id, F_LOOP_USED);
        bprintf(out, "goto L%" PRIu32 ";", c->id);
        return;
    }
    if (c->result != T_NONE) {
        if (top_type() != c->result) die("branch value type mismatch");
        if (height - 1 != c->height || stack[height - 1].kind != V_SLOT) {
            char *dst = slot_name(c->result, c->height), *v = arg_text(&stack[height - 1], height - 1);
            bprintf(out, "%s = %s; ", dst, v);
            free(dst);
            free(v);
        }
    }
    bprintf(out, "goto B%" PRIu32 ";", c->id);
}

/* An unconditional branch as one statement per line. */
static void emit_branch(uint32_t target) {
    Buf b = {0};
    char *p, *q;
    branch_text(&b, target);
    /* The parts are "x = y;", "w2c_leave();", "return ...;" or "goto ...;". */
    for (p = b.s; *p; p = q) {
        q = strstr(p, "; ");
        if (q) {
            *q = 0;
            line("%s;", p);
            q += 2;
        } else {
            line("%s", p);
            break;
        }
    }
    buf_free(&b);
}

/* `if (cond) branch` on one line. */
static void emit_cond_branch(const char *cond, uint32_t target) {
    Buf b = {0};
    branch_text(&b, target);
    if (strchr(b.s, ';') != b.s + b.n - 1)
        line("if (%s) { %s }", cond, b.s);
    else
        line("if (%s) %s", cond, b.s);
    buf_free(&b);
}

static uint32_t memarg(uint32_t max_align_log2) {
    uint32_t align = rd_u32();
    uint32_t offset = rd_u32();
    if (align > max_align_log2) die("memory access alignment larger than natural");
    if (!have_memory) die("memory access without a memory");
    return offset;
}

/* ---- instructions ------------------------------------------------------------------- */

static void op_local_set(uint32_t l, int tee) {
    char *v, *self;
    uint32_t hv;
    if (l >= num_locals) die("local index out of range");
    local_used[l] = 1;
    self = xprintf("v%" PRIu32, l);
    if (tee) {
        Val *top;
        if (top_type() != local_types[l]) die("local.tee type mismatch");
        hv = height - 1;
        flush_local(l, hv);
        top = &stack[hv];
        v = arg_text(top, hv);
        if (strcmp(v, self) != 0) line("%s = %s;", self, v);
        free(v);
        /* The value is now in the local. */
        val_clear(top);
        top->kind = V_PURE;
        top->atom = 1;
        top->is_bool = 0;
        top->reads_local = 1;
        top->reads_slot = 0;
        top->text = self;
        return;
    } else {
        Val x = pop_val(local_types[l]);
        hv = height;
        flush_local(l, height);
        v = arg_text(&x, hv);
        if (strcmp(v, self) != 0) line("%s = %s;", self, v);
        free(v);
        finish_pop(&x);
    }
    free(self);
}

static void op_compare(unsigned t, const char *op, const char *negop, int is_signed) {
    Val b = pop_val(t), a;
    uint32_t hb = height, ha;
    char *x, *y;
    a = pop_val(t);
    ha = height;
    if (is_signed) {
        /* Signed order by flipping the sign bit: no implementation-defined
         * conversions. */
        uint64_t sign = t == T_I32 ? UINT64_C(0x80000000) : UINT64_C(0x8000000000000000);
        char *ax = operand_text(&a, ha), *by = operand_text(&b, hb);
        const char *k = t == T_I32 ? "0x80000000u" : "UINT64_C(0x8000000000000000)";
        x = a.kind != V_SLOT && a.is_const ? const_text(t, a.bits ^ sign) : xprintf("(%s ^ %s)", ax, k);
        y = b.kind != V_SLOT && b.is_const ? const_text(t, b.bits ^ sign) : xprintf("(%s ^ %s)", by, k);
        free(ax);
        free(by);
    } else {
        x = operand_text(&a, ha);
        y = operand_text(&b, hb);
    }
    if (t == T_F32 || t == T_F64)
        push_bool(xprintf("%s %s %s", x, op, y), xprintf("!(%s %s %s)", x, op, y), kind_of(a.kind, b.kind), &a,
                  a.reads_local || b.reads_local, reads_upper_slot(&b));
    else
        push_bool(xprintf("%s %s %s", x, op, y), xprintf("%s %s %s", x, negop, y), kind_of(a.kind, b.kind), &a,
                  a.reads_local || b.reads_local, reads_upper_slot(&b));
    free(x);
    free(y);
    finish_pop(&a);
    finish_pop(&b);
}

static void op_eqz(unsigned t) {
    Val a = pop_val(t);
    uint32_t ha = height;
    if (a.kind != V_SLOT && a.is_bool) {
        char *text = a.negated, *neg = a.text;
        a.text = a.negated = NULL;
        push_bool(text, neg, a.kind, &a, a.reads_local, 0);
    } else {
        char *x = operand_text(&a, ha);
        push_bool(xprintf("%s == 0", x), xprintf("%s != 0", x), kind_of(a.kind, V_PURE), &a, a.reads_local, 0);
        free(x);
    }
    finish_pop(&a);
}

static void op_int_binary(unsigned t, unsigned which) {
    static const char *const calls[15] = {NULL,    NULL,    "mul",  "div_s", "div_u", "rem_s", "rem_u", NULL,
                                          NULL,    NULL,    NULL,   "shr_s", NULL,    "rotl",  "rotr"};
    Val b = pop_val(t), a;
    uint32_t hb = height, ha;
    const char *w = t == T_I32 ? "32" : "64";
    uint64_t mask = t == T_I32 ? 31u : 63u, top = t == T_I32 ? UINT64_C(0x80000000) : UINT64_C(0x8000000000000000);
    int kind, atom = 0;
    char *text;
    a = pop_val(t);
    ha = height;
    if (b.kind != V_SLOT && b.is_const &&
        (((which == 0 || which == 1 || which == 8 || which == 9) && b.bits == 0) || (which == 2 && b.bits == 1) ||
         ((which == 10 || which == 11 || which == 12 || which == 13 || which == 14) && (b.bits & mask) == 0))) {
        /* x + 0, x * 1, x << 0 ...: the operation is x itself. */
        grow_stack();
        stack[height++] = a;
        finish_pop(&b);
        return;
    }
    if (a.kind != V_SLOT && a.is_const && b.kind != V_SLOT && b.is_const && which != 3 && which != 4 &&
        which != 5 && which != 6) {
        /* Both constant: the result is a constant (division may trap, so it
         * stays). */
        uint64_t x = a.bits, y = b.bits, r = 0, width = t == T_I32 ? 32 : 64;
        uint64_t ones = t == T_I32 ? UINT64_C(0xffffffff) : ~UINT64_C(0);
        unsigned n = (unsigned)(y & mask);
        switch (which) {
            case 0: r = x + y; break;
            case 1: r = x - y; break;
            case 2: r = x * y; break;
            case 7: r = x & y; break;
            case 8: r = x | y; break;
            case 9: r = x ^ y; break;
            case 10: r = x << n; break;
            case 11: r = ((x & top) ? ((x & ones) >> n) | (ones & ~(ones >> n)) : (x & ones) >> n); break;
            case 12: r = (x & ones) >> n; break;
            case 13: r = n ? (x << n) | ((x & ones) >> (width - n)) : x; break;
            case 14: r = n ? ((x & ones) >> n) | (x << (width - n)) : x; break;
        }
        finish_pop(&a);
        finish_pop(&b);
        push_const(t, r & ones);
        return;
    }
    kind = kind_of(a.kind, b.kind);
    if (calls[which]) {
        char *x = arg_text(&a, ha), *y = arg_text(&b, hb);
        if (which >= 3 && which <= 6) kind = V_IMPURE; /* division traps */
        text = xprintf("w2c_%s%s(%s, %s)", calls[which], w, x, y);
        atom = 1;
        free(x);
        free(y);
    } else {
        char *x = operand_text(&a, ha), *y;
        int bconst = b.kind != V_SLOT && b.is_const;
        if ((which == 0 || which == 1) && bconst && (b.bits & top)) {
            /* x + (-k) is written x - k. */
            uint64_t k = (t == T_I32 ? (uint64_t)(uint32_t)(0u - (uint32_t)b.bits) : 0u - b.bits);
            y = const_text(t, k);
            text = xprintf("%s %s %s", x, which == 0 ? "-" : "+", y);
        } else if ((which == 10 || which == 12) && bconst) {
            text = xprintf("%s %s %" PRIu64, x, which == 10 ? "<<" : ">>", b.bits & mask);
            y = NULL;
        } else {
            static const char *const ops[15] = {"+", "-", NULL, NULL, NULL, NULL, NULL, "&",
                                                "|", "^", "<<", NULL, ">>", NULL, NULL};
            y = operand_text(&b, hb);
            if (which == 10 || which == 12)
                text = xprintf("%s %s (%s & %su)", x, ops[which], y, t == T_I32 ? "31" : "63");
            else
                text = xprintf("%s %s %s", x, ops[which], y);
        }
        free(x);
        free(y);
    }
    push_expr(t, kind, atom, text, &a, a.reads_local || b.reads_local, reads_upper_slot(&b));
    finish_pop(&a);
    finish_pop(&b);
}

/* A unary operation written by `fmt` around one operand. */
static void op_unary(unsigned from, unsigned to, const char *fmt, int as_operand, int atom) {
    Val a = pop_val(from);
    uint32_t ha = height;
    char *x = as_operand ? operand_text(&a, ha) : arg_text(&a, ha);
    char *text = xprintf(fmt, x);
    free(x);
    push_expr(to, kind_of(a.kind, V_PURE), atom, text, &a, a.reads_local, 0);
    finish_pop(&a);
}

static void op_call(uint32_t callee) {
    const FuncType *ct;
    Val *args;
    Buf call = {0};
    uint32_t i, base, store_local = UINT32_MAX;
    char cname[160];
    if (callee >= num_funcs) die("call target out of range");
    ct = &types[funcs[callee].type];
    flush_impure(height);
    args = xmalloc(((size_t)ct->nparams + 1) * sizeof(Val));
    for (i = ct->nparams; i > 0; i--) args[i - 1] = pop_val(ct->params[i - 1]);
    base = height;
    func_c_name(callee, cname, sizeof(cname));
    bprintf(&call, "%s(", cname);
    for (i = 0; i < ct->nparams; i++) {
        char *a = arg_text(&args[i], base + i);
        bprintf(&call, "%s%s", i ? ", " : "", a);
        free(a);
        finish_pop(&args[i]);
    }
    buf_puts(&call, ")");
    free(args);
    /* call; local.set L stores the result directly. */
    if (ct->result != T_NONE && rd_peek() == 0x21) {
        size_t at = in_pos;
        uint32_t l;
        (void)rd_u8();
        l = rd_u32();
        if (l < num_locals && local_types[l] == ct->result) store_local = l;
        else rd_seek(at);
    }
    if (store_local != UINT32_MAX) {
        local_used[store_local] = 1;
        flush_local(store_local, height);
        line("v%" PRIu32 " = %s;", store_local, call.s);
    } else if (ct->result != T_NONE) {
        char *s = slot_name(ct->result, base);
        push_slot(ct->result);
        line("%s = %s;", s, call.s);
        free(s);
    } else {
        line("%s;", call.s);
    }
    buf_free(&call);
}

static void op_call_indirect(uint32_t type) {
    const FuncType *ct;
    Val c, *args;
    Buf call = {0};
    uint32_t i, base, hc;
    char *index;
    if (type >= num_types) die("call_indirect type out of range");
    ct = &types[type];
    flush_impure(height);
    c = pop_val(T_I32);
    hc = height;
    index = arg_text(&c, hc);
    args = xmalloc(((size_t)ct->nparams + 1) * sizeof(Val));
    for (i = ct->nparams; i > 0; i--) args[i - 1] = pop_val(ct->params[i - 1]);
    base = height;
    buf_puts(&call, "((");
    emit_fnptr_type(&call, type);
    bprintf(&call, ")w2c_indirect(%s, %" PRIu32 "u))(", index, types[type].canon);
    for (i = 0; i < ct->nparams; i++) {
        char *a = arg_text(&args[i], base + i);
        bprintf(&call, "%s%s", i ? ", " : "", a);
        free(a);
        finish_pop(&args[i]);
    }
    buf_puts(&call, ")");
    free(args);
    free(index);
    finish_pop(&c);
    if (ct->result != T_NONE) {
        char *s = slot_name(ct->result, base);
        push_slot(ct->result);
        line("%s = %s;", s, call.s);
        free(s);
    } else {
        line("%s;", call.s);
    }
    buf_free(&call);
}

static void op_br_table(void) {
    uint32_t n = rd_u32(), i, j, *targets, hc;
    unsigned char *done;
    Val c;
    char *index;
    if (n > 1000000) die("br_table too large");
    targets = xmalloc(((size_t)n + 1) * sizeof(uint32_t));
    for (i = 0; i <= n; i++) {
        targets[i] = rd_u32();
        if (targets[i] >= ctl_depth) die("br_table depth out of range");
    }
    c = pop_val(T_I32);
    hc = height;
    flush_impure(height);
    index = arg_text(&c, hc);
    done = xcalloc(ctl_depth, 1);
    done[targets[n]] = 1; /* the default's cases need no case labels */
    for (i = 0; i < n && done[targets[i]]; i++) {
    }
    if (i == n) {
        if (c.kind == V_IMPURE) line(c.atom ? "(void)%s;" : "(void)(%s);", index);
        emit_branch(ctl_depth - 1 - targets[n]);
    } else {
        line("switch (%s) {", index);
        for (i = 0; i < n; i++) {
            Buf cases = {0}, br = {0};
            unsigned on_line = 0;
            if (done[targets[i]]) continue;
            done[targets[i]] = 1;
            for (j = i; j < n; j++) {
                if (targets[j] != targets[i]) continue;
                if (on_line == 8) {
                    line("%s", cases.s);
                    cases.n = 0;
                    on_line = 0;
                }
                bprintf(&cases, "%scase %" PRIu32 ":", on_line ? " " : "", j);
                on_line++;
            }
            branch_text(&br, ctl_depth - 1 - targets[i]);
            line("%s %s", cases.s, br.s);
            buf_free(&cases);
            buf_free(&br);
        }
        {
            Buf br = {0};
            branch_text(&br, ctl_depth - 1 - targets[n]);
            line("default: %s", br.s);
            buf_free(&br);
        }
        line("}");
    }
    free(done);
    free(index);
    free(targets);
    finish_pop(&c);
    live = 0;
}

/* Skips the immediates of an instruction in unreachable code. */
static void skip_immediates(unsigned op, size_t at) {
    uint32_t i;
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
            else if (sub == 11) (void)rd_u8();
            else die("unsupported 0xfc %u instruction at offset %zu", sub, at);
            break;
        }
        default:
            if (op >= 0x28 && op <= 0x3e) { (void)rd_u32(); (void)rd_u32(); }
            else if (op >= 0x45 && op <= 0xc4) { /* no immediates */ }
            else die("unknown opcode 0x%02x at offset %zu", op, at);
    }
}

/* Closes the then-branch of if `c` (the indentation is already the if's): a
 * branch of one simple statement is written `if (cond) statement;`. */
static void close_then(const Ctl *c) {
    const char *seg = body->s + c->then_pos, *p;
    size_t len = body->n - c->then_pos, lines = 0, if_len;
    for (p = seg; p < seg + len; p++) lines += *p == '\n';
    if (lines == 1) {
        while (*seg == ' ') seg++;
        len = (size_t)(body->s + body->n - seg) - 1; /* without the newline */
        if_len = c->then_pos - 3;                     /* the if line without " {\n" */
        p = body->s + if_len;
        while (p > body->s && p[-1] != '\n') p--;
        if (len && strncmp(seg, "if ", 3) != 0 && seg[len - 1] == ';' && !(len >= 2 && seg[len - 2] == ':') &&
            (size_t)(body->s + if_len - p) + 1 + len <= WRAP_COLUMN + 20) {
            char *stmt = xmalloc(len + 1);
            memcpy(stmt, seg, len);
            stmt[len] = 0;
            body->n = if_len;
            buf_puts(body, " ");
            buf_puts(body, stmt);
            buf_puts(body, "\n");
            free(stmt);
            return;
        }
    }
    line("}");
}

/* Structured control. Returns 1 when `op` was one. */
static int control(unsigned op, size_t at) {
    switch (op) {
        case 0x02: /* block */
        case 0x03: /* loop */ {
            unsigned bt = read_block_type();
            if (live) flush_for_block();
            push_ctl(op == 0x02 ? K_BLOCK : K_LOOP, bt);
            /* A block is only a label for forward branches; a loop is a
             * labeled compound statement. */
            if (live && op == 0x03) {
                Ctl *c = &ctls[ctl_depth - 1];
                if (!final_pass || has_flag(c->id, F_LOOP_USED))
                    line("L%" PRIu32 ": {", c->id);
                else
                    line("{");
                c->braced = 1;
                indent++;
            }
            return 1;
        }
        case 0x04: /* if */ {
            unsigned bt = read_block_type();
            if (live) {
                Val c = pop_val(T_I32);
                uint32_t hc = height;
                char *cond;
                Ctl *parent = &ctls[ctl_depth - 1], *y;
                int candidate;
                flush_for_block();
                cond = cond_text(&c, hc);
                candidate = parent->kind == K_IF && parent->had_else && parent->braced && !parent->flat_else &&
                            body->n == parent->else_pos;
                push_ctl(K_IF, bt);
                y = &ctls[ctl_depth - 1];
                y->braced = 1;
                y->chain_candidate = candidate;
                if (final_pass && has_flag(y->id, F_GUARD) && !(candidate && has_flag(y->id, F_ELSE_IF))) {
                    y->guard = 1;
                    y->else_pos = (size_t)-1;
                    y->guard_cond = c.kind != V_SLOT && c.is_bool ? xstrdup(c.negated) : NULL;
                    if (!y->guard_cond) {
                        char *x = operand_text(&c, hc);
                        y->guard_cond = xprintf("!%s", x);
                        free(x);
                    }
                    y->saved_body = body;
                    y->then_buf = xcalloc(1, sizeof(Buf));
                    body = y->then_buf;
                } else if (final_pass && candidate && has_flag(y->id, F_ELSE_IF)) {
                    /* `} else {` + `if (c) {` is `} else if (c) {`: the
                     * parent's end closes both. */
                    while (body->n && body->s[body->n - 1] == '\n') body->n--;
                    while (body->n && body->s[body->n - 1] != '\n') body->n--;
                    indent--;
                    line("} else if (%s) {", cond);
                    indent++;
                    y->chained = 1;
                } else {
                    line("if (%s) {", cond);
                    indent++;
                }
                y->then_pos = body->n;
                free(cond);
                finish_pop(&c);
            } else {
                push_ctl(K_IF, bt);
            }
            return 1;
        }
        case 0x05: /* else */ {
            Ctl *c = &ctls[ctl_depth - 1];
            if (c->kind != K_IF || c->had_else) die("else without matching if at offset %zu", at);
            if (live) {
                if (height != c->height + (c->result != T_NONE))
                    die("if-branch leaves %u values, expected %u", height - c->height, (unsigned)(c->result != T_NONE));
                if (c->result != T_NONE) {
                    if (stack[c->height].type != c->result) die("if-branch result type mismatch");
                    materialize(c->height);
                }
            }
            c->then_live = live;
            c->had_else = 1;
            c->then_size = body->n - c->then_pos;
            truncate_stack(c->height);
            if (c->guard) {
                body = c->saved_body;
                line("if (%s) {", c->guard_cond);
                indent++;
                c->then_pos = body->n;
            } else if (c->braced) {
                indent--;
                if (!live && !c->chained) {
                    close_then(c);
                    c->flat_else = 1;
                } else {
                    line("} else {");
                    indent++;
                    c->else_pos = body->n;
                }
            }
            c->else_start = body->n;
            live = c->live_at_entry;
            return 1;
        }
        case 0x0b: /* end */ {
            Ctl c = ctls[ctl_depth - 1];
            int keep, live_after;
            if (live) {
                if (height != c.height + (c.result != T_NONE))
                    die("block leaves %u values, expected %u", height - c.height, (unsigned)(c.result != T_NONE));
                if (c.result != T_NONE && stack[c.height].type != c.result) die("block result type mismatch");
            }
            if (c.kind == K_FUNC) {
                if (live) {
                    if (ft->result != T_NONE) {
                        char *v = arg_text(&stack[0], 0);
                        line("w2c_leave();");
                        line("return %s;", v);
                        free(v);
                    } else {
                        line("w2c_leave();");
                    }
                }
                truncate_stack(0);
                ctl_depth--;
                return 1;
            }
            if (c.kind == K_IF && !c.had_else && c.result != T_NONE) die("if without else cannot produce a value");
            /* A result no branch writes can stay pending. */
            keep = (c.kind == K_BLOCK && !c.used) || c.kind == K_LOOP;
            if (live && c.result != T_NONE && !keep) materialize(c.height);
            if (c.kind == K_BLOCK) live_after = live || c.used;
            else if (c.kind == K_LOOP) live_after = live;
            else if (!c.had_else) live_after = c.live_at_entry;
            else live_after = c.then_live || live || c.used;
            if (c.kind == K_IF && c.had_else && c.last_child != UINT32_MAX && body->n == c.last_child_end)
                set_flag(c.last_child, F_ELSE_IF); /* its else-branch is one if: a candidate (checked below) */
            if (c.kind == K_IF && c.had_else && c.braced && !live && body->n - c.else_start < c.then_size)
                set_flag(c.id, F_GUARD);
            if (c.guard) {
                indent--;
                close_then(&c);
                buf_put(body, c.then_buf->s ? c.then_buf->s : "", c.then_buf->n);
                buf_free(c.then_buf);
                free(c.then_buf);
                free(c.guard_cond);
            } else if (c.braced && !c.chained && !c.flat_else) {
                indent--;
                if (c.kind == K_IF && !c.had_else) close_then(&c);
                else line("}");
            }
            if (c.kind != K_LOOP && c.used) line("B%" PRIu32 ":;", c.id);
            ctl_depth--;
            /* For the parent's `else if` decision. */
            ctls[ctl_depth - 1].last_child = c.kind == K_IF && c.chain_candidate && !c.used ? c.id : UINT32_MAX;
            ctls[ctl_depth - 1].last_child_end = body->n;
            if (live && c.result != T_NONE) {
                truncate_stack(c.height + 1);
            } else {
                truncate_stack(c.height);
                if (live_after && c.result != T_NONE) push_slot(c.result);
            }
            live = live_after;
            return 1;
        }
    }
    return 0;
}

static const struct {
    const char *fn;
    unsigned result, align;
} loads[] = {
    {"w2c_load_i32", T_I32, 2},     {"w2c_load_i64", T_I64, 3},     {"w2c_load_f32", T_F32, 2},
    {"w2c_load_f64", T_F64, 3},     {"w2c_load_i32_8s", T_I32, 0},  {"w2c_load_i32_8u", T_I32, 0},
    {"w2c_load_i32_16s", T_I32, 1}, {"w2c_load_i32_16u", T_I32, 1}, {"w2c_load_i64_8s", T_I64, 0},
    {"w2c_load_i64_8u", T_I64, 0},  {"w2c_load_i64_16s", T_I64, 1}, {"w2c_load_i64_16u", T_I64, 1},
    {"w2c_load_i64_32s", T_I64, 2}, {"w2c_load_i64_32u", T_I64, 2},
};

static const struct {
    const char *fn;
    unsigned value, align;
} stores[] = {
    {"w2c_store_i32", T_I32, 2},    {"w2c_store_i64", T_I64, 3},    {"w2c_store_f32", T_F32, 2},
    {"w2c_store_f64", T_F64, 3},    {"w2c_store_i32_8", T_I32, 0},  {"w2c_store_i32_16", T_I32, 1},
    {"w2c_store_i64_8", T_I64, 0},  {"w2c_store_i64_16", T_I64, 1}, {"w2c_store_i64_32", T_I64, 2},
};

/* One reachable non-control instruction. */
static void instruction(unsigned op, size_t at) {
    uint32_t i;
    switch (op) {
        case 0x00: /* unreachable */
            flush_impure(height);
            line("w2c_trap(\"unreachable executed\");");
            live = 0;
            return;
        case 0x01: /* nop */
            return;
        case 0x0c: { /* br */
            uint32_t depth = rd_u32();
            Ctl *c;
            if (depth >= ctl_depth) die("br depth out of range");
            c = &ctls[ctl_depth - 1 - depth];
            /* The branch value is evaluated by the branch itself. */
            flush_impure(c->kind != K_LOOP && c->result != T_NONE && height ? height - 1 : height);
            emit_branch(ctl_depth - 1 - depth);
            live = 0;
            return;
        }
        case 0x0d: { /* br_if */
            uint32_t depth = rd_u32(), hc;
            Val c;
            char *cond;
            if (depth >= ctl_depth) die("br_if depth out of range");
            c = pop_val(T_I32);
            hc = height;
            flush_impure(height);
            cond = cond_text(&c, hc);
            emit_cond_branch(cond, ctl_depth - 1 - depth);
            free(cond);
            finish_pop(&c);
            return;
        }
        case 0x0e: /* br_table */
            op_br_table();
            return;
        case 0x0f: /* return */
            flush_impure(ft->result != T_NONE && height ? height - 1 : height);
            emit_branch(0);
            live = 0;
            return;
        case 0x10: /* call */
            op_call(rd_u32());
            return;
        case 0x11: { /* call_indirect */
            uint32_t type = rd_u32(), table = rd_u32();
            if (table != 0 || !have_table) die("call_indirect without table 0");
            op_call_indirect(type);
            return;
        }
        case 0x1a: { /* drop */
            Val a = pop_val(T_NONE);
            if (a.kind == V_IMPURE) /* still evaluated: it may trap */
                line(a.atom ? "(void)%s;" : "(void)(%s);", a.text);
            finish_pop(&a);
            return;
        }
        case 0x1b:   /* select */
        case 0x1c: { /* select t */
            unsigned t = T_NONE;
            Val c, b, a;
            uint32_t ha, hb, hc;
            char *x, *y, *z;
            if (op == 0x1c) {
                uint32_t n = rd_u32();
                if (n != 1) die("typed select with %u types", n);
                t = rd_u8();
                if (!is_value_type(t)) die("typed select: bad type");
            }
            /* Both operands are evaluated: neither may be pending and impure. */
            flush_impure(height);
            c = pop_val(T_I32);
            hc = height;
            if (t == T_NONE) t = top_type();
            b = pop_val(t);
            hb = height;
            a = pop_val(t);
            ha = height;
            z = cond_text(&c, hc);
            x = operand_text(&a, ha);
            y = operand_text(&b, hb);
            push_expr(t, V_PURE, 0, xprintf("%s ? %s : %s", z, x, y), &a, a.reads_local || b.reads_local || c.reads_local,
                      reads_upper_slot(&b) || reads_upper_slot(&c));
            free(x);
            free(y);
            free(z);
            finish_pop(&a);
            finish_pop(&b);
            finish_pop(&c);
            return;
        }
        case 0x20: { /* local.get */
            uint32_t l = rd_u32();
            if (l >= num_locals) die("local index out of range");
            local_used[l] = 1;
            push_expr(local_types[l], V_PURE, 1, xprintf("v%" PRIu32, l), NULL, 1, 0);
            return;
        }
        case 0x21: /* local.set */
            op_local_set(rd_u32(), 0);
            return;
        case 0x22: /* local.tee */
            op_local_set(rd_u32(), 1);
            return;
        case 0x23: { /* global.get */
            uint32_t g = rd_u32();
            if (g >= num_globals) die("global index out of range");
            push_expr(globals[g].type, V_IMPURE, 1, xprintf("w2c_g%" PRIu32, g), NULL, 0, 0);
            return;
        }
        case 0x24: { /* global.set */
            uint32_t g = rd_u32(), hv;
            Val v;
            char *s;
            if (g >= num_globals) die("global index out of range");
            if (!globals[g].mutable_) die("global.set on immutable global");
            v = pop_val(globals[g].type);
            hv = height;
            flush_impure(height);
            s = arg_text(&v, hv);
            line("w2c_g%" PRIu32 " = %s;", g, s);
            free(s);
            finish_pop(&v);
            return;
        }
        case 0x3f: /* memory.size */
            if (rd_u8() != 0 || !have_memory) die("memory.size on memory other than 0");
            push_expr(T_I32, V_IMPURE, 1, xstrdup("w2c_memory_pages()"), NULL, 0, 0);
            return;
        case 0x40: { /* memory.grow */
            Val a;
            uint32_t ha;
            char *x, *s;
            if (rd_u8() != 0 || !have_memory) die("memory.grow on memory other than 0");
            a = pop_val(T_I32);
            ha = height;
            flush_impure(height);
            x = arg_text(&a, ha);
            s = slot_name(T_I32, ha);
            push_slot(T_I32);
            line("%s = w2c_memory_grow(%s);", s, x);
            free(x);
            free(s);
            finish_pop(&a);
            return;
        }
        case 0x41:
            push_const(T_I32, rd_s32());
            return;
        case 0x42:
            push_const(T_I64, rd_s64());
            return;
        case 0x43: {
            uint32_t bits = 0;
            for (i = 0; i < 4; i++) bits |= (uint32_t)rd_u8() << (8 * i);
            push_const(T_F32, bits);
            return;
        }
        case 0x44: {
            uint64_t bits = 0;
            for (i = 0; i < 8; i++) bits |= (uint64_t)rd_u8() << (8 * i);
            push_const(T_F64, bits);
            return;
        }
        case 0x45: op_eqz(T_I32); return;
        case 0x50: op_eqz(T_I64); return;
    }

    if (op >= 0x28 && op <= 0x35) { /* loads */
        unsigned li = op - 0x28;
        uint32_t off = memarg(loads[li].align), ha;
        Val a = pop_val(T_I32);
        char *x;
        ha = height;
        x = arg_text(&a, ha);
        push_expr(loads[li].result, V_IMPURE, 1, xprintf("%s(%s, %" PRIu32 "u)", loads[li].fn, x, off), &a,
                  a.reads_local, 0);
        free(x);
        finish_pop(&a);
        return;
    }
    if (op >= 0x36 && op <= 0x3e) { /* stores */
        unsigned si = op - 0x36;
        uint32_t off = memarg(stores[si].align), hv, ha;
        Val v = pop_val(stores[si].value), a;
        char *x, *y;
        hv = height;
        a = pop_val(T_I32);
        ha = height;
        flush_impure(height);
        x = arg_text(&a, ha);
        y = arg_text(&v, hv);
        line("%s(%s, %" PRIu32 "u, %s);", stores[si].fn, x, off, y);
        free(x);
        free(y);
        finish_pop(&a);
        finish_pop(&v);
        return;
    }
    /* Comparisons: i32 0x46-0x4f, i64 0x51-0x5a, f32 0x5b-0x60, f64 0x61-0x66. */
    if ((op >= 0x46 && op <= 0x4f) || (op >= 0x51 && op <= 0x5a)) {
        static const char *const cmp[10] = {"==", "!=", "<", "<", ">", ">", "<=", "<=", ">=", ">="};
        static const char *const neg[10] = {"!=", "==", ">=", ">=", "<=", "<=", ">", ">", "<", "<"};
        unsigned t = op <= 0x4f ? T_I32 : T_I64, k = op - (op <= 0x4f ? 0x46 : 0x51);
        op_compare(t, cmp[k], neg[k], k >= 2 && k % 2 == 0);
        return;
    }
    if (op >= 0x5b && op <= 0x66) {
        static const char *const cmp[6] = {"==", "!=", "<", ">", "<=", ">="};
        unsigned t = op <= 0x60 ? T_F32 : T_F64;
        op_compare(t, cmp[op - (op <= 0x60 ? 0x5b : 0x61)], NULL, 0);
        return;
    }
    /* Integer unary: clz ctz popcnt. */
    if ((op >= 0x67 && op <= 0x69) || (op >= 0x79 && op <= 0x7b)) {
        static const char *const fmt32[3] = {"w2c_clz32(%s)", "w2c_ctz32(%s)", "w2c_popcnt32(%s)"};
        static const char *const fmt64[3] = {"w2c_clz64(%s)", "w2c_ctz64(%s)", "w2c_popcnt64(%s)"};
        unsigned t = op <= 0x69 ? T_I32 : T_I64;
        op_unary(t, t, t == T_I32 ? fmt32[op - 0x67] : fmt64[op - 0x79], 0, 1);
        return;
    }
    /* Integer binary: i32 0x6a-0x78, i64 0x7c-0x8a. */
    if ((op >= 0x6a && op <= 0x78) || (op >= 0x7c && op <= 0x8a)) {
        op_int_binary(op <= 0x78 ? T_I32 : T_I64, op <= 0x78 ? op - 0x6a : op - 0x7c);
        return;
    }
    /* Float unary and binary: f32 0x8b-0x98, f64 0x99-0xa6. Only operations
     * whose IEEE semantics C gives exactly (no libm) are accepted. */
    if (op >= 0x8b && op <= 0xa6) {
        unsigned t = op <= 0x98 ? T_F32 : T_F64, which = op <= 0x98 ? op - 0x8b : op - 0x99;
        if (which <= 6) { /* abs neg ceil floor trunc nearest sqrt */
            if (which >= 2) die("float operation 0x%02x needs libm; not supported", op);
            op_unary(t, t, t == T_F32 ? (which == 0 ? "w2c_f32_abs(%s)" : "w2c_f32_neg(%s)")
                                      : (which == 0 ? "w2c_f64_abs(%s)" : "w2c_f64_neg(%s)"), 0, 1);
        } else if (which <= 10 || which == 13) {
            static const char *const ops[4] = {"+", "-", "*", "/"};
            Val b = pop_val(t), a;
            uint32_t hb = height, ha;
            char *x, *y, *text;
            a = pop_val(t);
            ha = height;
            if (which == 13) {
                x = arg_text(&a, ha);
                y = arg_text(&b, hb);
                text = xprintf("w2c_f%s_copysign(%s, %s)", t == T_F32 ? "32" : "64", x, y);
                push_expr(t, kind_of(a.kind, b.kind), 1, text, &a, a.reads_local || b.reads_local, reads_upper_slot(&b));
            } else {
                /* One float operation per statement. */
                x = operand_text(&a, ha);
                y = operand_text(&b, hb);
                text = xprintf("%s %s %s", x, ops[which - 7], y);
                push_expr(t, kind_of(a.kind, b.kind), 0, text, &a, a.reads_local || b.reads_local, 1);
            }
            free(x);
            free(y);
            finish_pop(&a);
            finish_pop(&b);
        } else {
            die("float min/max (0x%02x) not supported", op);
        }
        return;
    }
    /* Conversions and sign extension; of a constant, a constant. */
    if (height > ctls[ctl_depth - 1].height && stack[height - 1].kind != V_SLOT && stack[height - 1].is_const &&
        (op == 0xa7 || op == 0xac || op == 0xad || (op >= 0xc0 && op <= 0xc4))) {
        Val a = pop_val(op == 0xa7 || op >= 0xc2 ? T_I64 : T_I32);
        uint64_t x = a.bits, r;
        unsigned to = op == 0xac || op == 0xad || op >= 0xc2 ? T_I64 : T_I32;
        switch (op) {
            case 0xa7: r = x & UINT64_C(0xffffffff); break;
            case 0xac: r = ((x & UINT64_C(0xffffffff)) ^ UINT64_C(0x80000000)) - UINT64_C(0x80000000); break;
            case 0xad: r = x & UINT64_C(0xffffffff); break;
            case 0xc0: case 0xc2: r = ((x & 0xffu) ^ 0x80u) - UINT64_C(0x80); break;
            case 0xc1: case 0xc3: r = ((x & 0xffffu) ^ 0x8000u) - UINT64_C(0x8000); break;
            default: r = ((x & UINT64_C(0xffffffff)) ^ UINT64_C(0x80000000)) - UINT64_C(0x80000000); break;
        }
        finish_pop(&a);
        push_const(to, to == T_I32 ? r & UINT64_C(0xffffffff) : r);
        return;
    }
    switch (op) {
        case 0xa7: op_unary(T_I64, T_I32, "(uint32_t)%s", 1, 1); return;
        case 0xac: op_unary(T_I32, T_I64, "(uint64_t)(%s ^ 0x80000000u) - UINT64_C(0x80000000)", 1, 0); return;
        case 0xad: op_unary(T_I32, T_I64, "(uint64_t)%s", 1, 1); return;
        case 0xbc: op_unary(T_F32, T_I32, "w2c_f32_bits(%s)", 0, 1); return;
        case 0xbd: op_unary(T_F64, T_I64, "w2c_f64_bits(%s)", 0, 1); return;
        case 0xbe: op_unary(T_I32, T_F32, "w2c_f32_from_bits(%s)", 0, 1); return;
        case 0xbf: op_unary(T_I64, T_F64, "w2c_f64_from_bits(%s)", 0, 1); return;
        case 0xc0: op_unary(T_I32, T_I32, "((%s & 0xffu) ^ 0x80u) - 0x80u", 1, 0); return;
        case 0xc1: op_unary(T_I32, T_I32, "((%s & 0xffffu) ^ 0x8000u) - 0x8000u", 1, 0); return;
        case 0xc2: op_unary(T_I64, T_I64, "((%s & 0xffu) ^ 0x80u) - UINT64_C(0x80)", 1, 0); return;
        case 0xc3: op_unary(T_I64, T_I64, "((%s & 0xffffu) ^ 0x8000u) - UINT64_C(0x8000)", 1, 0); return;
        case 0xc4:
            op_unary(T_I64, T_I64, "((%s & UINT64_C(0xffffffff)) ^ UINT64_C(0x80000000)) - UINT64_C(0x80000000)", 1, 0);
            return;
    }
    if (op >= 0xa8 && op <= 0xbb) die("float/int conversion 0x%02x is not supported (SultanC MSIR has none)", op);
    if (op == 0xfc) {
        uint32_t sub = rd_u32(), hn, hs, hd;
        Val n, s, d;
        char *x, *y, *z;
        if (sub == 10) { /* memory.copy dst src n */
            if (rd_u8() != 0 || rd_u8() != 0 || !have_memory) die("memory.copy on memory other than 0");
        } else if (sub == 11) { /* memory.fill dst value n */
            if (rd_u8() != 0 || !have_memory) die("memory.fill on memory other than 0");
        } else {
            die("unsupported 0xfc %u instruction at offset %zu", sub, at);
        }
        n = pop_val(T_I32);
        hn = height;
        s = pop_val(T_I32);
        hs = height;
        d = pop_val(T_I32);
        hd = height;
        flush_impure(height);
        x = arg_text(&d, hd);
        y = arg_text(&s, hs);
        z = arg_text(&n, hn);
        line("w2c_memory_%s(%s, %s, %s);", sub == 10 ? "copy" : "fill", x, y, z);
        free(x);
        free(y);
        free(z);
        finish_pop(&n);
        finish_pop(&s);
        finish_pop(&d);
        return;
    }
    die("unknown or unsupported opcode 0x%02x at offset %zu", op, at);
}

/* ---- functions ------------------------------------------------------------------------ */

/* One pass over the body of function idx into `out`. */
static void translate_pass(Buf *out, uint32_t idx) {
    body = out;
    rd_seek(funcs[idx].body_start);
    {
        uint32_t groups = rd_u32(), i;
        for (i = 0; i < groups; i++) {
            (void)rd_u32();
            (void)rd_u8();
        }
    }
    truncate_stack(0);
    ctl_depth = 0;
    next_label = 0;
    live = 1;
    indent = 1;
    memset(local_used, 0, num_locals);
    if (stack_cap) {
        uint32_t k;
        for (k = 0; k < 4; k++) memset(slot_used[k], 0, stack_cap);
    }
    push_ctl(K_FUNC, ft->result);
    while (ctl_depth > 0) {
        size_t at = in_pos;
        unsigned op;
        if (in_pos >= funcs[idx].body_end) die("body ends before its final end");
        op = rd_u8();
        if (control(op, at)) continue;
        if (!live) skip_immediates(op, at);
        else instruction(op, at);
    }
    if (in_pos != funcs[idx].body_end) die("instructions after the final end");
}

/* `type name, name, ...;` wrapped at WRAP_COLUMN. */
static void declare(Buf *out, unsigned t, const char *const *names, uint32_t count) {
    size_t col;
    uint32_t i;
    if (!count) return;
    bprintf(out, "%*s%s ", INDENT_WIDTH, "", ctype(t));
    col = INDENT_WIDTH + strlen(ctype(t)) + 1;
    for (i = 0; i < count; i++) {
        size_t len = strlen(names[i]) + 2;
        if (i && col + len > WRAP_COLUMN) {
            buf_puts(out, ",\n");
            buf_spaces(out, 2 * INDENT_WIDTH);
            col = 2 * INDENT_WIDTH;
        } else if (i) {
            buf_puts(out, ", ");
            col += 2;
        }
        buf_puts(out, names[i]);
        col += len - 2;
    }
    buf_puts(out, ";\n");
}

void translate_function(Buf *out, uint32_t idx) {
    static const unsigned value_types[4] = {T_I32, T_I64, T_F32, T_F64};
    Buf scratch = {0}, code = {0};
    char context[64], **names;
    uint32_t i, k, count, cap;

    snprintf(context, sizeof(context), "function %" PRIu32, idx);
    current_context = context;
    ft = &types[funcs[idx].type];

    /* Locals: the parameters, then the declared groups. */
    rd_seek(funcs[idx].body_start);
    num_locals = ft->nparams;
    local_types = xrealloc(local_types, (size_t)ft->nparams + 1);
    memcpy(local_types, ft->params, ft->nparams);
    {
        uint32_t groups = rd_u32();
        for (i = 0; i < groups; i++) {
            uint32_t n = rd_u32();
            unsigned t = rd_u8();
            if (!is_value_type(t)) die("unsupported local type 0x%02x", t);
            if ((uint64_t)num_locals + n > 50000) die("too many locals");
            local_types = xrealloc(local_types, (size_t)num_locals + n + 1);
            memset(local_types + num_locals, (int)t, n);
            num_locals += n;
        }
    }
    local_used = xrealloc(local_used, (size_t)num_locals + 1);

    /* Pass 1 finds the loops that branches target; pass 2 writes the C. */
    if (ctl_flags_cap) memset(ctl_flags, 0, ctl_flags_cap);
    final_pass = 0;
    translate_pass(&scratch, idx);
    buf_free(&scratch);
    final_pass = 1;
    translate_pass(&code, idx);

    emit_prototype(out, idx, 1);
    buf_puts(out, " {\n");
    cap = num_locals > stack_cap ? num_locals : stack_cap;
    names = xmalloc(((size_t)cap + 1) * sizeof(char *));
    /* WebAssembly locals start at zero. */
    for (k = 0; k < 4; k++) {
        count = 0;
        for (i = ft->nparams; i < num_locals; i++)
            if (local_types[i] == value_types[k] && local_used[i]) names[count++] = xprintf("v%" PRIu32 " = 0", i);
        declare(out, value_types[k], (const char *const *)names, count);
        for (i = 0; i < count; i++) free(names[i]);
    }
    for (k = 0; k < 4; k++) {
        count = 0;
        for (i = 0; i < stack_cap; i++)
            if (slot_used[k][i]) names[count++] = xprintf("%c%" PRIu32, tchar(value_types[k]), i);
        declare(out, value_types[k], (const char *const *)names, count);
        for (i = 0; i < count; i++) free(names[i]);
    }
    free(names);
    bprintf(out, "%*sw2c_enter();\n", INDENT_WIDTH, "");
    buf_put(out, code.s ? code.s : "", code.n);
    buf_puts(out, "}\n");
    buf_free(&code);
    current_context = "module";
}
