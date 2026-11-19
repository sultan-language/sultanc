/* zstd_decoder.h: a small Zstandard (RFC 8878) decompressor for the bootstrap.
 * Copyright 2026 SultanC contributors. MIT license (see repository LICENSE).
 *
 * Written from the format specification (RFC 8878) for SultanC; it contains
 * no code from the reference zstd implementation. It decodes whole buffers
 * only (the seed is a few MB), which keeps it simple: a match may refer to
 * anything already decoded in the same frame, so no window bookkeeping is
 * needed. Supported: one or more Zstandard frames, skippable frames, raw /
 * RLE / compressed blocks, all literal and sequence encodings, repeat tables
 * and offsets, and the optional XXH64 content checksum (verified).
 * Not supported: dictionaries (Dictionary_ID != 0 is rejected).
 *
 * Usage (single-header; define nothing, include once):
 *   int zstd_decompress(const unsigned char *src, size_t len,
 *                       unsigned char **out, size_t *out_len,
 *                       char *err, size_t err_cap);   // 1 = ok, 0 = error
 * The caller frees *out.
 */
#ifndef SULTANC_ZSTD_DECODER_H
#define SULTANC_ZSTD_DECODER_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ZD_MAX_BLOCK (128u * 1024u)
#define ZD_MAX_OUTPUT ((size_t)1 << 31) /* 2 GiB: far above any seed */

typedef struct {
    const unsigned char *src;
    size_t len, pos;
    unsigned char *out;
    size_t out_len, out_cap;
    char *err;
    size_t err_cap;
    int failed;
} ZdCtx;

static int zd_fail(ZdCtx *z, const char *msg) {
    if (!z->failed && z->err && z->err_cap) {
        size_t n = strlen(msg);
        if (n >= z->err_cap) n = z->err_cap - 1;
        memcpy(z->err, msg, n);
        z->err[n] = 0;
    }
    z->failed = 1;
    return 0;
}

static unsigned zd_highbit(uint32_t x) { /* x > 0 */
    unsigned n = 0;
    while (x >>= 1) n++;
    return n;
}

/* ---- XXH64 (for the content checksum) ---------------------------------------- */

#define ZD_P1 UINT64_C(11400714785074694791)
#define ZD_P2 UINT64_C(14029467366897019727)
#define ZD_P3 UINT64_C(1609587929392839161)
#define ZD_P4 UINT64_C(9650029242287828579)
#define ZD_P5 UINT64_C(2870177450012600261)

static uint64_t zd_rotl64(uint64_t x, unsigned r) { return (x << r) | (x >> (64 - r)); }
static uint64_t zd_le64(const unsigned char *p) {
    uint64_t v = 0;
    unsigned i;
    for (i = 0; i < 8; i++) v |= (uint64_t)p[i] << (8 * i);
    return v;
}
static uint32_t zd_le32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint64_t zd_round(uint64_t acc, uint64_t in) {
    acc += in * ZD_P2;
    acc = zd_rotl64(acc, 31);
    return acc * ZD_P1;
}
static uint64_t zd_merge(uint64_t acc, uint64_t v) {
    acc ^= zd_round(0, v);
    return acc * ZD_P1 + ZD_P4;
}
static uint64_t zd_xxh64(const unsigned char *p, size_t n) {
    const unsigned char *end = p + n;
    uint64_t h;
    if (n >= 32) {
        uint64_t v1 = ZD_P1 + ZD_P2, v2 = ZD_P2, v3 = 0, v4 = (uint64_t)0 - ZD_P1;
        const unsigned char *limit = end - 32;
        do {
            v1 = zd_round(v1, zd_le64(p)); p += 8;
            v2 = zd_round(v2, zd_le64(p)); p += 8;
            v3 = zd_round(v3, zd_le64(p)); p += 8;
            v4 = zd_round(v4, zd_le64(p)); p += 8;
        } while (p <= limit);
        h = zd_rotl64(v1, 1) + zd_rotl64(v2, 7) + zd_rotl64(v3, 12) + zd_rotl64(v4, 18);
        h = zd_merge(h, v1); h = zd_merge(h, v2); h = zd_merge(h, v3); h = zd_merge(h, v4);
    } else {
        h = ZD_P5;
    }
    h += (uint64_t)n;
    while (p + 8 <= end) {
        h ^= zd_round(0, zd_le64(p));
        h = zd_rotl64(h, 27) * ZD_P1 + ZD_P4;
        p += 8;
    }
    if (p + 4 <= end) {
        h ^= (uint64_t)zd_le32(p) * ZD_P1;
        h = zd_rotl64(h, 23) * ZD_P2 + ZD_P3;
        p += 4;
    }
    while (p < end) {
        h ^= (uint64_t)(*p++) * ZD_P5;
        h = zd_rotl64(h, 11) * ZD_P1;
    }
    h ^= h >> 33; h *= ZD_P2; h ^= h >> 29; h *= ZD_P3; h ^= h >> 32;
    return h;
}

