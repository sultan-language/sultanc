/* Owns existing Stage0 LLVM native object generation. */

#include "llvm/llvm_internal.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_module.h"
#include "llvm/llvm_optimization.h"
#include <string.h>

/* Emits the bootstrap LLVM object. */
int __Bootstrap_Emit_LLVM_Object__(__Semantic_Context__ *semantic, const char *path)
{
    /* Stores the emitter. */
    __LLVM_Emitter__ emitter;
    /* References the saved unit. */
    const __Program_Unit__ *saved_unit = NULL;
    /* Stores the machine. */
    LLVMTargetMachineRef machine = NULL;
    /* Stores the data. */
    LLVMTargetDataRef data = NULL;
    /* References the triple. */
    char *triple = NULL;
    /* References the layout. */
    char *layout = NULL;
    /* References the message. */
    char *message = NULL;
    /* Tracks whether the operation succeeded. */
    int ok = 0;

    memset(&emitter, 0, sizeof(emitter));
    if (path == NULL)
        return __LLVM_Fail__("missing native object output path");
    if (__LLVM_Prepare_Program_Module__(
            semantic, &emitter, &saved_unit, &machine, &data, &triple, &layout))
    {
        if (__LLVM_Optimize_Module__(&emitter, machine))
        {
            if (LLVMVerifyModule(emitter.module, LLVMReturnStatusAction, &message))
            {
                __LLVM_Fail_Message__("LLVM verifier rejected optimized module", message);
                message = NULL;
            }
            else if (LLVMTargetMachineEmitToFile(
                         machine, emitter.module, (char *)path, LLVMObjectFile, &message))
            {
                __LLVM_Fail_Message__("LLVM native object emission failed", message);
                message = NULL;
            }
            else
            {
                ok = 1;
            }
        }
    }
    if (message != NULL)
        LLVMDisposeMessage(message);
    __LLVM_Dispose_Program_Module__(&emitter, machine, data, triple, layout);
    if (semantic != NULL && saved_unit != NULL)
        semantic->__Active_Unit__ = saved_unit;
    return ok;
}
