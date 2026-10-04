/* Declares the bootstrap ARM64 Mach-O executable finalizer. */

#ifndef SULTANC_BOOTSTRAP_LINK_MACHO_EXECUTABLE_H
#define SULTANC_BOOTSTRAP_LINK_MACHO_EXECUTABLE_H

#include "link/byte_buffer.h"

#include <stddef.h>
#include <stdint.h>

int __Bootstrap_Finalize_MachO_ARM64__(const uint8_t *object,
                                       size_t object_size,
                                       __Bootstrap_Byte_Buffer__ *output,
                                       const char *llvm_runtime_library,
                                       const char *llvm_runtime_dir,
                                       char *error,
                                       size_t error_size);

#endif
