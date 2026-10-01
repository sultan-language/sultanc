/* Owns Stage0 LLVM host target initialization. */

#include "llvm/llvm_target.h"
#include "llvm/llvm_context.h"

/* Initializes the LLVM host target. */
int __LLVM_Initialize_Host_Target__(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmPrinter();
    return 1;
#elif defined(__aarch64__) || defined(_M_ARM64)
    LLVMInitializeAArch64TargetInfo();
    LLVMInitializeAArch64Target();
    LLVMInitializeAArch64TargetMC();
    LLVMInitializeAArch64AsmPrinter();
    return 1;
#else
    return __LLVM_Fail__(
        "current Bootstrap LLVM target initialization supports x86_64/AArch64 hosts");
#endif
}
