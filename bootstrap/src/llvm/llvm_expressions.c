/* Owns Stage0 LLVM expression lowering. */

#include "llvm/llvm_expressions.h"
#include "llvm/llvm_aggregates.h"
#include "llvm/llvm_builtins.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_functions.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "kernel/name/name.h"
#include "kernel/type/tagged.h"
#include "kernel/type/type.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* Emits the LLVM trap if. */
int
__LLVM_Emit_Trap_If__(__LLVM_Emitter__ *emitter, LLVMValueRef condition, const char *reason)
{
    /* Stores the trap name. */
    static const char trap_name[] = "llvm.trap";
    /* Stores the current. */
    LLVMBasicBlockRef current = LLVMGetInsertBlock(emitter->builder);
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the trap block. */
    LLVMBasicBlockRef trap_block;
    /* Stores the continue block. */
    LLVMBasicBlockRef continue_block;
    /* Stores the trap ID. */
    unsigned trap_id;
    /* Stores the trap function. */
    LLVMValueRef trap_function;
    /* Stores the trap type. */
    LLVMTypeRef trap_type;

    if (current == NULL || condition == NULL)
    {
        return __LLVM_Fail__("cannot construct L2 integer trap control flow");
    }
    function = LLVMGetBasicBlockParent(current);
    if (function == NULL)
    {
        return __LLVM_Fail__("cannot locate function for L2 integer trap");
    }

    trap_block = LLVMAppendBasicBlockInContext(emitter->context, function, reason);
    continue_block = LLVMAppendBasicBlockInContext(emitter->context, function, "integer.ok");
    LLVMBuildCondBr(emitter->builder, condition, trap_block, continue_block);

    LLVMPositionBuilderAtEnd(emitter->builder, trap_block);
    trap_id = LLVMLookupIntrinsicID(trap_name, sizeof(trap_name) - 1U);
    if (trap_id == 0U)
    {
        return __LLVM_Fail__("LLVM llvm.trap intrinsic is unavailable");
    }
    trap_function = LLVMGetIntrinsicDeclaration(emitter->module, trap_id, NULL, 0U);
    trap_type = LLVMFunctionType(LLVMVoidTypeInContext(emitter->context), NULL, 0U, 0);
    if (trap_function == NULL || trap_type == NULL)
    {
        return __LLVM_Fail__("cannot declare LLVM integer trap intrinsic");
    }
    LLVMBuildCall2(emitter->builder, trap_type, trap_function, NULL, 0U, "");
    LLVMBuildUnreachable(emitter->builder);

    LLVMPositionBuilderAtEnd(emitter->builder, continue_block);
    return 1;
}

static LLVMTypeRef __LLVM_Function_Type_From_Resolved__(__LLVM_Emitter__ *emitter,
                                                       const __Resolved_Type__ *resolved)
{
    LLVMTypeRef *parameters = NULL;
    LLVMTypeRef output;
    LLVMTypeRef function_type;
    size_t index;
    if (resolved == NULL || resolved->__Kind__ != __Resolved_Type_Function__)
        return NULL;
    if (resolved->__Parameter_Count__ != 0U)
    {
        parameters = (LLVMTypeRef *)calloc(resolved->__Parameter_Count__, sizeof(*parameters));
        if (parameters == NULL)
            return NULL;
    }
    for (index = 0U; index < resolved->__Parameter_Count__; ++index)
    {
        parameters[index] = __LLVM_Type__(emitter, resolved->__Parameters__[index]);
        if (parameters[index] == NULL)
        {
            free(parameters);
            return NULL;
        }
    }
    output = __LLVM_Type__(emitter, resolved->__Output__);
    if (output == NULL)
    {
        free(parameters);
        return NULL;
    }
    function_type = LLVMFunctionType(
        output, parameters, (unsigned)resolved->__Parameter_Count__, 0);
    free(parameters);
    return function_type;
}

/* Emits the LLVM checked arithmetic. */
static LLVMValueRef __LLVM_Emit_Checked_Arithmetic__(__LLVM_Emitter__ *emitter,
                                                     __Ast_Binary_Operation__ operation,
                                                     LLVMValueRef left,
                                                     LLVMValueRef right,
                                                     unsigned bits,
                                                     int is_signed)
{
    /* Stores the wide bits. */
    unsigned wide_bits = bits < 64U ? bits * 2U : 128U;
    /* Stores the narrow type. */
    LLVMTypeRef narrow_type = LLVMIntTypeInContext(emitter->context, bits);
    /* Stores the wide type. */
    LLVMTypeRef wide_type = LLVMIntTypeInContext(emitter->context, wide_bits);
    /* Stores the wide left. */
    LLVMValueRef wide_left;
    /* Stores the wide right. */
    LLVMValueRef wide_right;
    /* Stores the wide result. */
    LLVMValueRef wide_result;
    /* Stores the narrow result. */
    LLVMValueRef narrow_result;
    /* Stores the round trip. */
    LLVMValueRef round_trip;
    /* Stores the overflow. */
    LLVMValueRef overflow;

    wide_left = is_signed ? LLVMBuildSExt(emitter->builder, left, wide_type, "arith.left.sext")
                          : LLVMBuildZExt(emitter->builder, left, wide_type, "arith.left.zext");
    wide_right = is_signed ? LLVMBuildSExt(emitter->builder, right, wide_type, "arith.right.sext")
                           : LLVMBuildZExt(emitter->builder, right, wide_type, "arith.right.zext");

    switch (operation)
    {
        case __Binary_Add__:
            wide_result = LLVMBuildAdd(emitter->builder, wide_left, wide_right, "add.wide");
            break;
        case __Binary_Subtract__:
            wide_result = LLVMBuildSub(emitter->builder, wide_left, wide_right, "sub.wide");
            break;
        case __Binary_Multiply__:
            wide_result = LLVMBuildMul(emitter->builder, wide_left, wide_right, "mul.wide");
            break;
        default:
            __LLVM_Fail__("unsupported checked L2 arithmetic operation");
            return NULL;
    }

    narrow_result = LLVMBuildTrunc(emitter->builder, wide_result, narrow_type, "arith.narrow");
    round_trip =
        is_signed ? LLVMBuildSExt(emitter->builder, narrow_result, wide_type, "arith.check.sext")
                  : LLVMBuildZExt(emitter->builder, narrow_result, wide_type, "arith.check.zext");
    overflow =
        LLVMBuildICmp(emitter->builder, LLVMIntNE, wide_result, round_trip, "arith.overflow");
    if (!__LLVM_Emit_Trap_If__(emitter, overflow, "integer.overflow"))
    {
        return NULL;
    }
    return narrow_result;
}

