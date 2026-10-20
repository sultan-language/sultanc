/* Owns Stage0 LLVM builtin lowering helpers. */

#include "llvm/llvm_builtins.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "llvm/runtime/llvm_runtime_memory.h"
#include "kernel/layout/layout.h"
#include "kernel/memory/memory.h"
#include "kernel/name/name.h"
#include "kernel/type/tagged.h"
#include "kernel/type/type.h"
#include "semantic/body_internal.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
            if (emitter->target->architecture == __Bootstrap_Target_Architecture_AArch64__)
            {
                *out = 1ULL;
                return 1;
            }
            if (emitter->target->architecture == __Bootstrap_Target_Architecture_X86_64__)
            {
                *out = 2ULL;
                return 1;
            }
            if (emitter->target->architecture == __Bootstrap_Target_Architecture_Wasm32__)
            {
                *out = 3ULL;
                return 1;
            }
            if (emitter->target->architecture == __Bootstrap_Target_Architecture_Wasm64__)
            {
                *out = 4ULL;
                return 1;
            }
            return __LLVM_Fail__(
                "requested Bootstrap target architecture has no Sultan host-identity code");
        case __Name_Builtin_Host_Platform__:
            if (emitter->target->platform == __Bootstrap_Target_Platform_Darwin__)
            {
                *out = 1ULL;
                return 1;
            }
            if (emitter->target->platform == __Bootstrap_Target_Platform_Linux__)
            {
                *out = 2ULL;
                return 1;
            }
            if (emitter->target->platform == __Bootstrap_Target_Platform_Wasi__)
            {
                *out = 3ULL;
                return 1;
            }
            return __LLVM_Fail__(
                "requested Bootstrap target platform has no Sultan host-identity code");
        case __Name_Builtin_Host_Environment__:
            if (emitter->target->environment == __Bootstrap_Target_Environment_Unknown__)
            {
                *out = 0ULL;
                return 1;
            }
            if (emitter->target->environment == __Bootstrap_Target_Environment_GNU__)
            {
                *out = 1ULL;
                return 1;
            }
            if (emitter->target->environment == __Bootstrap_Target_Environment_Musl__)
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

