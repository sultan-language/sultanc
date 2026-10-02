/* Owns Stage0 LLVM target-registry initialization. */

#include "llvm/llvm_target.h"

/* Initializes every LLVM target backend linked into this Stage0 build. */
int __LLVM_Initialize_Targets__(void)
{
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmPrinters();
    return 1;
}
