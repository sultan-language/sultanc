/* Owns existing Stage0 LLVM execution/JIT path. */

#include "llvm/llvm_internal.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_module.h"
#include <string.h>

/* Runs the bootstrap LLVM program. */
int __Bootstrap_Run_LLVM_Program__(__Semantic_Context__ *semantic,
                                   int argc,
                                   const char *const *argv,
                                   int *status)
{
    /* Stores the emitter. */
    __LLVM_Emitter__ emitter;
    /* References the saved unit. */
    const __Program_Unit__ *saved_unit = NULL;
    /* Stores the machine. */
    LLVMTargetMachineRef machine = NULL;
    /* Stores the data. */
    LLVMTargetDataRef data = NULL;
    /* Stores the engine. */
    LLVMExecutionEngineRef engine = NULL;
    /* Stores the main value. */
    LLVMValueRef main_value = NULL;
    /* References the triple. */
    char *triple = NULL;
    /* References the layout. */
    char *layout = NULL;
    /* References the message. */
    char *message = NULL;
    /* Tracks whether the operation succeeded. */
    int ok = 0;

    memset(&emitter, 0, sizeof(emitter));
    if (status == NULL || argc < 1 || argv == NULL)
        return __LLVM_Fail__("invalid Direct-LLVM execution request");
    if (__LLVM_Prepare_Program_Module__(
            semantic, &emitter, &saved_unit, &machine, &data, &triple, &layout))
    {
        main_value = LLVMGetNamedFunction(emitter.module, "main");
        if (main_value == NULL)
        {
            __LLVM_Fail__("Direct-LLVM execution module has no process entry");
        }
        else
        {
            LLVMLinkInMCJIT();
            if (LLVMCreateExecutionEngineForModule(&engine, emitter.module, &message))
            {
                __LLVM_Fail_Message__("LLVM execution engine creation failed", message);
                message = NULL;
            }
            else
            {
                emitter.module = NULL;
                *status = LLVMRunFunctionAsMain(engine, main_value, (unsigned)argc, argv, NULL);
                ok = 1;
            }
        }
    }
    if (message != NULL)
        LLVMDisposeMessage(message);
    if (engine != NULL)
        LLVMDisposeExecutionEngine(engine);
    __LLVM_Dispose_Program_Module__(&emitter, machine, data, triple, layout);
    if (semantic != NULL && saved_unit != NULL)
        semantic->__Active_Unit__ = saved_unit;
    return ok;
}
