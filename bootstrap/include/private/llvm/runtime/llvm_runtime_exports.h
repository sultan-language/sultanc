/* Declares Stage0 definitions for SultanC runtime ABI exports. */

#ifndef SULTANC_BOOTSTRAP_LLVM_RUNTIME_EXPORTS_H
#define SULTANC_BOOTSTRAP_LLVM_RUNTIME_EXPORTS_H

#include "llvm/llvm_internal.h"

/* Defines runtime ABI symbols that are referenced by ordinary typed extern calls. */
int __LLVM_Define_Runtime_Exports__(__LLVM_Emitter__ *emitter);

#endif