/* ---- output ----------------------------------------------------------------- */

static int zd_reserve(ZdCtx *z, size_t extra) {
    size_t need, cap;
    unsigned char *p;
    if (extra > ZD_MAX_OUTPUT - z->out_len) return zd_fail(z, "decompressed data too large");
    need = z->out_len + extra;
    if (z->out && need <= z->out_cap) return 1; /* the buffer always exists, even for empty output */
    cap = z->out_cap ? z->out_cap : 1 << 16;
    while (cap < need) cap = cap > ZD_MAX_OUTPUT / 2 ? ZD_MAX_OUTPUT : cap * 2;
    p = (unsigned char *)realloc(z->out, cap);
    if (!p) return zd_fail(z, "out of memory");
    z->out = p;
    z->out_cap = cap;
    return 1;
}

/* ---- bit streams --------------------------------------------------------------- */

/* Forward little-endian bit reader (FSE table descriptions). */
typedef struct { const unsigned char *p; size_t len; size_t bit; } ZdFwd;

static uint32_t zd_fwd_bits(ZdFwd *r, unsigned n, int *ok) {
    uint32_t v = 0;
    unsigned i;
    for (i = 0; i < n; i++) {
        size_t byte = r->bit >> 3;
        if (byte >= r->len) { *ok = 0; return 0; }
        v |= (uint32_t)((r->p[byte] >> (r->bit & 7)) & 1) << i;
        r->bit++;
    }
    return v;
}

/* Backward bit reader: bits are consumed from the end of the buffer toward its
 * start; the last byte's highest set bit is a padding marker. Reading below
 * bit 0 yields zeros and drives `pos` negative (used to detect the end). */
typedef struct { const unsigned char *p; int64_t pos; } ZdBack;

static int zd_back_init(ZdBack *r, const unsigned char *p, size_t len) {
    unsigned char last;
    if (len == 0) return 0;
    last = p[len - 1];
    if (last == 0) return 0;
    r->p = p;
    r->pos = (int64_t)len * 8 - 8 + (int64_t)zd_highbit(last);
    return 1;
}
static uint64_t zd_back_bits(ZdBack *r, unsigned n) {
    int64_t start;
    uint64_t v = 0;
    unsigned i, skip = 0;
    if (n == 0) return 0;
    r->pos -= n;
    start = r->pos;
    if (start < 0) { skip = (unsigned)(-start) > n ? n : (unsigned)(-start); start = 0; }
    for (i = 0; i < n - skip; i++) {
        uint64_t b = (uint64_t)start + i;
        v |= (uint64_t)((r->p[b >> 3] >> (b & 7)) & 1) << i;
    }
    return skip ? (skip >= 64 ? 0 : v << skip) : v;
}

/* ---- FSE ---------------------------------------------------------------------- */

#define ZD_FSE_MAX_LOG 9
#define ZD_FSE_MAX_SYMS 256

typedef struct {
    unsigned log;
    uint8_t symbol[1 << ZD_FSE_MAX_LOG];
    uint8_t bits[1 << ZD_FSE_MAX_LOG];
    uint16_t base[1 << ZD_FSE_MAX_LOG];
} ZdFse;