/* Checks whether the LLVM is comparison. */
static int __LLVM_Is_Comparison__(__Ast_Binary_Operation__ operation)
{
    return operation == __Binary_Equal__ || operation == __Binary_Not_Equal__ ||
           operation == __Binary_Less_Than__ || operation == __Binary_Less_Or_Equal__ ||
           operation == __Binary_Greater_Or_Equal__ || operation == __Binary_Greater_Than__;
}

/* Emits the LLVM comparison. */
static LLVMValueRef __LLVM_Emit_Comparison__(__LLVM_Emitter__ *emitter,
                                             __Ast_Binary_Operation__ operation,
                                             LLVMValueRef left,
                                             LLVMValueRef right,
                                             int is_signed)
{
    /* Stores the predicate. */
    LLVMIntPredicate predicate;
    switch (operation)
    {
        case __Binary_Equal__:
            predicate = LLVMIntEQ;
            break;
        case __Binary_Not_Equal__:
            predicate = LLVMIntNE;
            break;
        case __Binary_Less_Than__:
            predicate = is_signed ? LLVMIntSLT : LLVMIntULT;
            break;
        case __Binary_Less_Or_Equal__:
            predicate = is_signed ? LLVMIntSLE : LLVMIntULE;
            break;
        case __Binary_Greater_Or_Equal__:
            predicate = is_signed ? LLVMIntSGE : LLVMIntUGE;
            break;
        case __Binary_Greater_Than__:
            predicate = is_signed ? LLVMIntSGT : LLVMIntUGT;
            break;
        default:
            __LLVM_Fail__("unsupported L2 integer comparison operation");
            return NULL;
    }
    return LLVMBuildICmp(emitter->builder, predicate, left, right, "integer.compare");
}

/* Emits the LLVM shift. */
static LLVMValueRef __LLVM_Emit_Shift__(__LLVM_Emitter__ *emitter,
                                        __Ast_Binary_Operation__ operation,
                                        LLVMValueRef left,
                                        LLVMValueRef right,
                                        unsigned bits,
                                        int is_signed)
{
    /* Stores the type. */
    LLVMTypeRef type = LLVMIntTypeInContext(emitter->context, bits);
    /* Stores the width. */
    LLVMValueRef width = LLVMConstInt(type, bits, 0);
    /* Stores the invalid. */
    LLVMValueRef invalid =
        LLVMBuildICmp(emitter->builder, LLVMIntUGE, right, width, "integer.shift.invalid");

    if (!__LLVM_Emit_Trap_If__(emitter, invalid, "integer.shift.out.of.range"))
    {
        return NULL;
    }
    if (operation == __Binary_Shift_Left_Logical__)
    {
        return LLVMBuildShl(emitter->builder, left, right, "shift.left");
    }
    if (operation == __Binary_Shift_Right_Logical__)
    {
        return is_signed ? LLVMBuildAShr(emitter->builder, left, right, "shift.right.arithmetic")
                         : LLVMBuildLShr(emitter->builder, left, right, "shift.right.logical");
    }
    __LLVM_Fail__("unsupported L2 integer shift operation");
    return NULL;
}

/* Emits the LLVM division or remainder. */
static LLVMValueRef __LLVM_Emit_Division_Or_Remainder__(__LLVM_Emitter__ *emitter,
                                                        __Ast_Binary_Operation__ operation,
                                                        LLVMValueRef left,
                                                        LLVMValueRef right,
                                                        unsigned bits,
                                                        int is_signed)
{
    /* Stores the type. */
    LLVMTypeRef type = LLVMIntTypeInContext(emitter->context, bits);
    /* Stores the zero. */
    LLVMValueRef zero = LLVMConstInt(type, 0U, 0);
    /* Tracks whether the value is zero. */
    LLVMValueRef is_zero =
        LLVMBuildICmp(emitter->builder, LLVMIntEQ, right, zero, "integer.divisor.zero");

    if (!__LLVM_Emit_Trap_If__(emitter, is_zero, "integer.divide.by.zero"))
    {
        return NULL;
    }

    if (is_signed)
    {
        /* Stores the min pattern. */
        unsigned long long min_pattern = 1ULL << (bits - 1U);
        /* Stores the minimum. */
        LLVMValueRef minimum = LLVMConstInt(type, min_pattern, 0);
        /* Stores the minus one. */
        LLVMValueRef minus_one = LLVMConstInt(type, UINT64_MAX, 0);
        /* Tracks whether the value is minimum. */
        LLVMValueRef is_minimum =
            LLVMBuildICmp(emitter->builder, LLVMIntEQ, left, minimum, "integer.is.minimum");
        /* Tracks whether the value is minus one. */
        LLVMValueRef is_minus_one =
            LLVMBuildICmp(emitter->builder, LLVMIntEQ, right, minus_one, "integer.is.minus.one");
        /* Stores the invalid pair. */
        LLVMValueRef invalid_pair =
            LLVMBuildAnd(emitter->builder, is_minimum, is_minus_one, "integer.min.minus.one");

        if (!__LLVM_Emit_Trap_If__(emitter, invalid_pair, "integer.signed.divide.overflow"))
        {
            return NULL;
        }
        return operation == __Binary_Divide__
                   ? LLVMBuildSDiv(emitter->builder, left, right, "sdiv")
                   : LLVMBuildSRem(emitter->builder, left, right, "srem");
    }

    return operation == __Binary_Divide__ ? LLVMBuildUDiv(emitter->builder, left, right, "udiv")
                                          : LLVMBuildURem(emitter->builder, left, right, "urem");
}

