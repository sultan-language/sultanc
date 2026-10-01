/* Owns the existing Stage0 LLVM optimization and SROA pipeline. */

#include "llvm/llvm_optimization.h"
#include "llvm/llvm_context.h"
#include <stdlib.h>

/* Returns whether an LLVM type is an aggregate passed by value. */
static int __LLVM_Type_Is_Direct_Aggregate__(LLVMTypeRef type)
{
    LLVMTypeKind kind;
    if (type == NULL)
        return 0;
    kind = LLVMGetTypeKind(type);
    return kind == LLVMStructTypeKind || kind == LLVMArrayTypeKind;
}

/* Returns whether SROA must preserve this function's aggregate ABI surface. */
static int __LLVM_Function_Has_Direct_Aggregate_Signature__(LLVMValueRef function)
{
    LLVMTypeRef function_type;
    LLVMTypeRef return_type;
    LLVMTypeRef *parameters = NULL;
    unsigned count;
    unsigned index;
    int found = 0;

    if (function == NULL)
        return 0;
    function_type = LLVMGlobalGetValueType(function);
    if (function_type == NULL || LLVMGetTypeKind(function_type) != LLVMFunctionTypeKind)
        return 0;
    return_type = LLVMGetReturnType(function_type);
    if (__LLVM_Type_Is_Direct_Aggregate__(return_type))
        return 1;
    count = LLVMCountParamTypes(function_type);
    if (count == 0)
        return 0;
    parameters = (LLVMTypeRef *)calloc((size_t)count, sizeof(*parameters));
    if (parameters == NULL)
        return 1;
    LLVMGetParamTypes(function_type, parameters);
    for (index = 0; index < count; ++index)
    {
        if (__LLVM_Type_Is_Direct_Aggregate__(parameters[index]))
        {
            found = 1;
            break;
        }
    }
    free(parameters);
    return found;
}

/* Temporarily excludes direct-aggregate ABI functions from SROA. */
static int __LLVM_Set_Aggregate_SROA_Guard__(__LLVM_Emitter__ *emitter, int enabled)
{
    LLVMValueRef function;
    unsigned optnone_kind;
    unsigned noinline_kind;

    if (emitter == NULL || emitter->module == NULL || emitter->context == NULL)
        return __LLVM_Fail__("invalid LLVM SROA guard request");
    optnone_kind = LLVMGetEnumAttributeKindForName("optnone", 7U);
    noinline_kind = LLVMGetEnumAttributeKindForName("noinline", 8U);
    if (optnone_kind == 0 || noinline_kind == 0)
        return __LLVM_Fail__("LLVM aggregate SROA guard attributes are unavailable");
    function = LLVMGetFirstFunction(emitter->module);
    while (function != NULL)
    {
        if (LLVMCountBasicBlocks(function) != 0 &&
            __LLVM_Function_Has_Direct_Aggregate_Signature__(function))
        {
            if (enabled)
            {
                LLVMAttributeRef noinline_attribute =
                    LLVMCreateEnumAttribute(emitter->context, noinline_kind, 0);
                LLVMAttributeRef optnone_attribute =
                    LLVMCreateEnumAttribute(emitter->context, optnone_kind, 0);
                if (noinline_attribute == NULL || optnone_attribute == NULL)
                    return __LLVM_Fail__("LLVM aggregate SROA guard creation failed");
                LLVMAddAttributeAtIndex(function, LLVMAttributeFunctionIndex, noinline_attribute);
                LLVMAddAttributeAtIndex(function, LLVMAttributeFunctionIndex, optnone_attribute);
            }
            else
            {
                LLVMRemoveEnumAttributeAtIndex(function, LLVMAttributeFunctionIndex, optnone_kind);
                LLVMRemoveEnumAttributeAtIndex(function, LLVMAttributeFunctionIndex, noinline_kind);
            }
        }
        function = LLVMGetNextFunction(function);
    }
    return 1;
}

/* Promotes locals while keeping direct aggregate ABI functions out of SROA. */
int __LLVM_Optimize_Module__(__LLVM_Emitter__ *emitter, LLVMTargetMachineRef machine)
{
    LLVMPassBuilderOptionsRef options;
    LLVMErrorRef error;

    if (emitter == NULL || emitter->module == NULL || machine == NULL)
        return __LLVM_Fail__("invalid LLVM optimization request");
    options = LLVMCreatePassBuilderOptions();
    if (options == NULL)
        return __LLVM_Fail__("LLVM pass builder options creation failed");
    error = LLVMRunPasses(emitter->module, "function(mem2reg)", machine, options);
    if (error != NULL)
    {
        LLVMDisposePassBuilderOptions(options);
        return __LLVM_Fail_Error__("LLVM local SSA optimization failed", error);
    }
    if (!__LLVM_Set_Aggregate_SROA_Guard__(emitter, 1))
    {
        LLVMDisposePassBuilderOptions(options);
        return 0;
    }
    error = LLVMRunPasses(emitter->module, "function(sroa)", machine, options);
    if (!__LLVM_Set_Aggregate_SROA_Guard__(emitter, 0))
    {
        LLVMDisposePassBuilderOptions(options);
        if (error != NULL)
            LLVMConsumeError(error);
        return 0;
    }
    LLVMDisposePassBuilderOptions(options);
    if (error != NULL)
        return __LLVM_Fail_Error__("LLVM scalar replacement optimization failed", error);
    return 1;
}