static int zd_fse_build(ZdCtx *z, ZdFse *t, const int16_t *norm, unsigned nsym, unsigned log) {
    unsigned size = 1u << log, high = size, step, mask = size - 1, pos = 0, s, i;
    uint16_t next[ZD_FSE_MAX_SYMS];
    if (log > ZD_FSE_MAX_LOG || nsym > ZD_FSE_MAX_SYMS) return zd_fail(z, "FSE table too large");
    t->log = log;
    for (s = 0; s < nsym; s++) {
        if (norm[s] == -1) {
            t->symbol[--high] = (uint8_t)s;
            next[s] = 1;
        } else {
            next[s] = (uint16_t)(norm[s] > 0 ? norm[s] : 0);
        }
    }
    step = (size >> 1) + (size >> 3) + 3;
    for (s = 0; s < nsym; s++) {
        int k;
        for (k = 0; k < norm[s]; k++) {
            t->symbol[pos] = (uint8_t)s;
            do pos = (pos + step) & mask; while (pos >= high);
        }
    }
    if (pos != 0) return zd_fail(z, "corrupt FSE distribution");
    for (i = 0; i < size; i++) {
        unsigned sym = t->symbol[i], ns = next[sym]++;
        t->bits[i] = (uint8_t)(log - zd_highbit(ns));
        t->base[i] = (uint16_t)((ns << t->bits[i]) - size);
    }
    return 1;
}

/* Reads an FSE table description at src[*pos..end); advances *pos. */
static int zd_fse_read(ZdCtx *z, ZdFse *t, const unsigned char *src, size_t len, size_t *pos,
                       unsigned max_log, unsigned max_sym) {
    ZdFwd r;
    int ok = 1;
    int16_t norm[ZD_FSE_MAX_SYMS];
    unsigned log, sym = 0;
    int32_t remaining;
    r.p = src + *pos;
    r.len = len - *pos;
    r.bit = 0;
    log = zd_fwd_bits(&r, 4, &ok) + 5;
    if (!ok || log > max_log) return zd_fail(z, "FSE accuracy log too large");
    remaining = (int32_t)(1u << log);
    while (remaining > 0 && sym <= max_sym) {
        unsigned nb = zd_highbit((uint32_t)remaining + 1) + 1;
        uint32_t val = zd_fwd_bits(&r, nb, &ok);
        uint32_t lower = (1u << (nb - 1)) - 1, threshold = (1u << nb) - 1 - ((uint32_t)remaining + 1);
        int32_t proba;
        if ((val & lower) < threshold) {
            r.bit--;
            val &= lower;
        } else if (val > lower) {
            val -= threshold;
        }
        if (!ok) return zd_fail(z, "truncated FSE table");
        proba = (int32_t)val - 1;
        remaining -= proba < 0 ? -proba : proba;
        norm[sym++] = (int16_t)proba;
        if (proba == 0) {
            uint32_t rep = zd_fwd_bits(&r, 2, &ok);
            for (;;) {
                uint32_t k;
                for (k = 0; k < rep && sym <= max_sym; k++) norm[sym++] = 0;
                if (rep != 3) break;
                rep = zd_fwd_bits(&r, 2, &ok);
                if (!ok) return zd_fail(z, "truncated FSE table");
            }
        }
        if (!ok) return zd_fail(z, "truncated FSE table");
    }
    if (remaining != 0) return zd_fail(z, "corrupt FSE table");
    *pos += (r.bit + 7) >> 3;
    return zd_fse_build(z, t, norm, sym, log);
}

static void zd_fse_rle(ZdFse *t, uint8_t symbol) {
    t->log = 0;
    t->symbol[0] = symbol;
    t->bits[0] = 0;
    t->base[0] = 0;
}

/* ---- Huffman (literals) ------------------------------------------------------------- */

#define ZD_HUF_MAX_BITS 11

typedef struct {
    unsigned max_bits;
    uint8_t symbol[1 << ZD_HUF_MAX_BITS];
    uint8_t bits[1 << ZD_HUF_MAX_BITS];
    int valid;
} ZdHuf;

