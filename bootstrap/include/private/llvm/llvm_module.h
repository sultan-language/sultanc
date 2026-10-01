/* Declares private Stage0 LLVM module lifecycle. */

#ifndef SULTANC_BOOTSTRAP_LLVM_MODULE_H
#define SULTANC_BOOTSTRAP_LLVM_MODULE_H

#include "llvm/llvm_internal.h"

void __LLVM_Dispose_Program_Module__(__LLVM_Emitter__ *emitter,
                                     LLVMTargetMachineRef machine,
                                     LLVMTargetDataRef data,
                                     char *triple,
                                     char *layout);
int __LLVM_Prepare_Program_Module__(__Semantic_Context__ *semantic,
                                    __LLVM_Emitter__ *emitter,
                                    const __Program_Unit__ **saved_unit,
                                    LLVMTargetMachineRef *machine,
                                    LLVMTargetDataRef *data,
                                    char **triple,
                                    char **layout);

#endif