/* Emits the LLVM atom. */
__LLVM_Value__
__LLVM_Emit_Atom__(__LLVM_Emitter__ *emitter, __Ast_Atom__ *atom, __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the bits. */
    unsigned bits;
    /* Tracks whether the value is signed. */
    int is_signed;
    if (atom == NULL)
    {
        __LLVM_Fail__("missing L2.3 atom");
        return result;
    }
    if (atom->__Kind__ == __Ast_Atom_Lvalue__)
    {
        result = __LLVM_Emit_Lvalue__(emitter, atom->__As__.__Lvalue__);
        if (expected == NULL)
        {
            return result;
        }
        {
            /* Stores the source resolved. */
            __Resolved_Type__ source_resolved;
            /* Stores the target resolved. */
            __Resolved_Type__ target_resolved;
            if (__Type_Resolve__(emitter->semantic, result.type, &source_resolved) &&
                __Type_Resolve__(emitter->semantic, expected, &target_resolved) &&
                source_resolved.__Kind__ == __Resolved_Type_Boolean__ &&
                target_resolved.__Kind__ == __Resolved_Type_Boolean__)
            {
                result.type = expected;
                return result;
            }
        }
        return __LLVM_Coerce__(emitter, result, expected);
    }
    if (atom->__Kind__ != __Ast_Atom_Literal__ || atom->__As__.__Literal__ == NULL)
    {
        __LLVM_Fail__("L2.3 supports only scalar literals/lvalues");
        return result;
    }
    if (atom->__As__.__Literal__->__Kind__ == __Ast_Literal_Boolean__)
    {
        result.value = LLVMConstInt(LLVMIntTypeInContext(emitter->context, 1U),
                                    atom->__As__.__Literal__->__As__.__Boolean__ ? 1U : 0U,
                                    0);
        result.type = expected != NULL ? expected : &__LLVM_Boolean_Type__;
        return result;
    }
    if (atom->__As__.__Literal__->__Kind__ == __Ast_Literal_String__)
    {
        return __LLVM_Emit_String_Literal__(
            emitter, atom->__As__.__Literal__->__As__.__String__, expected);
    }
    if (atom->__As__.__Literal__->__Kind__ != __Ast_Literal_Integer__ || expected == NULL ||
        !__LLVM_Resolve_Integer__(emitter, expected, &bits, &is_signed))
    {
        __LLVM_Fail__("L2.3 integer literal requires a typed integer context");
        return result;
    }
    result.value =
        LLVMConstInt(LLVMIntTypeInContext(emitter->context, bits),
                     (unsigned long long)atom->__As__.__Literal__->__As__.__Integer__.__Value__,
                     is_signed ? 1 : 0);
    result.type = expected;
    return result;
}