static int zd_huf_read(ZdCtx *z, ZdHuf *h, const unsigned char *src, size_t len, size_t *pos) {
    uint8_t weight[256];
    unsigned n = 0, i, max_bits, last;
    uint32_t total = 0, left;
    unsigned rank_count[ZD_HUF_MAX_BITS + 2], rank_next[ZD_HUF_MAX_BITS + 2];
    unsigned header;
    if (*pos >= len) return zd_fail(z, "truncated Huffman header");
    header = src[(*pos)++];
    if (header >= 128) {
        n = header - 127;
        if (*pos + (n + 1) / 2 > len) return zd_fail(z, "truncated Huffman weights");
        for (i = 0; i < n; i++) {
            unsigned b = src[*pos + i / 2];
            weight[i] = (uint8_t)(i % 2 == 0 ? b >> 4 : b & 15);
        }
        *pos += (n + 1) / 2;
    } else {
        /* FSE-compressed weights: two interleaved states, one backward stream. */
        ZdFse t;
        ZdBack br;
        size_t start = *pos, table_end;
        uint32_t s1, s2;
        if (start + header > len || header == 0) return zd_fail(z, "truncated Huffman weights");
        table_end = start;
        if (!zd_fse_read(z, &t, src, start + header, &table_end, 6, 255)) return 0;
        if (table_end >= start + header || !zd_back_init(&br, src + table_end, start + header - table_end))
            return zd_fail(z, "corrupt Huffman weight stream");
        s1 = (uint32_t)zd_back_bits(&br, t.log);
        s2 = (uint32_t)zd_back_bits(&br, t.log);
        for (;;) {
            if (n >= 255) return zd_fail(z, "too many Huffman weights");
            weight[n++] = t.symbol[s1];
            s1 = t.base[s1] + (uint32_t)zd_back_bits(&br, t.bits[s1]);
            if (br.pos < 0) { weight[n++] = t.symbol[s2]; break; }
            if (n >= 255) return zd_fail(z, "too many Huffman weights");
            weight[n++] = t.symbol[s2];
            s2 = t.base[s2] + (uint32_t)zd_back_bits(&br, t.bits[s2]);
            if (br.pos < 0) { weight[n++] = t.symbol[s1]; break; }
        }
        *pos = start + header;
    }
    for (i = 0; i < n; i++) {
        if (weight[i] > ZD_HUF_MAX_BITS) return zd_fail(z, "Huffman weight too large");
        if (weight[i]) total += 1u << (weight[i] - 1);
    }
    if (total == 0) return zd_fail(z, "empty Huffman table");
    max_bits = zd_highbit(total) + 1;
    left = (1u << max_bits) - total;
    if (left & (left - 1)) return zd_fail(z, "corrupt Huffman weights");
    last = zd_highbit(left) + 1;
    if (n >= 256 || max_bits > ZD_HUF_MAX_BITS) return zd_fail(z, "corrupt Huffman weights");
    weight[n++] = (uint8_t)last;
    /* Canonical codes: longest codes first in the table. */
    memset(rank_count, 0, sizeof(rank_count));
    for (i = 0; i < n; i++) if (weight[i]) rank_count[max_bits + 1 - weight[i]]++;
    rank_next[max_bits] = 0;
    for (i = max_bits; i >= 1; i--) rank_next[i - 1] = rank_next[i] + rank_count[i] * (1u << (max_bits - i));
    if (rank_next[0] != (1u << max_bits)) return zd_fail(z, "corrupt Huffman tree");
    for (i = 0; i < n; i++) {
        unsigned b, len_codes, k;
        if (!weight[i]) continue;
        b = max_bits + 1 - weight[i];
        len_codes = 1u << (max_bits - b);
        for (k = 0; k < len_codes; k++) {
            h->symbol[rank_next[b] + k] = (uint8_t)i;
            h->bits[rank_next[b] + k] = (uint8_t)b;
        }
        rank_next[b] += len_codes;
    }
    h->max_bits = max_bits;
    h->valid = 1;
    return 1;
}

