/* sultanc-wasm2c: translate the SultanC WASI seed (a WebAssembly module) to C.
 * Copyright 2026 SultanC contributors. MIT license (see repository LICENSE).
 *
 * The seed is translated ahead of time to plain C99, compiled with the host C
 * compiler and linked with the runtime (runtime/w2c.c) and the WASI host
 * (runtime/wasi.c), so building SultanC from source needs only a C compiler.
 *
 * The parts:
 *   stream.c   the module bytes: LEB128, names, loading (zstd_decoder.h)
 *   module.c   the module model, its parser and what is accepted
 *   funcgen.c  one function body to C
 *   output.c   the generated files
 *
 * Usage: sultanc-wasm2c [--units=N] SEED.wasm[.zst] OUT_PREFIX
 *        sultanc-wasm2c --decompress FILE.zst OUT
 * A Zstandard-compressed seed (as committed) is decompressed in memory with
 * the in-tree decoder; a malformed frame is rejected with a diagnostic, as is
 * a malformed module. The translation is deterministic: the same seed always
 * gives the same C.
 */
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "module.h"
#include "output.h"
#include "stream.h"

/* Function units, compiled in parallel. */
#define DEFAULT_UNITS 8

int main(int argc, char **argv) {
    const char *seed = NULL, *prefix = NULL;
    unsigned units = DEFAULT_UNITS;
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
        if (strncmp(argv[i], "--units=", 8) == 0) {
            units = (unsigned)strtoul(argv[i] + 8, NULL, 10);
            if (units < 1 || units > 999) die("--units must be 1..999");
        } else if (!seed) {
            seed = argv[i];
        } else if (!prefix) {
            prefix = argv[i];
        } else {
            die("unexpected argument %s", argv[i]);
        }
    }
    if (!seed || !prefix) {
        fprintf(stderr,
                "usage: sultanc-wasm2c [--units=N] SEED.wasm[.zst] OUT_PREFIX\n"
                "       sultanc-wasm2c --decompress FILE.zst OUT\n");
        return 2;
    }
    bytes = load_file(seed, &size, 1);
    in_bytes = bytes;
    in_size = size;
    in_pos = 0;
    parse_module();
    write_header(prefix);
    write_module_unit(prefix);
    write_function_units(prefix, units);
    fprintf(stderr,
            "sultanc-wasm2c: %" PRIu32 " functions (%" PRIu32 " imported), %" PRIu32 " types, %" PRIu32
            " globals, %" PRIu32 " data segments -> %u units\n",
            num_funcs, num_imported, num_types, num_globals, num_datas, units);
    return 0;
}
