/* stream.c: reading the module bytes. */
#include "stream.h"

#include <stdlib.h>
#include <string.h>

#include "zstd_decoder.h"

const unsigned char *in_bytes;
size_t in_size, in_pos;

uint8_t rd_u8(void) {
    if (in_pos >= in_size) die("truncated module at offset %zu", in_pos);
    return in_bytes[in_pos++];
}
uint64_t rd_leb(unsigned bits, int is_signed) {
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
uint32_t rd_u32(void) { return (uint32_t)rd_leb(32, 0); }
uint32_t rd_s32(void) { return (uint32_t)rd_leb(32, 1); }
uint64_t rd_s64(void) { return rd_leb(64, 1); }
int64_t rd_s33(void) {
    /* Block types: a signed 33-bit LEB (negative single-byte forms for value
     * types and the empty type, non-negative type indices). */
    uint64_t v = rd_leb(64, 1);
    return (int64_t)v;
}
void rd_seek(size_t pos) {
    if (pos > in_size) die("offset %zu outside module", pos);
    in_pos = pos;
}
char *rd_name(void) {
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


int rd_peek(void) { return in_pos < in_size ? in_bytes[in_pos] : -1; }

unsigned char *load_file(const char *path, size_t *len, int decompress) {
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
