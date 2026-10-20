/* Owns Stage0 LLVM program module setup and teardown. */

#include "llvm/llvm_module.h"
#include "llvm/llvm_aggregates.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_functions.h"
#include "llvm/llvm_globals.h"
#include "llvm/llvm_target.h"
#include "kernel/type/type.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Defines one executable-owned process startup symbol, reusing a source-level
 * external declaration of the same name (canonical
 * compiler/backend/provider/llvm/entry.sn أعلن_أو_أعد_استخدام_عام_إقلاع). The
 * symbol names are the canonical startup ABI spellings owned by
 * compiler/runtime/primitive_runtime_abi.sn. ELF definitions are weak so a
 * strong native entry object may supply them; Mach-O definitions are hidden. */
static LLVMValueRef __LLVM_Define_Startup_Global__(__LLVM_Emitter__ *emitter,
                                                   const char *name,
                                                   LLVMTypeRef type)
{
    /* Stores the symbol text. */
    __Text_Slice__ symbol;
    /* Stores the global. */
    LLVMValueRef global;

    symbol.__Data__ = name;
    symbol.__Length__ = strlen(name);
    global = __LLVM_Declare_External_Global__(emitter, symbol, type);
    if (global == NULL)
        return NULL;
    if (LLVMGetInitializer(global) != NULL)
    {
        __LLVM_Fail__("process startup symbol is already defined in this module");
        return NULL;
    }
    LLVMSetInitializer(global, LLVMConstNull(type));
    LLVMSetAlignment(global, 8U);
    if (emitter->target->platform == __Bootstrap_Target_Platform_Darwin__)
        LLVMSetVisibility(global, LLVMHiddenVisibility);
    else
        LLVMSetLinkage(global, LLVMWeakAnyLinkage);
    return global;
}

/* Exposes the semantic entry to the host C runtime without changing its type. */
static int __LLVM_Emit_Process_Entry__(__LLVM_Emitter__ *emitter,
                                      __Semantic_Function_Entry__ *language_entry,
                                      const __Resolved_Type__ *language_result)
{
    LLVMValueRef language_function = NULL;
    LLVMValueRef process_function;
    LLVMTypeRef language_type;
    LLVMTypeRef process_type;
    LLVMTypeRef int32_type;
    LLVMTypeRef process_parameters[4];
    unsigned process_parameter_count = 2U;
    LLVMTypeRef pointer_type;
    LLVMValueRef argument_count = NULL;
    LLVMValueRef argument_vector = NULL;
    LLVMValueRef environment_vector = NULL;
    /* Linux GNU/musl and Darwin CRTs pass envp to main (Darwin also passes an
     * Apple vector). Only those targets receive the canonical startup data. */
    int startup_data;
    LLVMValueRef result;
    LLVMValueRef exit_code;
    LLVMBasicBlockRef entry_block;
    LLVMBuilderRef builder;
    size_t index;

    /* The CRT's main ABI is not WASI's _start or an unknown target's ABI. */
    if (emitter->target->platform != __Bootstrap_Target_Platform_Darwin__ &&
        emitter->target->platform != __Bootstrap_Target_Platform_Linux__ &&
        emitter->target->platform != __Bootstrap_Target_Platform_Windows__)
        return 1;

    for (index = 0U; index < emitter->function_count; ++index)
    {
        if (emitter->functions[index].semantic == language_entry)
        {
            language_function = emitter->functions[index].value;
            language_type = emitter->functions[index].type;
            break;
        }
    }
    if (language_function == NULL ||
        LLVMGetTypeKind(LLVMGetReturnType(language_type)) != LLVMIntegerTypeKind)
        return __LLVM_Fail__("the semantic entry has no integer-returning LLVM function");
    if (LLVMGetNamedFunction(emitter->module, "main") != NULL)
        return __LLVM_Fail__("external symbol main conflicts with the process entry");

    startup_data =
        emitter->target->platform == __Bootstrap_Target_Platform_Darwin__ ||
        (emitter->target->platform == __Bootstrap_Target_Platform_Linux__ &&
         (emitter->target->environment == __Bootstrap_Target_Environment_GNU__ ||
          emitter->target->environment == __Bootstrap_Target_Environment_Musl__));
    int32_type = LLVMIntTypeInContext(emitter->context, 32U);
    pointer_type = LLVMPointerTypeInContext(emitter->context, 0U);
    process_parameters[0] = int32_type;
    process_parameters[1] = pointer_type;
    if (startup_data)
    {
        process_parameters[process_parameter_count++] = pointer_type;
        if (emitter->target->platform == __Bootstrap_Target_Platform_Darwin__)
            process_parameters[process_parameter_count++] = pointer_type;
        /* The canonical startup count is u64; argv and envp are pointers. */
        argument_count = __LLVM_Define_Startup_Global__(
            emitter, "sultanc.bootstrap.argc", LLVMIntTypeInContext(emitter->context, 64U));
        argument_vector =
            __LLVM_Define_Startup_Global__(emitter, "sultanc.bootstrap.argv", pointer_type);
        environment_vector =
            __LLVM_Define_Startup_Global__(emitter, "sultanc.bootstrap.envp", pointer_type);
        if (argument_count == NULL || argument_vector == NULL || environment_vector == NULL)
            return 0;
    }
    process_type =
        LLVMFunctionType(int32_type, process_parameters, process_parameter_count, 0);
    process_function = LLVMAddFunction(emitter->module, "main", process_type);
    if (process_function == NULL)
        return __LLVM_Fail__("could not declare the host process entry");

    builder = LLVMCreateBuilderInContext(emitter->context);
    if (builder == NULL)
        return __LLVM_Fail__("could not create the host process entry builder");
    entry_block = LLVMAppendBasicBlockInContext(
        emitter->context, process_function, "entry");
    LLVMPositionBuilderAtEnd(builder, entry_block);
    if (startup_data)
    {
        /* Process globals are populated before the language entry runs. CRT argc
         * includes argv[0]; it is zero-extended, never adjusted. */
        LLVMBuildStore(builder,
                       LLVMBuildZExt(builder,
                                     LLVMGetParam(process_function, 0U),
                                     LLVMIntTypeInContext(emitter->context, 64U),
                                     "startup.argc"),
                       argument_count);
        LLVMBuildStore(builder, LLVMGetParam(process_function, 1U), argument_vector);
        LLVMBuildStore(builder, LLVMGetParam(process_function, 2U), environment_vector);
    }
    result = LLVMBuildCall2(builder, language_type, language_function, NULL, 0U, "result");
    if (language_result->__Bits__ > 32U)
        exit_code = LLVMBuildTrunc(builder, result, int32_type, "exit.code");
    else if (language_result->__Bits__ < 32U)
        exit_code = language_result->__Kind__ == __Resolved_Type_Signed_Integer__
                        ? LLVMBuildSExt(builder, result, int32_type, "exit.code")
                        : LLVMBuildZExt(builder, result, int32_type, "exit.code");
    else
        exit_code = result;
    LLVMBuildRet(builder, exit_code);
    LLVMDisposeBuilder(builder);
    return 1;
}

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
    free(emitter->function_worklist);
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
    /* The target is borrowed from the driver, which owns its lifetime. */
    emitter->target = NULL;
}

