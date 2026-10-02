/* Owns existing Stage0 LLVM runtime process bridges. */

#include "llvm/runtime/llvm_runtime_process.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "llvm/runtime/llvm_runtime_memory.h"
#include <string.h>

/* Returns the LLVM ensure process globals. */
int __LLVM_Ensure_Process_Globals__(__LLVM_Emitter__ *emitter)
{
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8;
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64;
    /* Stores the argv type. */
    LLVMTypeRef argv_type;
    if (emitter->process_argc_global != NULL && emitter->process_argv_global != NULL)
        return 1;
    i8 = LLVMIntTypeInContext(emitter->context, 8U);
    i64 = LLVMIntTypeInContext(emitter->context, 64U);
    argv_type = LLVMPointerType(LLVMPointerType(i8, 0U), 0U);
    emitter->process_argc_global = LLVMAddGlobal(emitter->module, i64, "sultanc.bootstrap.argc");
    emitter->process_argv_global =
        LLVMAddGlobal(emitter->module, argv_type, "sultanc.bootstrap.argv");
    if (emitter->process_argc_global == NULL || emitter->process_argv_global == NULL)
        return __LLVM_Fail__("cannot create Bootstrap process argument bridge");
    LLVMSetInitializer(emitter->process_argc_global, LLVMConstInt(i64, 0U, 0));
    LLVMSetInitializer(emitter->process_argv_global, LLVMConstNull(argv_type));
    return 1;
}

/* Returns the LLVM strlen. */
LLVMValueRef __LLVM_Strlen__(__LLVM_Emitter__ *emitter, LLVMValueRef data)
{
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(LLVMIntTypeInContext(emitter->context, 8U), 0U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[1];
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    if (emitter->strlen_function == NULL)
    {
        parameters[0] = i8_pointer;
        emitter->strlen_type = LLVMFunctionType(i64, parameters, 1U, 0);
        emitter->strlen_function = LLVMAddFunction(emitter->module, "strlen", emitter->strlen_type);
        if (emitter->strlen_function == NULL)
        {
            __LLVM_Fail__("cannot declare strlen for Bootstrap argument bridge");
            return NULL;
        }
    }
    arguments[0] = data;
    return LLVMBuildCall2(emitter->builder,
                          emitter->strlen_type,
                          emitter->strlen_function,
                          arguments,
                          1U,
                          "argument.length");
}

/* Emits the LLVM argument count. */
__LLVM_Value__ __LLVM_Emit_Argument_Count__(__LLVM_Emitter__ *emitter,
                                                   __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    if (!__LLVM_Ensure_Process_Globals__(emitter))
        return result;
    result.value =
        LLVMBuildLoad2(emitter->builder, i64, emitter->process_argc_global, "argument.count");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Returns the LLVM bootstrap target identity code embedded in generated code. */
static int __LLVM_Bootstrap_Host_Identity_Code__(__LLVM_Emitter__ *emitter,
                                                 __Name_Builtin_Function__ builtin,
                                                 unsigned long long *out)
{
    if (emitter == NULL || out == NULL)
        return 0;
    switch (builtin)
    {
        case __Name_Builtin_Host_Architecture__:
            if (emitter->target.architecture == __Bootstrap_Target_Architecture_AArch64__)
            {
                *out = 1ULL;
                return 1;
            }
            if (emitter->target.architecture == __Bootstrap_Target_Architecture_X86_64__)
            {
                *out = 2ULL;
                return 1;
            }
            return __LLVM_Fail__(
                "requested Bootstrap target architecture has no Sultan host-identity code");
        case __Name_Builtin_Host_Platform__:
            if (emitter->target.platform == __Bootstrap_Target_Platform_Darwin__)
            {
                *out = 1ULL;
                return 1;
            }
            if (emitter->target.platform == __Bootstrap_Target_Platform_Linux__)
            {
                *out = 2ULL;
                return 1;
            }
            return __LLVM_Fail__(
                "requested Bootstrap target platform has no Sultan host-identity code");
        case __Name_Builtin_Host_Environment__:
            if (emitter->target.environment == __Bootstrap_Target_Environment_Unknown__)
            {
                *out = 0ULL;
                return 1;
            }
            if (emitter->target.environment == __Bootstrap_Target_Environment_GNU__)
            {
                *out = 1ULL;
                return 1;
            }
            if (emitter->target.environment == __Bootstrap_Target_Environment_Musl__)
            {
                *out = 2ULL;
                return 1;
            }
            return __LLVM_Fail__(
                "requested Bootstrap target environment has no Sultan host-identity code");
        default:
            return __LLVM_Fail__("non-host builtin used for Bootstrap host identity");
    }
}

/* Emits the LLVM host identity. */
__LLVM_Value__ __LLVM_Emit_Host_Identity__(__LLVM_Emitter__ *emitter,
                                                  __Name_Builtin_Function__ builtin,
                                                  __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the code. */
    unsigned long long code = 0ULL;
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64;
    if (!__LLVM_Bootstrap_Host_Identity_Code__(emitter, builtin, &code))
        return result;
    i64 = LLVMIntTypeInContext(emitter->context, 64U);
    result.value = LLVMConstInt(i64, code, 0);
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime exit. */
__LLVM_Value__ __LLVM_Emit_Runtime_Exit__(__LLVM_Emitter__ *emitter,
                                                 __Ast_Expression__ *expression)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the status. */
    __LLVM_Value__ status;
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[1] = {i32};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("exit disagrees with canonical builtin arity");
        return result;
    }
    status = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
    if (status.value == NULL)
        return result;
    status = __LLVM_Coerce__(emitter, status, &__LLVM_Integer_Type__);
    if (status.value == NULL)
        return result;
    function = __LLVM_Declare_Runtime_Function__(
        emitter, "exit", LLVMVoidTypeInContext(emitter->context), parameters, 1U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] = LLVMBuildTrunc(emitter->builder, status.value, i32, "runtime.exit.status");
    result.value = LLVMBuildCall2(emitter->builder, function_type, function, arguments, 1U, "");
    result.type = &__LLVM_Void_Type__;
    return result;
}
