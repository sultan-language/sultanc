/* stream.h: the module bytes and the readers for WebAssembly's encodings
 * (bytes, LEB128 integers, names), plus loading a seed file. */
#ifndef W2C_STREAM_H
#define W2C_STREAM_H

#include "common.h"

extern const unsigned char *in_bytes;
extern size_t in_size, in_pos;

uint8_t rd_u8(void);
/* The byte at the read position, without consuming it (-1 at the end). */
int rd_peek(void);
uint64_t rd_leb(unsigned bits, int is_signed);
uint32_t rd_u32(void);
uint32_t rd_s32(void);
uint64_t rd_s64(void);
int64_t rd_s33(void);
void rd_seek(size_t pos);
char *rd_name(void);

/* Reads a whole file; a Zstandard frame is decompressed when `decompress`. */
unsigned char *load_file(const char *path, size_t *len, int decompress);

#endif