static int zd_huf_stream(ZdCtx *z, const ZdHuf *h, const unsigned char *src, size_t len,
                         unsigned char *dst, size_t count) {
    ZdBack br;
    uint32_t state, mask = (1u << h->max_bits) - 1;
    size_t i;
    if (!zd_back_init(&br, src, len)) return zd_fail(z, "corrupt Huffman stream");
    state = (uint32_t)zd_back_bits(&br, h->max_bits);
    for (i = 0; i < count; i++) {
        unsigned b = h->bits[state];
        dst[i] = h->symbol[state];
        state = ((state << b) | (uint32_t)zd_back_bits(&br, b)) & mask;
    }
    if (br.pos != -(int64_t)h->max_bits) return zd_fail(z, "Huffman stream length mismatch");
    return 1;
}

/* ---- sequences -------------------------------------------------------------------- */

static const uint32_t zd_ll_base[36] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22, 24, 28, 32, 40,
    48, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
static const uint8_t zd_ll_bits[36] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3,
    4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
static const uint32_t zd_ml_base[53] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
    27, 28, 29, 30, 31, 32, 33, 34, 35, 37, 39, 41, 43, 47, 51, 59, 67, 83, 99, 131, 259, 515,
    1027, 2051, 4099, 8195, 16387, 32771, 65539};
static const uint8_t zd_ml_bits[53] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
static const int16_t zd_ll_default[36] = {
    4, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 1, 1, 1, 1, 1,
    -1, -1, -1, -1};
static const int16_t zd_ml_default[53] = {
    1, 4, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1};
static const int16_t zd_of_default[29] = {
    1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1};

typedef struct {
    ZdHuf huf;
    ZdFse ll, of, ml;
    int have_ll, have_of, have_ml;
    uint64_t rep[3];
} ZdFrame;

static int zd_seq_table(ZdCtx *z, ZdFse *t, int *have, unsigned mode, const unsigned char *src, size_t len,
                        size_t *pos, const int16_t *def, unsigned def_n, unsigned def_log, unsigned max_log,
                        unsigned max_sym) {
    switch (mode) {
        case 0: *have = 1; return zd_fse_build(z, t, def, def_n, def_log);
        case 1:
            if (*pos >= len) return zd_fail(z, "truncated RLE sequence table");
            if (src[*pos] > max_sym) return zd_fail(z, "RLE sequence symbol out of range");
            zd_fse_rle(t, src[(*pos)++]);
            *have = 1;
            return 1;
        case 2:
            *have = 1;
            return zd_fse_read(z, t, src, len, pos, max_log, max_sym);
        default:
            if (!*have) return zd_fail(z, "repeat sequence table without a previous table");
            return 1;
    }
}

