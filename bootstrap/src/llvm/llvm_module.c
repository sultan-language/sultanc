/* Owns Stage0 LLVM program module setup and teardown. */

#include "llvm/llvm_module.h"
#include "llvm/llvm_aggregates.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_functions.h"
#include "llvm/llvm_target.h"
#include "kernel/type/type.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Releases the LLVM program module. */
void __LLVM_Dispose_Program_Module__(__LLVM_Emitter__ *emitter,
                                    LLVMTargetMachineRef machine,
                                    LLVMTargetDataRef data)
{
    if (data != NULL)
        LLVMDisposeTargetData(data);
    if (machine != NULL)
        LLVMDisposeTargetMachine(machine);
    free(emitter->locals);
    free(emitter->functions);
    free(emitter->aggregate_types);
    if (emitter->allocation_builder != NULL)
        LLVMDisposeBuilder(emitter->allocation_builder);
    if (emitter->builder != NULL)
        LLVMDisposeBuilder(emitter->builder);
    if (emitter->module != NULL)
        LLVMDisposeModule(emitter->module);
    if (emitter->context != NULL)
        LLVMContextDispose(emitter->context);
    __Bootstrap_Target_Destroy__(&emitter->target);
}

/* Returns the LLVM prepare program module. */
int __LLVM_Prepare_Program_Module__(__Semantic_Context__ *semantic,
                                   const char *requested_target,
                                   __LLVM_Emitter__ *emitter,
                                   const __Program_Unit__ **saved_unit,
                                   LLVMTargetMachineRef *machine,
                                   LLVMTargetDataRef *data)
{
    /* References the main entry. */
    __Semantic_Function_Entry__ *main_entry;
    /* References the main function. */
    __Ast_Function__ *main_function;
    /* Stores the output resolved. */
    __Resolved_Type__ output_resolved;
    /* Stores the target. */
    LLVMTargetRef target = NULL;
    /* References the message. */
    char *message = NULL;
    /* Stores a target error. */
    char target_error[256];
    /* Stores an LLVM target lookup prefix. */
    char target_lookup_error[320];
    /* Tracks the index. */
    size_t index;

    memset(emitter, 0, sizeof(*emitter));
    *saved_unit = NULL;
    *machine = NULL;
    *data = NULL;
    target_error[0] = '\0';
    __LLVM_Error__[0] = '\0';
    if (semantic == NULL || semantic->__Main__ == NULL)
        return __LLVM_Fail__("invalid direct LLVM emitter arguments or missing entry point");

    main_entry = semantic->__Main__;
    main_function = main_entry->__Function__;
    if (main_function == NULL || main_function->__Body__ == NULL ||
        main_function->__Parameter_Count__ != 0U)
        return __LLVM_Fail__("L2.3 entry point must be a zero-parameter function with a body");

    *saved_unit = semantic->__Active_Unit__;
    semantic->__Active_Unit__ = main_entry->__Unit__;
    if (!__Type_Resolve__(semantic, main_function->__Output__.__Type__, &output_resolved) ||
        (output_resolved.__Kind__ != __Resolved_Type_Signed_Integer__ &&
         output_resolved.__Kind__ != __Resolved_Type_Unsigned_Integer__) ||
        output_resolved.__Bits__ == 0U)
        return __LLVM_Fail__("Bootstrap language entry must return a canonical integer Type");

    if (!__Bootstrap_Target_Init__(
            &emitter->target, requested_target, target_error, sizeof(target_error)))
        return __LLVM_Fail__(target_error);
    if (!__LLVM_Initialize_Targets__())
        return 0;

    emitter->semantic = semantic;
    emitter->context = LLVMContextCreate();
    emitter->module = LLVMModuleCreateWithNameInContext("sultanc_stage0_l25", emitter->context);
    emitter->builder = LLVMCreateBuilderInContext(emitter->context);
    emitter->allocation_builder = LLVMCreateBuilderInContext(emitter->context);
    if (emitter->context == NULL || emitter->module == NULL || emitter->builder == NULL ||
        emitter->allocation_builder == NULL)
        return __LLVM_Fail__("LLVM context/module/builder creation failed");

    LLVMSetTarget(emitter->module, emitter->target.triple);
    if (LLVMGetTargetFromTriple(emitter->target.triple, &target, &message))
    {
        snprintf(target_lookup_error,
                 sizeof(target_lookup_error),
                 "LLVM target backend is unavailable for '%s'",
                 emitter->target.triple);
        __LLVM_Fail_Message__(target_lookup_error, message);
        return 0;
    }
    *machine = LLVMCreateTargetMachine(target,
                                       emitter->target.triple,
                                       "",
                                       "",
                                       LLVMCodeGenLevelNone,
                                       LLVMRelocDefault,
                                       LLVMCodeModelDefault);
    if (*machine == NULL)
    {
        snprintf(target_lookup_error,
                 sizeof(target_lookup_error),
                 "LLVM TargetMachine creation failed for '%s'",
                 emitter->target.triple);
        return __LLVM_Fail__(target_lookup_error);
    }
    *data = LLVMCreateTargetDataLayout(*machine);
    if (*data == NULL)
        return __LLVM_Fail__("LLVM target data layout creation failed");
    if (LLVMPointerSize(*data) != 8U)
        return __LLVM_Fail__(
            "current Bootstrap canonical layout requires a 64-bit LLVM target");
    LLVMSetModuleDataLayout(emitter->module, *data);

    if (!__LLVM_Predeclare_Aggregate_Types__(emitter) ||
        !__LLVM_Define_Aggregate_Types__(emitter) || !__LLVM_Declare_Functions__(emitter))
        return 0;
    for (index = 0U; index < emitter->function_count; ++index)
        if (!__LLVM_Emit_Function_Body__(emitter, &emitter->functions[index]))
            return 0;
    if (!__LLVM_Emit_Process_Entry__(emitter))
        return 0;

    if (LLVMVerifyModule(emitter->module, LLVMReturnStatusAction, &message))
    {
        __LLVM_Fail_Message__("LLVM verifier rejected L2.3 module", message);
        return 0;
    }
    return 1;
}
