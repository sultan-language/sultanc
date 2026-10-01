/* Owns existing Stage0 LLVM runtime stream bridges. */

#include "llvm/runtime/llvm_runtime_stream.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "llvm/runtime/llvm_runtime_memory.h"

/* Emits the LLVM runtime stream. */
__LLVM_Value__ __LLVM_Emit_Runtime_Stream__(__LLVM_Emitter__ *emitter,
                                                   __Ast_Expression__ *expression,
                                                   __Ast_Type__ *expected,
                                                   int descriptor)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the text. */
    __LLVM_Value__ text;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[3] = {i32, i8_pointer, i64};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the call arguments. */
    LLVMValueRef arguments[3];
    /* Stores the written. */
    LLVMValueRef written;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the complete. */
    LLVMValueRef complete;
    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("stdout/stderr disagrees with canonical builtin arity");
        return result;
    }
    text = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_String_Type__);
    if (text.value == NULL)
        return result;
    function =
        __LLVM_Declare_Runtime_Function__(emitter, "write", i64, parameters, 3U, &function_type);
    if (function == NULL)
        return result;
    length = LLVMBuildExtractValue(emitter->builder, text.value, 1U, "runtime.stream.length");
    arguments[0] = LLVMConstInt(i32, (unsigned)descriptor, 0);
    arguments[1] = LLVMBuildExtractValue(emitter->builder, text.value, 0U, "runtime.stream.data");
    arguments[2] = length;
    written = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 3U, "runtime.stream.written");
    complete =
        LLVMBuildICmp(emitter->builder, LLVMIntEQ, written, length, "runtime.stream.complete");
    result.value = LLVMBuildSelect(emitter->builder,
                                   complete,
                                   written,
                                   LLVMConstInt(i64, (unsigned long long)-1LL, 1),
                                   "runtime.stream.status");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}