static int zd_block(ZdCtx *z, ZdFrame *f, const unsigned char *src, size_t len) {
    unsigned char lit_buf[ZD_MAX_BLOCK];
    size_t pos = 0, lit_len = 0, lit_pos = 0, nseq, i;
    unsigned ltype, lsize;
    /* Literals section. */
    if (len < 1) return zd_fail(z, "empty compressed block");
    ltype = src[0] & 3;
    lsize = (src[0] >> 2) & 3;
    if (ltype <= 1) {
        size_t hdr = lsize == 1 ? 2 : lsize == 3 ? 3 : 1;
        if (len < hdr) return zd_fail(z, "truncated literals header");
        if (hdr == 1) lit_len = src[0] >> 3;
        else if (hdr == 2) lit_len = (src[0] >> 4) | ((size_t)src[1] << 4);
        else lit_len = (src[0] >> 4) | ((size_t)src[1] << 4) | ((size_t)src[2] << 12);
        if (lit_len > ZD_MAX_BLOCK) return zd_fail(z, "literals too large");
        pos = hdr;
        if (ltype == 0) {
            if (pos + lit_len > len) return zd_fail(z, "truncated raw literals");
            memcpy(lit_buf, src + pos, lit_len);
            pos += lit_len;
        } else {
            if (pos >= len) return zd_fail(z, "truncated RLE literals");
            memset(lit_buf, src[pos++], lit_len);
        }
    } else {
        size_t hdr = lsize <= 1 ? 3 : lsize == 2 ? 4 : 5, comp, j;
        unsigned bits = lsize <= 1 ? 10 : lsize == 2 ? 14 : 18, streams = lsize == 0 ? 1 : 4;
        uint64_t v = 0;
        size_t tree_start;
        if (len < hdr) return zd_fail(z, "truncated literals header");
        for (j = 0; j < hdr; j++) v |= (uint64_t)src[j] << (8 * j);
        lit_len = (size_t)((v >> 4) & ((1u << bits) - 1));
        comp = (size_t)((v >> (4 + bits)) & ((1u << bits) - 1));
        if (lit_len > ZD_MAX_BLOCK) return zd_fail(z, "literals too large");
        pos = hdr;
        if (pos + comp > len) return zd_fail(z, "truncated compressed literals");
        tree_start = pos;
        if (ltype == 2) {
            if (!zd_huf_read(z, &f->huf, src, pos + comp, &pos)) return 0;
        } else if (!f->huf.valid) {
            return zd_fail(z, "treeless literals without a previous Huffman table");
        }
        comp -= pos - tree_start;
        if (streams == 1) {
            if (!zd_huf_stream(z, &f->huf, src + pos, comp, lit_buf, lit_len)) return 0;
        } else {
            size_t s1, s2, s3, s4, each = (lit_len + 3) / 4, o = 6;
            if (comp < 6) return zd_fail(z, "truncated literal jump table");
            s1 = src[pos] | ((size_t)src[pos + 1] << 8);
            s2 = src[pos + 2] | ((size_t)src[pos + 3] << 8);
            s3 = src[pos + 4] | ((size_t)src[pos + 5] << 8);
            if (s1 + s2 + s3 + 6 > comp || each * 3 > lit_len) return zd_fail(z, "corrupt literal jump table");
            s4 = comp - 6 - s1 - s2 - s3;
            if (!zd_huf_stream(z, &f->huf, src + pos + o, s1, lit_buf, each)) return 0;
            o += s1;
            if (!zd_huf_stream(z, &f->huf, src + pos + o, s2, lit_buf + each, each)) return 0;
            o += s2;
            if (!zd_huf_stream(z, &f->huf, src + pos + o, s3, lit_buf + 2 * each, each)) return 0;
            o += s3;
            if (!zd_huf_stream(z, &f->huf, src + pos + o, s4, lit_buf + 3 * each, lit_len - 3 * each)) return 0;
        }
        pos += comp;
    }

    /* Sequences section. */
    if (pos >= len) {
        nseq = 0;
    } else {
        unsigned b0 = src[pos++];
        if (b0 < 128) nseq = b0;
        else if (b0 < 255) {
            if (pos >= len) return zd_fail(z, "truncated sequence count");
            nseq = ((size_t)(b0 - 128) << 8) + src[pos++];
        } else {
            if (pos + 2 > len) return zd_fail(z, "truncated sequence count");
            nseq = (size_t)src[pos] + ((size_t)src[pos + 1] << 8) + 0x7f00;
            pos += 2;
        }
    }
    if (nseq > 0) {
        unsigned modes;
        ZdBack br;
        uint32_t sll, sof, sml;
        if (pos >= len) return zd_fail(z, "truncated sequence modes");
        modes = src[pos++];
        if (modes & 3) return zd_fail(z, "reserved sequence mode bits set");
        if (!zd_seq_table(z, &f->ll, &f->have_ll, modes >> 6, src, len, &pos, zd_ll_default, 36, 6, 9, 35) ||
            !zd_seq_table(z, &f->of, &f->have_of, (modes >> 4) & 3, src, len, &pos, zd_of_default, 29, 5, 8, 31) ||
            !zd_seq_table(z, &f->ml, &f->have_ml, (modes >> 2) & 3, src, len, &pos, zd_ml_default, 53, 6, 9, 52))
            return 0;
        if (!zd_back_init(&br, src + pos, len - pos)) return zd_fail(z, "corrupt sequence stream");
        sll = (uint32_t)zd_back_bits(&br, f->ll.log);
        sof = (uint32_t)zd_back_bits(&br, f->of.log);
        sml = (uint32_t)zd_back_bits(&br, f->ml.log);
        for (i = 0; i < nseq; i++) {
            unsigned llc = f->ll.symbol[sll], ofc = f->of.symbol[sof], mlc = f->ml.symbol[sml];
            uint64_t ofv, offset, ll, ml;
            if (llc > 35 || mlc > 52 || ofc > 31) return zd_fail(z, "sequence code out of range");
            ofv = ((uint64_t)1 << ofc) + zd_back_bits(&br, ofc);
            ml = zd_ml_base[mlc] + zd_back_bits(&br, zd_ml_bits[mlc]);
            ll = zd_ll_base[llc] + zd_back_bits(&br, zd_ll_bits[llc]);
            if (ofv > 3) {
                offset = ofv - 3;
                f->rep[2] = f->rep[1]; f->rep[1] = f->rep[0]; f->rep[0] = offset;
            } else {
                unsigned idx = (unsigned)ofv - 1 + (ll == 0);
                if (idx == 0) {
                    offset = f->rep[0];
                } else {
                    offset = idx < 3 ? f->rep[idx] : f->rep[0] - 1;
                    if (idx > 1) f->rep[2] = f->rep[1];
                    f->rep[1] = f->rep[0];
                    f->rep[0] = offset;
                }
            }
            if (ll > lit_len - lit_pos) return zd_fail(z, "sequence literals exceed the literal buffer");
            if (!zd_reserve(z, (size_t)ll + (size_t)ml)) return 0;
            memcpy(z->out + z->out_len, lit_buf + lit_pos, (size_t)ll);
            z->out_len += (size_t)ll;
            lit_pos += (size_t)ll;
            if (offset == 0 || offset > z->out_len) return zd_fail(z, "match offset out of range");
            {
                /* Byte-wise copy: overlapping matches repeat the pattern. */
                unsigned char *d = z->out + z->out_len;
                const unsigned char *s = d - offset;
                uint64_t k;
                for (k = 0; k < ml; k++) d[k] = s[k];
                z->out_len += (size_t)ml;
            }
            if (i + 1 < nseq) {
                sll = f->ll.base[sll] + (uint32_t)zd_back_bits(&br, f->ll.bits[sll]);
                sml = f->ml.base[sml] + (uint32_t)zd_back_bits(&br, f->ml.bits[sml]);
                sof = f->of.base[sof] + (uint32_t)zd_back_bits(&br, f->of.bits[sof]);
            }
        }
        if (br.pos != 0) return zd_fail(z, "sequence stream length mismatch");
    }
    if (!zd_reserve(z, lit_len - lit_pos)) return 0;
    memcpy(z->out + z->out_len, lit_buf + lit_pos, lit_len - lit_pos);
    z->out_len += lit_len - lit_pos;
    return 1;
}