/* Prepares the Stage0 LLVM object module. */
int __LLVM_Prepare_Program_Module__(__Semantic_Context__ *semantic,
                                   const __Bootstrap_Target__ *selected_target,
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
    /* Stores an LLVM target lookup prefix. */
    char target_lookup_error[320];

    memset(emitter, 0, sizeof(*emitter));
    *saved_unit = NULL;
    *machine = NULL;
    *data = NULL;
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

    if (selected_target == NULL || selected_target->triple == NULL)
        return __LLVM_Fail__("missing canonical bootstrap compilation target");
    emitter->target = selected_target;
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

    LLVMSetTarget(emitter->module, emitter->target->triple);
    if (LLVMGetTargetFromTriple(emitter->target->triple, &target, &message))
    {
        snprintf(target_lookup_error,
                 sizeof(target_lookup_error),
                 "LLVM target backend is unavailable for '%s'",
                 emitter->target->triple);
        __LLVM_Fail_Message__(target_lookup_error, message);
        return 0;
    }
    *machine = LLVMCreateTargetMachine(target,
                                       emitter->target->triple,
                                       "",
                                       "",
                                       LLVMCodeGenLevelNone,
                                       /* The Stage1 object is linked by the
                                        * host C linker driver, which may
                                        * default to PIE; position-independent
                                        * code links into PIE and non-PIE
                                        * executables alike. */
                                       LLVMRelocPIC,
                                       LLVMCodeModelDefault);
    if (*machine == NULL)
    {
        snprintf(target_lookup_error,
                 sizeof(target_lookup_error),
                 "LLVM TargetMachine creation failed for '%s'",
                 emitter->target->triple);
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
    if (!__LLVM_Emit_Reachable_Function_Bodies__(emitter, main_entry))
        return 0;
    return __LLVM_Emit_Process_Entry__(emitter, main_entry, &output_resolved);
}
