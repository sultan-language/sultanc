/* Declares the bootstrap x86_64 ELF executable finalizer. */

#ifndef SULTANC_BOOTSTRAP_LINK_ELF_EXECUTABLE_H
#define SULTANC_BOOTSTRAP_LINK_ELF_EXECUTABLE_H

#include "link/byte_buffer.h"

#include <stddef.h>
#include <stdint.h>

int __Bootstrap_Finalize_ELF_X86_64__(const uint8_t *object,
                                      size_t object_size,
                                      __Bootstrap_Byte_Buffer__ *output,
                                      char *error,
                                      size_t error_size);

#endif
