/* Declares private Stage0 LLVM optimization. */

#ifndef SULTANC_BOOTSTRAP_LLVM_OPTIMIZATION_H
#define SULTANC_BOOTSTRAP_LLVM_OPTIMIZATION_H

#include "llvm/llvm_internal.h"

int __LLVM_Optimize_Module__(__LLVM_Emitter__ *emitter, LLVMTargetMachineRef machine);

#endif
