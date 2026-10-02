/* Declares private Stage0 LLVM module lifecycle. */

#ifndef SULTANC_BOOTSTRAP_LLVM_MODULE_H
#define SULTANC_BOOTSTRAP_LLVM_MODULE_H

#include "llvm/llvm_internal.h"

void __LLVM_Dispose_Program_Module__(__LLVM_Emitter__ *emitter,
                                     LLVMTargetMachineRef machine,
                                     LLVMTargetDataRef data);
int __LLVM_Prepare_Program_Module__(__Semantic_Context__ *semantic,
                                    const char *requested_target,
                                    __LLVM_Emitter__ *emitter,
                                    const __Program_Unit__ **saved_unit,
                                    LLVMTargetMachineRef *machine,
                                    LLVMTargetDataRef *data);

#endif