/* Lowers the queried bootstrap target identity. */
static __LLVM_Value__ __LLVM_Emit_Host_Identity__(__LLVM_Emitter__ *emitter,
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


/* Returns the LLVM builtin call identity. */
__Name_Builtin_Function__ __LLVM_Builtin_Call_Identity__(__LLVM_Emitter__ *emitter,
                                                                __Ast_Expression__ *expression)
{
    /* References the function. */
    __Ast_Lvalue__ *function;
    /* Stores the name. */
    __Text_Slice__ name;
    /* Stores the resolved name. */
    __Text_Slice__ resolved_name;
    /* Stores the direct. */
    __Name_Builtin_Function__ direct;
    /* References the unit. */
    const __Program_Unit__ *unit;
    if (expression == NULL || expression->__Kind__ != __Ast_Expression_Call__)
        return __Name_Builtin_None__;
    function = expression->__As__.__Call__.__Function__;
    if (function == NULL || function->__Kind__ != __Ast_Lvalue_Base__ ||
        function->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__)
        return __Name_Builtin_None__;
    name = function->__As__.__Base__.__As__.__Identifier__;
    if (__LLVM_Find_Local__(emitter, name) != NULL)
        return __Name_Builtin_None__;

    direct = __Name_Find_Builtin_Function__(name);
    if (direct != __Name_Builtin_None__ && __Name_Builtin_Is_Direct_Source__(direct))
        return direct;

    unit = (emitter->current_function == NULL) ? NULL : emitter->current_function->__Unit__;
    resolved_name = name;
    if (unit != NULL)
        (void)__Name_Resolve_Module_Alias_Name__(unit, name, &resolved_name);

    direct = __Name_Find_Builtin_Function__(resolved_name);
    if (direct != __Name_Builtin_None__)
        return direct;

    return __Name_Builtin_None__;
}


/* Emits the LLVM builtin call. */
__LLVM_Value__ __LLVM_Emit_Builtin_Call__(__LLVM_Emitter__ *emitter,
                                          __Ast_Expression__ *expression,
                                          __Ast_Type__ *expected,
                                          __Name_Builtin_Function__ builtin)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    if (builtin == __Name_Builtin_Host_Architecture__ ||
        builtin == __Name_Builtin_Host_Platform__ ||
        builtin == __Name_Builtin_Host_Environment__)
        return __LLVM_Emit_Host_Identity__(emitter, builtin, expected);
    if (builtin == __Name_Builtin_F64_Bits__ ||
        builtin == __Name_Builtin_F64_From_Bits__ ||
        builtin == __Name_Builtin_F32_Bits__ ||
        builtin == __Name_Builtin_F32_From_Bits__)
    {
        __Ast_Type__ *argument_type = NULL;
        __Ast_Type__ *result_type;
        __LLVM_Value__ argument;
        LLVMTypeRef llvm_result_type;
        if (expression->__As__.__Call__.__Argument_Count__ != 1U ||
            (argument_type = __LLVM_Expression_Type__(
                emitter, expression->__As__.__Call__.__Arguments__[0])) == NULL)
        {
            __LLVM_Fail__("scalar bit builtin has no verified argument type");
            return result;
        }
        /* Stage0 uses its canonical machine Type descriptors, not host C floats. */
        result_type = (builtin == __Name_Builtin_F64_Bits__)
                          ? &__Body_Builtin_U64_Type__
                          : (builtin == __Name_Builtin_F64_From_Bits__)
                                ? &__Body_Builtin_F64_Type__
                                : (builtin == __Name_Builtin_F32_Bits__)
                                      ? &__Body_Builtin_U32_Type__
                                      : &__Body_Builtin_F32_Type__;
        llvm_result_type = __LLVM_Type__(emitter, result_type);
        if (llvm_result_type == NULL)
            return result;
        argument = __LLVM_Emit_Expression__(
            emitter, expression->__As__.__Call__.__Arguments__[0], argument_type);
        if (argument.value == NULL)
            return result;
        {
            LLVMTypeKind operand_kind = LLVMGetTypeKind(LLVMTypeOf(argument.value));
            LLVMTypeKind output_kind = LLVMGetTypeKind(llvm_result_type);
            int canonical = 0;
            if (builtin == __Name_Builtin_F64_Bits__)
                canonical = operand_kind == LLVMDoubleTypeKind &&
                            output_kind == LLVMIntegerTypeKind &&
                            LLVMGetIntTypeWidth(llvm_result_type) == 64U;
            else if (builtin == __Name_Builtin_F64_From_Bits__)
                canonical = operand_kind == LLVMIntegerTypeKind &&
                            LLVMGetIntTypeWidth(LLVMTypeOf(argument.value)) == 64U &&
                            output_kind == LLVMDoubleTypeKind;
            else if (builtin == __Name_Builtin_F32_Bits__)
                canonical = operand_kind == LLVMFloatTypeKind &&
                            output_kind == LLVMIntegerTypeKind &&
                            LLVMGetIntTypeWidth(llvm_result_type) == 32U;
            else if (builtin == __Name_Builtin_F32_From_Bits__)
                canonical = operand_kind == LLVMIntegerTypeKind &&
                            LLVMGetIntTypeWidth(LLVMTypeOf(argument.value)) == 32U &&
                            output_kind == LLVMFloatTypeKind;
            if (!canonical)
            {
                __LLVM_Fail__("scalar bit builtin received a noncanonical type or width");
                return result;
            }
        }
        result.value = LLVMBuildBitCast(emitter->builder,
                                        argument.value, llvm_result_type, "scalar.bits");
        result.type = result_type;
        return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
    }
    if (builtin == __Name_Builtin_Raw_Kernel_Call__)
    {
        /* Mirrors canonical ابن_نداء_نواة_خام_إل_إل_في_إم
         * (compiler/backend/provider/llvm/lowering.sn): seven i64 operands,
         * pointers converted with ptrtoint, side-effecting inline asm. The
         * selected target, never the host, decides the ABI. */
        static const char x86_asm[] = "syscall";
        static const char x86_constraints[] =
            "={rax},0,{rdi},{rsi},{rdx},{r10},{r8},{r9},~{rcx},~{r11},~{flags},~{memory}";
        static const char arm_asm[] = "svc #0";
        static const char arm_constraints[] =
            "={x0},{x8},0,{x1},{x2},{x3},{x4},{x5},~{memory},~{cc}";
        const char *asm_text;
        const char *constraints;
        LLVMTypeRef word = LLVMIntTypeInContext(emitter->context, 64U);
        LLVMTypeRef operand_types[7];
        LLVMValueRef operands[7];
        LLVMTypeRef signature;
        LLVMValueRef inline_asm;
        size_t index;
        if (emitter->target == NULL ||
            emitter->target->platform != __Bootstrap_Target_Platform_Linux__ ||
            expression->__As__.__Call__.__Argument_Count__ != 7U)
        {
            __LLVM_Fail__("raw kernel call is supported only for registered 64-bit Linux targets");
            return result;
        }
        if (emitter->target->architecture == __Bootstrap_Target_Architecture_X86_64__)
        {
            asm_text = x86_asm;
            constraints = x86_constraints;
        }
        else if (emitter->target->architecture == __Bootstrap_Target_Architecture_AArch64__)
        {
            asm_text = arm_asm;
            constraints = arm_constraints;
        }
        else
        {
            __LLVM_Fail__("raw kernel call is supported only for registered 64-bit Linux targets");
            return result;
        }
        for (index = 0U; index < 7U; ++index)
        {
            __Ast_Expression__ *argument = expression->__As__.__Call__.__Arguments__[index];
            __Ast_Type__ *argument_type = __LLVM_Expression_Type__(emitter, argument);
            __LLVM_Value__ value;
            LLVMTypeRef value_type;
            if (argument_type == NULL)
                argument_type = &__Body_Builtin_U64_Type__;
            value = __LLVM_Emit_Expression__(emitter, argument, argument_type);
            if (value.value == NULL)
                return result;
            value_type = LLVMTypeOf(value.value);
            if (LLVMGetTypeKind(value_type) == LLVMPointerTypeKind)
            {
                value.value = LLVMBuildPtrToInt(emitter->builder, value.value, word, "kernel.ptr");
            }
            else if (LLVMGetTypeKind(value_type) != LLVMIntegerTypeKind ||
                     LLVMGetIntTypeWidth(value_type) != 64U)
            {
                __LLVM_Fail__("raw kernel call operand is not a 64-bit word or pointer");
                return result;
            }
            operand_types[index] = word;
            operands[index] = value.value;
        }
        signature = LLVMFunctionType(word, operand_types, 7U, 0);
        inline_asm = LLVMGetInlineAsm(signature, (char *)asm_text, strlen(asm_text),
                                      (char *)constraints, strlen(constraints),
                                      1, 0, LLVMInlineAsmDialectATT, 0);
        result.value = LLVMBuildCall2(emitter->builder, signature, inline_asm, operands, 7U,
                                      "kernel.result");
        result.type = &__Body_Builtin_U64_Type__;
        return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
    }
    if (builtin == __Name_Builtin_Release__)
    {
        /* Compiler-side release requires STD's canonical ownership/disposal
         * provider. Never silently lower this to free() or a no-op. */
        __LLVM_Fail__("release requires the canonical runtime disposal provider");
        return result;
    }
    if (builtin == __Name_Builtin_Length__)
    {
        /* References the argument type. */
        __Ast_Type__ *argument_type;
        /* Stores the resolved. */
        __Resolved_Type__ resolved;
        /* Stores the sequence. */
        __LLVM_Value__ sequence;
        if (expression->__As__.__Call__.__Argument_Count__ != 1U ||
            (argument_type = __LLVM_Expression_Type__(
                 emitter, expression->__As__.__Call__.__Arguments__[0])) == NULL ||
            !__Type_Resolve__(emitter->semantic, argument_type, &resolved) ||
            (resolved.__Kind__ != __Resolved_Type_Vector__ &&
             resolved.__Kind__ != __Resolved_Type_String__))
        {
            __LLVM_Fail__("L2.7 length call disagrees with canonical builtin semantics");
            return result;
        }
        sequence = __LLVM_Emit_Expression__(
            emitter, expression->__As__.__Call__.__Arguments__[0], argument_type);
        if (sequence.value == NULL)
            return result;
        result.value =
            LLVMBuildExtractValue(emitter->builder, sequence.value, 1U, "sequence.length");
        result.type = &__LLVM_Integer_Type__;
        return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
    }
    if (builtin == __Name_Builtin_Swap__)
    {
        __Ast_Expression__ *left_expression;
        __Ast_Expression__ *right_expression;
        __Ast_Lvalue__ *left_lvalue;
        __Ast_Lvalue__ *right_lvalue;
        __LLVM_Place__ left_place;
        __LLVM_Place__ right_place;
        LLVMValueRef left_value;
        LLVMValueRef right_value;
        LLVMValueRef last_store;

        if (expression->__As__.__Call__.__Argument_Count__ != 2U ||
            (left_expression = expression->__As__.__Call__.__Arguments__[0]) == NULL ||
            (right_expression = expression->__As__.__Call__.__Arguments__[1]) == NULL ||
            left_expression->__Kind__ != __Ast_Expression_Atom__ ||
            right_expression->__Kind__ != __Ast_Expression_Atom__ ||
            left_expression->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
            right_expression->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
            (left_lvalue = left_expression->__As__.__Atom__.__As__.__Lvalue__) == NULL ||
            (right_lvalue = right_expression->__As__.__Atom__.__As__.__Lvalue__) == NULL)
        {
            __LLVM_Fail__("L2.7 swap disagrees with canonical builtin semantics");
            return result;
        }
        left_place = __LLVM_Emit_Place__(emitter, left_lvalue);
        right_place = __LLVM_Emit_Place__(emitter, right_lvalue);
        if (left_place.address == NULL || right_place.address == NULL ||
            left_place.type == NULL || right_place.type == NULL ||
            left_place.llvm_type == NULL || right_place.llvm_type == NULL ||
            !__Type_Compatible__(emitter->semantic, left_place.type, right_place.type) ||
            !__Type_Compatible__(emitter->semantic, right_place.type, left_place.type))
        {
            __LLVM_Fail__("L2.7 swap requires two compatible mutable places");
            return result;
        }
        left_value = LLVMBuildLoad2(
            emitter->builder, left_place.llvm_type, left_place.address, "swap.left");
        right_value = LLVMBuildLoad2(
            emitter->builder, right_place.llvm_type, right_place.address, "swap.right");
        LLVMBuildStore(emitter->builder, right_value, left_place.address);
        last_store = LLVMBuildStore(emitter->builder, left_value, right_place.address);
        result.value = last_store;
        result.type = &__LLVM_Void_Type__;
        return result;
    }
    if (builtin == __Name_Builtin_Append__)
    {
        /* References the vector expression. */
        __Ast_Expression__ *vector_expression;
        /* References the vector lvalue. */
        __Ast_Lvalue__ *vector_lvalue;
        /* Stores the vector place. */
        __LLVM_Place__ vector_place;
        /* Stores the vector resolved. */
        __Resolved_Type__ vector_resolved;
        /* Stores the element type. */
        LLVMTypeRef element_type;
        /* Stores the element pointer type. */
        LLVMTypeRef element_pointer_type;
        /* Stores the LLVM i64 type. */
        LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
        /* Stores the data field. */
        LLVMValueRef data_field;
        /* Stores the length field. */
        LLVMValueRef length_field;
        /* Stores the capacity field. */
        LLVMValueRef capacity_field;
        /* Stores the data. */
        LLVMValueRef data;
        /* Stores the length. */
        LLVMValueRef length;
        /* Stores the capacity. */
        LLVMValueRef capacity;
        /* Stores the one. */
        LLVMValueRef one = LLVMConstInt(i64, 1U, 0);
        /* Stores the required. */
        LLVMValueRef required;
        /* Stores the length overflow. */
        LLVMValueRef length_overflow;
        /* Stores the need grow. */
        LLVMValueRef need_grow;
        /* Stores the function. */
        LLVMValueRef function;
        /* Stores the current. */
        LLVMBasicBlockRef current;
        /* Stores the grow block. */
        LLVMBasicBlockRef grow_block;
        /* Stores the append block. */
        LLVMBasicBlockRef append_block;
        /* References the policy. */
        const __Memory_Vector_Growth_Policy__ *policy;
        /* Stores the element size. */
        size_t element_size = 0U;
        /* Stores the element alignment. */
        size_t element_alignment = 1U;
        /* Stores the element. */
        __LLVM_Value__ element;

        if (expression->__As__.__Call__.__Argument_Count__ != 2U ||
            (vector_expression = expression->__As__.__Call__.__Arguments__[0]) == NULL)
        {
            __LLVM_Fail__("L2.7 append disagrees with canonical builtin semantics");
            return result;
        }
        if (vector_expression->__Kind__ == __Ast_Expression_Atom__ &&
            vector_expression->__As__.__Atom__.__Kind__ == __Ast_Atom_Lvalue__ &&
            (vector_lvalue = vector_expression->__As__.__Atom__.__As__.__Lvalue__) != NULL)
        {
            vector_place = __LLVM_Emit_Place__(emitter, vector_lvalue);
        }
        else if (vector_expression->__Kind__ == __Ast_Expression_Unary__ &&
                 vector_expression->__As__.__Unary__.__Operation__ == __Unary_Dereference__)
        {
            __LLVM_Value__ reference = __LLVM_Emit_Expression__(
                emitter, vector_expression->__As__.__Unary__.__Operand__, NULL);
            __Resolved_Type__ reference_resolved;
            if (reference.value == NULL || reference.type == NULL ||
                !__Type_Resolve__(emitter->semantic, reference.type, &reference_resolved) ||
                reference_resolved.__Kind__ != __Resolved_Type_Reference__ ||
                !reference_resolved.__Reference_Mutable__ ||
                reference_resolved.__Inner__ == NULL)
            {
                __LLVM_Fail__("L2.7 append dereference requires mutable reference storage");
                return result;
            }
            vector_place.address = reference.value;
            vector_place.type = reference_resolved.__Inner__;
            vector_place.llvm_type = __LLVM_Type__(emitter, vector_place.type);
        }
        else
        {
            __LLVM_Fail__("L2.7 append disagrees with canonical builtin semantics");
            return result;
        }
        if (vector_place.address == NULL || vector_place.type == NULL ||
            !__Type_Resolve__(emitter->semantic, vector_place.type, &vector_resolved) ||
            vector_resolved.__Kind__ != __Resolved_Type_Vector__ ||
            vector_resolved.__Inner__ == NULL ||
            !__Layout_Type__(emitter->semantic,
                             vector_resolved.__Inner__,
                             &element_size,
                             &element_alignment) ||
            element_size == 0U)
        {
            __LLVM_Fail__("L2.7 append requires canonical mutable vector storage");
            return result;
        }
        (void)element_alignment;
        policy = __Memory_Vector_Growth_Policy_View__();
        if (policy == NULL || policy->__Minimum_Capacity__ == 0U ||
            policy->__Growth_Factor__ < 2U)
        {
            __LLVM_Fail__("L2.7 append could not consume canonical vector growth policy");
            return result;
        }
        element_type = __LLVM_Type__(emitter, vector_resolved.__Inner__);
        if (element_type == NULL)
            return result;
        element_pointer_type = LLVMPointerType(element_type, 0U);
        element = __LLVM_Emit_Expression__(emitter,
                                           expression->__As__.__Call__.__Arguments__[1],
                                           vector_resolved.__Inner__);
        if (element.value == NULL)
            return result;
        element = __LLVM_Coerce__(emitter, element, vector_resolved.__Inner__);
        if (element.value == NULL)
            return result;

        data_field = LLVMBuildStructGEP2(emitter->builder,
                                         vector_place.llvm_type,
                                         vector_place.address,
                                         0U,
                                         "append.data.field");
        length_field = LLVMBuildStructGEP2(emitter->builder,
                                           vector_place.llvm_type,
                                           vector_place.address,
                                           1U,
                                           "append.length.field");
        capacity_field = LLVMBuildStructGEP2(emitter->builder,
                                             vector_place.llvm_type,
                                             vector_place.address,
                                             2U,
                                             "append.capacity.field");
        data = LLVMBuildLoad2(
            emitter->builder, element_pointer_type, data_field, "append.data");
        length = LLVMBuildLoad2(emitter->builder, i64, length_field, "append.length");
        capacity = LLVMBuildLoad2(emitter->builder, i64, capacity_field, "append.capacity");
        required = LLVMBuildAdd(emitter->builder, length, one, "append.required");
        length_overflow = LLVMBuildICmp(
            emitter->builder, LLVMIntULT, required, length, "append.length.overflow");
        if (!__LLVM_Emit_Trap_If__(emitter, length_overflow, "vector.length.overflow"))
            return result;
        need_grow = LLVMBuildICmp(
            emitter->builder, LLVMIntUGT, required, capacity, "append.needs.grow");
        current = LLVMGetInsertBlock(emitter->builder);
        function = current != NULL ? LLVMGetBasicBlockParent(current) : NULL;
        if (function == NULL)
        {
            __LLVM_Fail__("L2.7 append lost current LLVM function");
            return result;
        }
        grow_block =
            LLVMAppendBasicBlockInContext(emitter->context, function, "append.grow");
        append_block =
            LLVMAppendBasicBlockInContext(emitter->context, function, "append.store");
        LLVMBuildCondBr(emitter->builder, need_grow, grow_block, append_block);

        LLVMPositionBuilderAtEnd(emitter->builder, grow_block);
        {
            /* Stores the zero. */
            LLVMValueRef zero = LLVMConstInt(i64, 0U, 0);
            /* Tracks whether the capacity is is zero. */
            LLVMValueRef capacity_is_zero = LLVMBuildICmp(
                emitter->builder, LLVMIntEQ, capacity, zero, "append.capacity.zero");
            /* Stores the factor. */
            LLVMValueRef factor =
                LLVMConstInt(i64, (unsigned long long)policy->__Growth_Factor__, 0);
            /* Stores the min capacity. */
            LLVMValueRef min_capacity =
                LLVMConstInt(i64, (unsigned long long)policy->__Minimum_Capacity__, 0);
            /* Stores the max before growth. */
            LLVMValueRef max_before_growth = LLVMConstInt(
                i64, UINT64_MAX / (unsigned long long)policy->__Growth_Factor__, 0);
            /* Stores the growth overflow. */
            LLVMValueRef growth_overflow = LLVMBuildICmp(emitter->builder,
                                                         LLVMIntUGT,
                                                         capacity,
                                                         max_before_growth,
                                                         "append.capacity.overflow");
            /* Stores the grown. */
            LLVMValueRef grown;
            /* Stores the new capacity. */
            LLVMValueRef new_capacity;
            /* Stores the max before bytes. */
            LLVMValueRef max_before_bytes =
                LLVMConstInt(i64, UINT64_MAX / (unsigned long long)element_size, 0);
            /* Stores the bytes overflow. */
            LLVMValueRef bytes_overflow;
            /* Stores the byte count. */
            LLVMValueRef byte_count;
            /* Stores the new data. */
            LLVMValueRef new_data;
            if (!__LLVM_Emit_Trap_If__(
                    emitter, growth_overflow, "vector.capacity.overflow"))
                return result;
            grown =
                LLVMBuildMul(emitter->builder, capacity, factor, "append.grown.capacity");
            new_capacity = LLVMBuildSelect(emitter->builder,
                                           capacity_is_zero,
                                           min_capacity,
                                           grown,
                                           "append.new.capacity");
            /* Growth factor >= 2 covers one append unless arithmetic overflowed. */
            bytes_overflow = LLVMBuildICmp(emitter->builder,
                                           LLVMIntUGT,
                                           new_capacity,
                                           max_before_bytes,
                                           "append.bytes.overflow");
            if (!__LLVM_Emit_Trap_If__(
                    emitter, bytes_overflow, "vector.backing.bytes.overflow"))
                return result;
            byte_count =
                LLVMBuildMul(emitter->builder,
                             new_capacity,
                             LLVMConstInt(i64, (unsigned long long)element_size, 0),
                             "append.backing.bytes");
            new_data = __LLVM_Reallocate_Bytes__(
                emitter, data, byte_count, element_pointer_type, "append.new.data");
            if (new_data == NULL)
                return result;
            LLVMBuildStore(emitter->builder, new_data, data_field);
            LLVMBuildStore(emitter->builder, new_capacity, capacity_field);
            LLVMBuildBr(emitter->builder, append_block);
        }

        LLVMPositionBuilderAtEnd(emitter->builder, append_block);
        data = LLVMBuildLoad2(
            emitter->builder, element_pointer_type, data_field, "append.current.data");
        {
            /* Stores the element place. */
            LLVMValueRef element_place = LLVMBuildGEP2(
                emitter->builder, element_type, data, &length, 1U, "append.element.place");
            /* Stores the store. */
            LLVMValueRef store =
                LLVMBuildStore(emitter->builder, element.value, element_place);
            LLVMBuildStore(emitter->builder, required, length_field);
            result.value = store;
            result.type = &__LLVM_Void_Type__;
            return result;
        }
    }
    if (builtin != __Name_Builtin_None__)
    {
        __LLVM_Fail__("canonical builtin is outside the native runtime lowering boundary");
        return result;
    }
    return result;
}

/* Resolves the LLVM builtin tagged construct. */
int __LLVM_Resolve_Builtin_Tagged_Construct__(__Ast_Type__ *target_type,
                                                     __Ast_Expression__ *expression,
                                                     __Type_Tagged_Constructor__ *out_constructor)
{
    /* References the function. */
    __Ast_Lvalue__ *function;
    /* References the parent. */
    __Ast_Lvalue__ *parent;
    if (target_type == NULL || expression == NULL ||
        expression->__Kind__ != __Ast_Expression_Call__ || !__Type_Is_Builtin_Tagged__(target_type))
    {
        return 0;
    }
    function = expression->__As__.__Call__.__Function__;
    if (function == NULL || function->__Kind__ != __Ast_Lvalue_Field__)
    {
        return 0;
    }
    parent = function->__As__.__Field__.__Parent__;
    if (parent == NULL || parent->__Kind__ != __Ast_Lvalue_Base__ ||
        parent->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__ ||
        !__Type_Tagged_Name_Matches__(target_type, parent->__As__.__Base__.__As__.__Identifier__))
    {
        return 0;
    }
    return __Type_Tagged_Find_Constructor__(
        target_type, function->__As__.__Field__.__Field__, out_constructor);
}