/* Emits the LLVM expression. */
__LLVM_Value__ __LLVM_Emit_Expression__(__LLVM_Emitter__ *emitter,
                                               __Ast_Expression__ *expression,
                                               __Ast_Type__ *expected)
{
    /* Stores the left. */
    __LLVM_Value__ left, right, result = __LLVM_Invalid_Value__();
    /* References the operation type. */
    __Ast_Type__ *operation_type;
    if (expression == NULL)
    {
        __LLVM_Fail__("missing L2 expression");
        return result;
    }
    /* Preserve the semantic contextual type when lowering has no stronger type. */
    if (expected == NULL && expression->__Contextual_Type__ != NULL)
    {
        expected = expression->__Contextual_Type__;
    }
    switch (expression->__Kind__)
    {
        case __Ast_Expression_Atom__:
            return __LLVM_Emit_Atom__(emitter, &expression->__As__.__Atom__, expected);

        case __Ast_Expression_Conversion__:
            /* An explicit conversion supplies the type context for an untyped literal.
             * Do not first lower that literal without context: doing so records a
             * permanent LLVM failure before the typed retry can succeed. */
            if (expression->__As__.__Conversion__.__Operand__->__Kind__ ==
                    __Ast_Expression_Atom__ &&
                expression->__As__.__Conversion__.__Operand__->__As__.__Atom__.__Kind__ ==
                    __Ast_Atom_Literal__)
            {
                left = __LLVM_Emit_Atom__(
                    emitter,
                    &expression->__As__.__Conversion__.__Operand__->__As__.__Atom__,
                    expression->__As__.__Conversion__.__Target_Type__);
            }
            else
            {
                left = __LLVM_Emit_Expression__(
                    emitter, expression->__As__.__Conversion__.__Operand__, NULL);
            }
            if (left.value == NULL)
            {
                return result;
            }
            return __LLVM_Coerce__(
                emitter, left, expression->__As__.__Conversion__.__Target_Type__);

        case __Ast_Expression_Binary__:
        {
            /* Stores the operation. */
            __Ast_Binary_Operation__ operation = expression->__As__.__Binary__.__Operation__;
            /* Stores the bits. */
            unsigned bits;
            /* Tracks whether the value is signed. */
            int is_signed;
            /* References the left expression type. */
            __Ast_Type__ *left_expression_type =
                __LLVM_Expression_Type__(emitter, expression->__As__.__Binary__.__Left__);
            /* References the right expression type. */
            __Ast_Type__ *right_expression_type =
                __LLVM_Expression_Type__(emitter, expression->__As__.__Binary__.__Right__);
            /* Stores the left resolved. */
            __Resolved_Type__ left_resolved;
            /* Stores the right resolved. */
            __Resolved_Type__ right_resolved;

            if (operation == __Binary_Logical_And__ || operation == __Binary_Logical_Or__)
            {
                /* Stores the boolean type. */
                LLVMTypeRef boolean_type = LLVMIntTypeInContext(emitter->context, 1U);
                /* Stores the storage. */
                LLVMValueRef storage;
                /* Stores the current. */
                LLVMBasicBlockRef current;
                /* Stores the function. */
                LLVMValueRef function;
                /* Stores the right block. */
                LLVMBasicBlockRef right_block;
                /* Stores the merge block. */
                LLVMBasicBlockRef merge_block;
                /* Stores the resolved. */
                __Resolved_Type__ resolved;

                left = __LLVM_Emit_Expression__(
                    emitter, expression->__As__.__Binary__.__Left__, &__LLVM_Boolean_Type__);
                if (left.value == NULL || left.type == NULL ||
                    !__Type_Resolve__(emitter->semantic, left.type, &resolved) ||
                    resolved.__Kind__ != __Resolved_Type_Boolean__)
                {
                    __LLVM_Fail__("logical binary operation requires canonical boolean operands");
                    return result;
                }
                current = LLVMGetInsertBlock(emitter->builder);
                function = current != NULL ? LLVMGetBasicBlockParent(current) : NULL;
                if (function == NULL)
                {
                    __LLVM_Fail__("logical binary operation lost current LLVM function");
                    return result;
                }
                storage = __LLVM_Allocate_Stack__(emitter, boolean_type, "logical.result");
                LLVMBuildStore(emitter->builder, left.value, storage);
                right_block =
                    LLVMAppendBasicBlockInContext(emitter->context, function, "logical.right");
                merge_block =
                    LLVMAppendBasicBlockInContext(emitter->context, function, "logical.end");
                if (operation == __Binary_Logical_And__)
                    LLVMBuildCondBr(emitter->builder, left.value, right_block, merge_block);
                else
                    LLVMBuildCondBr(emitter->builder, left.value, merge_block, right_block);

                LLVMPositionBuilderAtEnd(emitter->builder, right_block);
                right = __LLVM_Emit_Expression__(
                    emitter, expression->__As__.__Binary__.__Right__, &__LLVM_Boolean_Type__);
                if (right.value == NULL || right.type == NULL ||
                    !__Type_Resolve__(emitter->semantic, right.type, &resolved) ||
                    resolved.__Kind__ != __Resolved_Type_Boolean__)
                {
                    __LLVM_Fail__("logical binary operation requires canonical boolean operands");
                    return result;
                }
                LLVMBuildStore(emitter->builder, right.value, storage);
                if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(emitter->builder)) == NULL)
                    LLVMBuildBr(emitter->builder, merge_block);

                LLVMPositionBuilderAtEnd(emitter->builder, merge_block);
                result.value =
                    LLVMBuildLoad2(emitter->builder, boolean_type, storage, "logical.value");
                result.type = &__LLVM_Boolean_Type__;
                return result;
            }

            if ((operation == __Binary_Equal__ || operation == __Binary_Not_Equal__) &&
                left_expression_type != NULL && right_expression_type != NULL &&
                __Type_Resolve__(emitter->semantic, left_expression_type, &left_resolved) &&
                __Type_Resolve__(emitter->semantic, right_expression_type, &right_resolved) &&
                left_resolved.__Kind__ == __Resolved_Type_Boolean__ &&
                right_resolved.__Kind__ == __Resolved_Type_Boolean__)
            {
                left = __LLVM_Emit_Expression__(
                    emitter, expression->__As__.__Binary__.__Left__, &__LLVM_Boolean_Type__);
                right = __LLVM_Emit_Expression__(
                    emitter, expression->__As__.__Binary__.__Right__, &__LLVM_Boolean_Type__);
                if (left.value == NULL || right.value == NULL)
                    return result;
                result.value = LLVMBuildICmp(emitter->builder,
                                             operation == __Binary_Equal__ ? LLVMIntEQ : LLVMIntNE,
                                             left.value,
                                             right.value,
                                             "boolean.compare");
                result.type = &__LLVM_Boolean_Type__;
                return result;
            }

            if (operation == __Binary_Equal__ || operation == __Binary_Not_Equal__)
            {
                /* References the left enum. */
                __Semantic_Type_Entry__ *left_enum = NULL;
                /* References the right enum. */
                __Semantic_Type_Entry__ *right_enum = NULL;
                /* References the enum value type. */
                __Ast_Type__ *enum_value_type = NULL;
                if (left_expression_type != NULL &&
                    __Type_Resolve__(emitter->semantic, left_expression_type, &left_resolved) &&
                    left_resolved.__Kind__ == __Resolved_Type_Enum__)
                {
                    left_enum = left_resolved.__Named__;
                    enum_value_type = left_expression_type;
                }
                else if (expression->__As__.__Binary__.__Left__->__Kind__ ==
                         __Ast_Expression_Call__)
                {
                    /* Tracks the ignored index. */
                    size_t ignored_index = 0U;
                    /* References the ignored constructor. */
                    __Ast_Enum_Constructor__ *ignored_constructor = NULL;
                    (void)__Name_Resolve_Enum_Constructor_Lvalue__(
                        emitter->semantic,
                        expression->__As__.__Binary__.__Left__->__As__.__Call__.__Function__,
                        &left_enum,
                        &ignored_index,
                        &ignored_constructor);
                }
                if (right_expression_type != NULL &&
                    __Type_Resolve__(emitter->semantic, right_expression_type, &right_resolved) &&
                    right_resolved.__Kind__ == __Resolved_Type_Enum__)
                {
                    right_enum = right_resolved.__Named__;
                    if (enum_value_type == NULL)
                        enum_value_type = right_expression_type;
                }
                else if (expression->__As__.__Binary__.__Right__->__Kind__ ==
                         __Ast_Expression_Call__)
                {
                    /* Tracks the ignored index. */
                    size_t ignored_index = 0U;
                    /* References the ignored constructor. */
                    __Ast_Enum_Constructor__ *ignored_constructor = NULL;
                    (void)__Name_Resolve_Enum_Constructor_Lvalue__(
                        emitter->semantic,
                        expression->__As__.__Binary__.__Right__->__As__.__Call__.__Function__,
                        &right_enum,
                        &ignored_index,
                        &ignored_constructor);
                }
                if (left_enum != NULL && left_enum == right_enum && enum_value_type != NULL &&
                    __LLVM_Enum_Is_Payload_Free__(left_enum))
                {
                    left = __LLVM_Emit_Expression__(
                        emitter, expression->__As__.__Binary__.__Left__, enum_value_type);
                    right = __LLVM_Emit_Expression__(
                        emitter, expression->__As__.__Binary__.__Right__, enum_value_type);
                    if (left.value == NULL || right.value == NULL)
                        return result;
                    result.value = LLVMBuildICmp(
                        emitter->builder,
                        operation == __Binary_Equal__ ? LLVMIntEQ : LLVMIntNE,
                        LLVMBuildExtractValue(emitter->builder, left.value, 0U, "enum.left.tag"),
                        LLVMBuildExtractValue(emitter->builder, right.value, 0U, "enum.right.tag"),
                        "enum.tag.compare");
                    result.type = &__LLVM_Boolean_Type__;
                    return result;
                }
            }

            if (__LLVM_Is_Comparison__(operation) && left_expression_type != NULL &&
                right_expression_type != NULL &&
                __Type_Resolve__(emitter->semantic, left_expression_type, &left_resolved) &&
                __Type_Resolve__(emitter->semantic, right_expression_type, &right_resolved) &&
                left_resolved.__Kind__ == __Resolved_Type_String__ &&
                right_resolved.__Kind__ == __Resolved_Type_String__)
            {
                left = __LLVM_Emit_Expression__(
                    emitter, expression->__As__.__Binary__.__Left__, left_expression_type);
                right = __LLVM_Emit_Expression__(
                    emitter, expression->__As__.__Binary__.__Right__, right_expression_type);
                if (left.value == NULL || right.value == NULL)
                    return result;
                result.value =
                    __LLVM_Compare_Text_Values__(emitter, left.value, right.value, operation);
                result.type = &__LLVM_Boolean_Type__;
                return result.value != NULL ? result : __LLVM_Invalid_Value__();
            }

            if (operation != __Binary_Add__ && operation != __Binary_Subtract__ &&
                operation != __Binary_Multiply__ && operation != __Binary_Divide__ &&
                operation != __Binary_Modulo__ && operation != __Binary_Or__ &&
                operation != __Binary_Xor__ && operation != __Binary_And__ &&
                !__LLVM_Is_Comparison__(operation) && operation != __Binary_Shift_Left_Logical__ &&
                operation != __Binary_Shift_Right_Logical__)
            {
                __LLVM_Fail__("operation is outside the L2 integer proof profile");
                return result;
            }

            operation_type = __LLVM_Concrete_Integer_Type__(
                emitter,
                __LLVM_Expression_Integer_Type__(emitter, expression->__As__.__Binary__.__Left__));
            if (operation_type == NULL)
            {
                operation_type = __LLVM_Concrete_Integer_Type__(
                    emitter,
                    __LLVM_Expression_Integer_Type__(emitter,
                                                     expression->__As__.__Binary__.__Right__));
            }
            if (operation_type == NULL)
            {
                operation_type = __LLVM_Concrete_Integer_Type__(emitter, expected);
            }
            if (operation_type == NULL ||
                !__LLVM_Resolve_Integer__(emitter, operation_type, &bits, &is_signed))
            {
                __LLVM_Fail__("L2 integer binary operation lacks a resolved integer Type");
                return result;
            }

            left = __LLVM_Emit_Expression__(
                emitter, expression->__As__.__Binary__.__Left__, operation_type);
            if (left.value == NULL)
            {
                return result;
            }
            left = __LLVM_Coerce__(emitter, left, operation_type);
            if (left.value == NULL)
            {
                return result;
            }
            right = __LLVM_Emit_Expression__(
                emitter, expression->__As__.__Binary__.__Right__, operation_type);
            if (right.value == NULL)
            {
                return result;
            }
            right = __LLVM_Coerce__(emitter, right, operation_type);
            if (right.value == NULL)
            {
                return result;
            }

            if (operation == __Binary_Divide__ || operation == __Binary_Modulo__)
            {
                result.value = __LLVM_Emit_Division_Or_Remainder__(
                    emitter, operation, left.value, right.value, bits, is_signed);
            }
            else if (operation == __Binary_Add__ || operation == __Binary_Subtract__ ||
                     operation == __Binary_Multiply__)
            {
                result.value = __LLVM_Emit_Checked_Arithmetic__(
                    emitter, operation, left.value, right.value, bits, is_signed);
            }
            else if (operation == __Binary_Or__)
            {
                result.value = LLVMBuildOr(emitter->builder, left.value, right.value, "bitwise.or");
            }
            else if (operation == __Binary_Xor__)
            {
                result.value =
                    LLVMBuildXor(emitter->builder, left.value, right.value, "bitwise.xor");
            }
            else if (operation == __Binary_And__)
            {
                result.value =
                    LLVMBuildAnd(emitter->builder, left.value, right.value, "bitwise.and");
            }
            else if (__LLVM_Is_Comparison__(operation))
            {
                result.value = __LLVM_Emit_Comparison__(
                    emitter, operation, left.value, right.value, is_signed);
                result.type = &__LLVM_Boolean_Type__;
                return result.value != NULL ? result : __LLVM_Invalid_Value__();
            }
            else
            {
                result.value = __LLVM_Emit_Shift__(
                    emitter, operation, left.value, right.value, bits, is_signed);
            }
            if (result.value == NULL)
            {
                return __LLVM_Invalid_Value__();
            }
            result.type = operation_type;
            return result;
        }

        case __Ast_Expression_Call__:
        {
            /* References the tagged target. */
            __Ast_Type__ *tagged_target =
                expected != NULL ? expected : expression->__Contextual_Type__;
            /* Stores the builtin. */
            __Name_Builtin_Function__ builtin = __LLVM_Builtin_Call_Identity__(emitter, expression);
            if (builtin != __Name_Builtin_None__)
                return __LLVM_Emit_Builtin_Call__(
                    emitter, expression, expected, tagged_target, builtin);
            /* References the enum type. */
            __Semantic_Type_Entry__ *enum_type = NULL;
            /* References the enum constructor. */
            __Ast_Enum_Constructor__ *enum_constructor = NULL;
            /* Tracks the enum constructor index. */
            size_t enum_constructor_index = 0U;
            /* Stores the builtin constructor. */
            __Type_Tagged_Constructor__ builtin_constructor;

            if (__Name_Resolve_Enum_Constructor_Lvalue__(emitter->semantic,
                                                         expression->__As__.__Call__.__Function__,
                                                         &enum_type,
                                                         &enum_constructor_index,
                                                         &enum_constructor))
            {
                /* Stores the target resolved. */
                __Resolved_Type__ target_resolved;
                if (tagged_target == NULL ||
                    !__Type_Resolve__(emitter->semantic, tagged_target, &target_resolved) ||
                    target_resolved.__Kind__ != __Resolved_Type_Enum__ ||
                    target_resolved.__Named__ == NULL || enum_type == NULL)
                {
                    __LLVM_Fail__(
                        "L2.5 enum construction disagrees with canonical semantic identity");
                    return result;
                }
                {
                    __Ast_Type_Declaration__ *target_template =
                        target_resolved.__Named__->__Template_Declaration__ != NULL
                            ? target_resolved.__Named__->__Template_Declaration__
                            : target_resolved.__Named__->__Declaration__;
                    __Ast_Type_Declaration__ *source_template =
                        enum_type->__Template_Declaration__ != NULL
                            ? enum_type->__Template_Declaration__
                            : enum_type->__Declaration__;
                    if (target_template != source_template ||
                        !__Name_Find_Enum_Constructor__(
                            target_resolved.__Named__,
                            expression->__As__.__Call__.__Function__->__As__.__Field__.__Field__,
                            &enum_constructor_index, &enum_constructor) ||
                        enum_constructor == NULL ||
                        enum_constructor->__Payload_Count__ !=
                            expression->__As__.__Call__.__Argument_Count__)
                    {
                        __LLVM_Fail__(
                            "L2.5 enum construction disagrees with concrete generic identity");
                        return result;
                    }
                    enum_type = target_resolved.__Named__;
                }
                return __LLVM_Emit_Tagged_Construct__(
                    emitter,
                    tagged_target,
                    enum_constructor_index,
                    expression->__As__.__Call__.__Arguments__,
                    expression->__As__.__Call__.__Argument_Count__);
            }

            memset(&builtin_constructor, 0, sizeof(builtin_constructor));
            if (__LLVM_Resolve_Builtin_Tagged_Construct__(
                    tagged_target, expression, &builtin_constructor))
            {
                if (builtin_constructor.__Payload_Count__ !=
                    expression->__As__.__Call__.__Argument_Count__)
                {
                    __LLVM_Fail__(
                        "L2.5 builtin tagged construction payload count disagrees with semantics");
                    return result;
                }
                return __LLVM_Emit_Tagged_Construct__(
                    emitter,
                    tagged_target,
                    builtin_constructor.__Tag__,
                    expression->__As__.__Call__.__Arguments__,
                    expression->__As__.__Call__.__Argument_Count__);
            }

            /* References the callee. */
            __Semantic_Function_Entry__ *callee =
                __LLVM_Resolve_Ordinary_Callee__(emitter, expression);
            /* References the LLVM callee. */
            __LLVM_Function__ *llvm_callee;
            /* Stores the call arguments. */
            LLVMValueRef *arguments = NULL;
            /* Tracks the index. */
            size_t index;
            if (callee == NULL)
            {
                __Ast_Type__ *callable_type =
                    __LLVM_Lvalue_Type__(emitter, expression->__As__.__Call__.__Function__);
                __Resolved_Type__ callable_resolved;
                __LLVM_Value__ callable;
                LLVMTypeRef callable_function_type;
                LLVMValueRef *indirect_arguments = NULL;
                size_t indirect_index;
                if (callable_type == NULL ||
                    !__Type_Resolve__(emitter->semantic, callable_type, &callable_resolved) ||
                    callable_resolved.__Kind__ != __Resolved_Type_Function__ ||
                    callable_resolved.__Parameter_Count__ !=
                        expression->__As__.__Call__.__Argument_Count__)
                {
                    __LLVM_Fail__("L2.3 call is not a canonical SultanC function value");
                    return result;
                }
                callable = __LLVM_Emit_Lvalue__(emitter, expression->__As__.__Call__.__Function__);
                callable_function_type =
                    __LLVM_Function_Type_From_Resolved__(emitter, &callable_resolved);
                if (callable.value == NULL || callable_function_type == NULL)
                    return result;
                if (callable_resolved.__Parameter_Count__ != 0U)
                {
                    indirect_arguments = (LLVMValueRef *)calloc(
                        callable_resolved.__Parameter_Count__, sizeof(*indirect_arguments));
                    if (indirect_arguments == NULL)
                    {
                        __LLVM_Fail__("out of memory while lowering indirect call arguments");
                        return result;
                    }
                }
                for (indirect_index = 0U;
                     indirect_index < callable_resolved.__Parameter_Count__; ++indirect_index)
                {
                    __LLVM_Value__ argument = __LLVM_Emit_Expression__(
                        emitter, expression->__As__.__Call__.__Arguments__[indirect_index],
                        callable_resolved.__Parameters__[indirect_index]);
                    if (argument.value == NULL)
                    {
                        free(indirect_arguments);
                        return result;
                    }
                    argument = __LLVM_Coerce__(
                        emitter, argument, callable_resolved.__Parameters__[indirect_index]);
                    if (argument.value == NULL)
                    {
                        free(indirect_arguments);
                        return result;
                    }
                    indirect_arguments[indirect_index] = argument.value;
                }
                {
                    __Resolved_Type__ output_resolved;
                    const char *call_name = "call.indirect";
                    if (!__Type_Resolve__(
                            emitter->semantic, callable_resolved.__Output__, &output_resolved))
                    {
                        free(indirect_arguments);
                        return result;
                    }
                    if (output_resolved.__Kind__ == __Resolved_Type_Void__)
                        call_name = "";
                    result.value = LLVMBuildCall2(
                        emitter->builder, callable_function_type, callable.value,
                        indirect_arguments, (unsigned)callable_resolved.__Parameter_Count__,
                        call_name);
                }
                free(indirect_arguments);
                result.type = callable_resolved.__Output__;
                if (expected != NULL)
                    return __LLVM_Coerce__(emitter, result, expected);
                return result;
            }
            llvm_callee = __LLVM_Find_Function_By_Semantic__(emitter, callee);
            if (llvm_callee == NULL)
            {
                __LLVM_Fail__("L2.3 ordinary callee has no LLVM declaration");
                return result;
            }
            if (expression->__As__.__Call__.__Argument_Count__ !=
                callee->__Function__->__Parameter_Count__)
            {
                __LLVM_Fail__("L2.3 ordinary call argument count disagrees with semantics");
                return result;
            }
            if (callee->__Function__->__Parameter_Count__ != 0U)
            {
                arguments = (LLVMValueRef *)calloc(callee->__Function__->__Parameter_Count__,
                                                   sizeof(*arguments));
                if (arguments == NULL)
                {
                    __LLVM_Fail__("out of memory while lowering L2.3 call arguments");
                    return result;
                }
            }
            for (index = 0U; index < callee->__Function__->__Parameter_Count__; ++index)
            {
                /* Stores the argument. */
                __LLVM_Value__ argument = __LLVM_Emit_Expression__(
                    emitter,
                    expression->__As__.__Call__.__Arguments__[index],
                    callee->__Function__->__Parameters__[index].__Slot__.__Type__);
                if (argument.value == NULL)
                {
                    free(arguments);
                    return result;
                }
                {
                    /* Stores the resolved. */
                    __Resolved_Type__ resolved;
                    if (!__Type_Resolve__(
                            emitter->semantic,
                            callee->__Function__->__Parameters__[index].__Slot__.__Type__,
                            &resolved))
                    {
                        free(arguments);
                        __LLVM_Fail__("L2.3 could not resolve call parameter Type");
                        return result;
                    }
                    if (resolved.__Kind__ != __Resolved_Type_Boolean__)
                    {
                        argument = __LLVM_Coerce__(
                            emitter,
                            argument,
                            callee->__Function__->__Parameters__[index].__Slot__.__Type__);
                    }
                }
                if (argument.value == NULL)
                {
                    free(arguments);
                    return result;
                }
                arguments[index] = argument.value;
            }
            {
                /* Stores the call output. */
                __Resolved_Type__ call_output;
                /* References the call name. */
                const char *call_name = "call";
                if (!__Type_Resolve__(
                        emitter->semantic, callee->__Function__->__Output__.__Type__, &call_output))
                {
                    free(arguments);
                    __LLVM_Fail__("Direct LLVM could not resolve ordinary call result Type");
                    return result;
                }
                if (call_output.__Kind__ == __Resolved_Type_Void__)
                    call_name = "";
                result.value = LLVMBuildCall2(emitter->builder,
                                              llvm_callee->type,
                                              llvm_callee->value,
                                              arguments,
                                              (unsigned)callee->__Function__->__Parameter_Count__,
                                              call_name);
            }
            free(arguments);
            result.type = callee->__Function__->__Output__.__Type__;
            if (expected != NULL)
            {
                /* Stores the output resolved. */
                __Resolved_Type__ output_resolved;
                /* Stores the expected resolved. */
                __Resolved_Type__ expected_resolved;
                if (__Type_Resolve__(emitter->semantic, result.type, &output_resolved) &&
                    __Type_Resolve__(emitter->semantic, expected, &expected_resolved) &&
                    output_resolved.__Kind__ == __Resolved_Type_Boolean__ &&
                    expected_resolved.__Kind__ == __Resolved_Type_Boolean__)
                {
                    result.type = expected;
                    return result;
                }
                return __LLVM_Coerce__(emitter, result, expected);
            }
            return result;
        }

        case __Ast_Expression_Unary__:
        {
            /* Stores the operation. */
            __Ast_Unary_Operation__ operation = expression->__As__.__Unary__.__Operation__;
            if (operation == __Unary_Address__ || operation == __Unary_Address_Mutable__)
            {
                /* References the operand. */
                __Ast_Expression__ *operand = expression->__As__.__Unary__.__Operand__;
                /* References the reference type. */
                __Ast_Type__ *reference_type =
                    expected != NULL ? expected : expression->__Contextual_Type__;
                /* Stores the resolved. */
                __Resolved_Type__ resolved;
                /* Stores the operand place. */
                __LLVM_Place__ operand_place;
                if (operand == NULL || operand->__Kind__ != __Ast_Expression_Atom__ ||
                    operand->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
                    reference_type == NULL ||
                    !__Type_Resolve__(emitter->semantic, reference_type, &resolved) ||
                    (resolved.__Kind__ != __Resolved_Type_Reference__ &&
                     resolved.__Kind__ != __Resolved_Type_Pointer__))
                {
                    __LLVM_Fail__(
                        "L2.6 address creation requires canonical semantic reference/pointer Type");
                    return result;
                }
                operand_place =
                    __LLVM_Emit_Place__(emitter, operand->__As__.__Atom__.__As__.__Lvalue__);
                if (operand_place.address == NULL || operand_place.type == NULL ||
                    resolved.__Inner__ == NULL ||
                    !__Type_Compatible__(emitter->semantic, operand_place.type, resolved.__Inner__))
                {
                    __LLVM_Fail__(
                        "L2.6 address operand disagrees with canonical reference inner Type");
                    return result;
                }
                result.value = LLVMBuildPointerCast(
                    emitter->builder,
                    operand_place.address,
                    __LLVM_Type__(emitter, reference_type),
                    operation == __Unary_Address_Mutable__ ? "reference.mutable" : "reference");
                result.type = reference_type;
                return result;
            }
            if (operation == __Unary_Dereference__)
            {
                /* Stores the resolved. */
                __Resolved_Type__ resolved;
                /* Stores the inner type. */
                LLVMTypeRef inner_type;
                left = __LLVM_Emit_Expression__(
                    emitter, expression->__As__.__Unary__.__Operand__, NULL);
                if (left.value == NULL || left.type == NULL ||
                    !__Type_Resolve__(emitter->semantic, left.type, &resolved) ||
                    (resolved.__Kind__ != __Resolved_Type_Reference__ &&
                     resolved.__Kind__ != __Resolved_Type_Box__) ||
                    resolved.__Inner__ == NULL)
                {
                    __LLVM_Fail__("L2.6 dereference requires canonical reference/box value");
                    return result;
                }
                inner_type = __LLVM_Type__(emitter, resolved.__Inner__);
                if (inner_type == NULL)
                    return result;
                result.value =
                    LLVMBuildLoad2(emitter->builder, inner_type, left.value, "dereference.load");
                result.type = resolved.__Inner__;
                return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
            }
            if (operation == __Unary_Not__)
            {
                /* Stores the resolved. */
                __Resolved_Type__ resolved;
                left = __LLVM_Emit_Expression__(
                    emitter, expression->__As__.__Unary__.__Operand__, NULL);
                if (left.value == NULL || left.type == NULL ||
                    !__Type_Resolve__(emitter->semantic, left.type, &resolved) ||
                    resolved.__Kind__ != __Resolved_Type_Boolean__)
                {
                    __LLVM_Fail__("L2.3 logical not requires boolean operand");
                    return result;
                }
                result.value =
                    LLVMBuildXor(emitter->builder,
                                 left.value,
                                 LLVMConstInt(LLVMIntTypeInContext(emitter->context, 1U), 1U, 0),
                                 "logical.not");
                result.type = &__LLVM_Boolean_Type__;
                return result;
            }
            operation_type = __LLVM_Concrete_Integer_Type__(
                emitter,
                __LLVM_Expression_Integer_Type__(emitter,
                                                 expression->__As__.__Unary__.__Operand__));
            if (operation_type == NULL)
            {
                operation_type = __LLVM_Concrete_Integer_Type__(emitter, expected);
            }
            if (operation_type == NULL)
            {
                __LLVM_Fail__("L2.3 unary integer operation lacks Type context");
                return result;
            }
            {
                /* Stores the bits. */
                unsigned bits;
                /* Tracks whether the value is signed. */
                int is_signed;
                /* Stores the zero. */
                LLVMValueRef zero;
                if (!__LLVM_Resolve_Integer__(emitter, operation_type, &bits, &is_signed))
                {
                    __LLVM_Fail__("L2.3 unary operation requires integer Type");
                    return result;
                }
                left = __LLVM_Emit_Expression__(
                    emitter, expression->__As__.__Unary__.__Operand__, operation_type);
                if (left.value == NULL)
                {
                    return result;
                }
                if (operation == __Unary_Negate__)
                {
                    zero = LLVMConstInt(LLVMIntTypeInContext(emitter->context, bits), 0U, 0);
                    result.value = __LLVM_Emit_Checked_Arithmetic__(
                        emitter, __Binary_Subtract__, zero, left.value, bits, is_signed);
                }
                else if (operation == __Unary_Bitwise_Not__)
                {
                    /* Stores the ones. */
                    LLVMValueRef ones =
                        LLVMConstInt(LLVMIntTypeInContext(emitter->context, bits), UINT64_MAX, 0);
                    result.value = LLVMBuildXor(emitter->builder, left.value, ones, "bitwise.not");
                }
                else
                {
                    __LLVM_Fail__("unary reference operation is outside L2.3 scalar profile");
                    return result;
                }
                result.type = operation_type;
                return result.value != NULL ? result : __LLVM_Invalid_Value__();
            }
        }
    }
    __LLVM_Fail__("unknown L2 expression");
    return result;
}