static int zd_frame(ZdCtx *z) {
    const unsigned char *s = z->src;
    unsigned fhd, fcs_flag, single, checksum, did_flag;
    size_t start_out = z->out_len;
    uint64_t content_size = 0;
    int have_size = 0;
    ZdFrame *f;
    if (z->pos + 1 > z->len) return zd_fail(z, "truncated frame header");
    fhd = s[z->pos++];
    fcs_flag = fhd >> 6;
    single = (fhd >> 5) & 1;
    checksum = (fhd >> 2) & 1;
    did_flag = fhd & 3;
    if (fhd & 8) return zd_fail(z, "reserved frame header bit set");
    if (!single) {
        if (z->pos >= z->len) return zd_fail(z, "truncated frame header");
        z->pos++; /* window descriptor: not needed for whole-buffer decoding */
    }
    if (did_flag) {
        unsigned n = did_flag == 3 ? 4 : did_flag, k;
        uint32_t id = 0;
        if (z->pos + n > z->len) return zd_fail(z, "truncated frame header");
        for (k = 0; k < n; k++) id |= (uint32_t)s[z->pos + k] << (8 * k);
        z->pos += n;
        if (id) return zd_fail(z, "dictionaries are not supported");
    }
    {
        unsigned n = fcs_flag == 0 ? (single ? 1 : 0) : fcs_flag == 1 ? 2 : fcs_flag == 2 ? 4 : 8, k;
        if (z->pos + n > z->len) return zd_fail(z, "truncated frame header");
        for (k = 0; k < n; k++) content_size |= (uint64_t)s[z->pos + k] << (8 * k);
        if (n == 2) content_size += 256;
        z->pos += n;
        have_size = n > 0;
    }
    if (have_size && content_size <= ZD_MAX_OUTPUT && !zd_reserve(z, (size_t)content_size)) return 0;
    f = (ZdFrame *)calloc(1, sizeof(*f));
    if (!f) return zd_fail(z, "out of memory");
    f->rep[0] = 1; f->rep[1] = 4; f->rep[2] = 8;
    for (;;) {
        uint32_t bh;
        unsigned last, type;
        size_t size;
        if (z->pos + 3 > z->len) { free(f); return zd_fail(z, "truncated block header"); }
        bh = (uint32_t)s[z->pos] | (uint32_t)s[z->pos + 1] << 8 | (uint32_t)s[z->pos + 2] << 16;
        z->pos += 3;
        last = bh & 1;
        type = (bh >> 1) & 3;
        size = bh >> 3;
        if (type == 3) { free(f); return zd_fail(z, "reserved block type"); }
        if (size > ZD_MAX_BLOCK) { free(f); return zd_fail(z, "block too large"); }
        if (type == 1) {
            if (z->pos + 1 > z->len || !zd_reserve(z, size)) { free(f); return zd_fail(z, "truncated RLE block"); }
            memset(z->out + z->out_len, s[z->pos], size);
            z->out_len += size;
            z->pos += 1;
        } else {
            if (z->pos + size > z->len) { free(f); return zd_fail(z, "truncated block"); }
            if (type == 0) {
                if (!zd_reserve(z, size)) { free(f); return 0; }
                memcpy(z->out + z->out_len, s + z->pos, size);
                z->out_len += size;
            } else if (!zd_block(z, f, s + z->pos, size)) {
                free(f);
                return 0;
            }
            z->pos += size;
        }
        if (last) break;
    }
    free(f);
    if (have_size && z->out_len - start_out != content_size) return zd_fail(z, "frame content size mismatch");
    if (checksum) {
        uint64_t h;
        if (z->pos + 4 > z->len) return zd_fail(z, "truncated content checksum");
        h = zd_xxh64(z->out + start_out, z->out_len - start_out);
        if ((uint32_t)h != zd_le32(s + z->pos)) return zd_fail(z, "content checksum mismatch");
        z->pos += 4;
    }
    return 1;
}

static int zstd_is_compressed(const unsigned char *src, size_t len) {
    return len >= 4 && zd_le32(src) == 0xFD2FB528u;
}

static int zstd_decompress(const unsigned char *src, size_t len, unsigned char **out, size_t *out_len,
                           char *err, size_t err_cap) {
    ZdCtx z;
    memset(&z, 0, sizeof(z));
    z.src = src;
    z.len = len;
    z.err = err;
    z.err_cap = err_cap;
    if (!zstd_is_compressed(src, len)) { zd_fail(&z, "not a Zstandard frame"); return 0; }
    while (z.pos < len) {
        uint32_t magic;
        if (z.pos + 4 > len) { zd_fail(&z, "trailing bytes after the last frame"); break; }
        magic = zd_le32(src + z.pos);
        z.pos += 4;
        if ((magic & 0xFFFFFFF0u) == 0x184D2A50u) { /* skippable frame */
            uint32_t n;
            if (z.pos + 4 > len) { zd_fail(&z, "truncated skippable frame"); break; }
            n = zd_le32(src + z.pos);
            z.pos += 4;
            if (n > len - z.pos) { zd_fail(&z, "truncated skippable frame"); break; }
            z.pos += n;
            continue;
        }
        if (magic != 0xFD2FB528u) { zd_fail(&z, "unknown frame magic"); break; }
        if (!zd_frame(&z)) break;
    }
    if (z.failed) { free(z.out); return 0; }
    *out = z.out ? z.out : (unsigned char *)malloc(1);
    *out_len = z.out_len;
    return *out != NULL;
}

#endif
