/* Owns Stage0 LLVM statement and control-flow lowering. */

#include "llvm/llvm_statements.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_patterns.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "llvm/runtime/llvm_runtime_memory.h"
#include "kernel/layout/layout.h"
#include "kernel/memory/memory.h"
#include "kernel/type/type.h"

static int __LLVM_Emit_Statement__(__LLVM_Emitter__ *emitter,
                                   __Ast_Statement__ *statement,
                                   __Ast_Type__ *return_type,
                                   int *path_terminated);

/* Emits the LLVM statement list. */
int __LLVM_Emit_Statement_List__(__LLVM_Emitter__ *emitter,
                                        __Ast_Block__ *block,
                                        __Ast_Type__ *return_type,
                                        int *path_terminated)
{
    /* Tracks the index. */
    size_t index;
    /* Stores the terminated. */
    int terminated = 0;

    if (path_terminated == NULL)
    {
        return __LLVM_Fail__("missing L2 CFG termination output");
    }
    *path_terminated = 0;
    if (block == NULL)
    {
        return 1;
    }
    for (index = 0U; index < block->__Statement_Count__; ++index)
    {
        if (!__LLVM_Emit_Statement__(
                emitter, block->__Statements__[index], return_type, &terminated))
        {
            return 0;
        }
        if (terminated)
        {
            *path_terminated = 1;
            return 1;
        }
    }
    return 1;
}

/* Emits the LLVM statement. */
static int __LLVM_Emit_Statement__(__LLVM_Emitter__ *emitter,
                                   __Ast_Statement__ *statement,
                                   __Ast_Type__ *return_type,
                                   int *path_terminated)
{
    /* Stores the value. */
    __LLVM_Value__ value;

    if (path_terminated == NULL)
    {
        return __LLVM_Fail__("missing L2 CFG termination output");
    }
    *path_terminated = 0;
    if (statement == NULL)
    {
        return __LLVM_Fail__("missing L2 statement");
    }
    if (LLVMGetInsertBlock(emitter->builder) != NULL &&
        LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(emitter->builder)) != NULL)
    {
        return __LLVM_Fail__("attempted to emit an L2 statement after a block terminator");
    }
    switch (statement->__Kind__)
    {
        case __Ast_Statement_Variable_Declaration__:
        {
            /* Stores the name. */
            __Text_Slice__ name = {NULL, 0U};
            /* Stores the temporary. */
            __Temporary_Id__ temporary = 0U;
            if (statement->__As__.__Variable__.__Name_Kind__ == __Ast_Lvalue_Base_Identifier__)
                name = statement->__As__.__Variable__.__Name__.__Identifier__;
            else
                temporary = statement->__As__.__Variable__.__Name__.__Temporary__;
            if (__LLVM_Add_Local_Identity__(emitter,
                                            statement->__As__.__Variable__.__Name_Kind__,
                                            name,
                                            temporary,
                                            statement->__As__.__Variable__.__Slot__.__Type__) ==
                NULL)
            {
                return 0;
            }
            return 1;
        }

        case __Ast_Statement_Copy__:
        {
            /* References the destination lvalue. */
            __Ast_Lvalue__ *destination_lvalue = statement->__As__.__Copy__.__Destination__;
            /* References the destination type. */
            __Ast_Type__ *destination_type = __LLVM_Lvalue_Type__(emitter, destination_lvalue);
            /* Stores the destination resolved. */
            __Resolved_Type__ destination_resolved;
            if (destination_type != NULL &&
                __Type_Resolve__(emitter->semantic, destination_type, &destination_resolved) &&
                destination_resolved.__Kind__ == __Resolved_Type_Void__)
            {
                value = __LLVM_Emit_Expression__(
                    emitter, statement->__As__.__Copy__.__Expression__, destination_type);
                return value.value != NULL;
            }
            {
                /* Stores the destination. */
                __LLVM_Place__ destination = __LLVM_Emit_Place__(emitter, destination_lvalue);
                if (destination.address == NULL || destination.type == NULL)
                {
                    return 0;
                }
                value = __LLVM_Emit_Expression__(
                    emitter, statement->__As__.__Copy__.__Expression__, destination.type);
                if (value.value == NULL)
                {
                    return 0;
                }
                value = __LLVM_Coerce__(emitter, value, destination.type);
                if (value.value == NULL)
                {
                    return 0;
                }
                LLVMBuildStore(emitter->builder, value.value, destination.address);
                return 1;
            }
        }

        case __Ast_Statement_Return__:
            if (statement->__As__.__Return__ == NULL)
            {
                /* Stores the resolved return. */
                __Resolved_Type__ resolved_return;
                if (return_type == NULL ||
                    !__Type_Resolve__(emitter->semantic, return_type, &resolved_return) ||
                    resolved_return.__Kind__ != __Resolved_Type_Void__)
                    return __LLVM_Fail__(
                        "Direct LLVM value-return function is missing a return value");
                LLVMBuildRetVoid(emitter->builder);
                *path_terminated = 1;
                return 1;
            }
            value = __LLVM_Emit_Expression__(emitter, statement->__As__.__Return__, return_type);
            if (value.value == NULL)
            {
                return 0;
            }
            value = __LLVM_Coerce__(emitter, value, return_type);
            if (value.value == NULL)
            {
                return 0;
            }
            LLVMBuildRet(emitter->builder, value.value);
            *path_terminated = 1;
            return 1;

        case __Ast_Statement_If__:
        {
            /* Stores the condition. */
            __LLVM_Value__ condition =
                __LLVM_Emit_Expression__(emitter, statement->__As__.__If__.__Condition__, NULL);
            /* Stores the condition type. */
            __Resolved_Type__ condition_type;
            /* Stores the current. */
            LLVMBasicBlockRef current = LLVMGetInsertBlock(emitter->builder);
            /* Stores the function. */
            LLVMValueRef function;
            /* Stores the then block. */
            LLVMBasicBlockRef then_block;
            /* Stores the else block. */
            LLVMBasicBlockRef else_block;
            /* Stores the then exit. */
            LLVMBasicBlockRef then_exit;
            /* Stores the else exit. */
            LLVMBasicBlockRef else_exit;
            /* Stores the merge block. */
            LLVMBasicBlockRef merge_block = NULL;
            /* Stores the then terminated. */
            int then_terminated = 0;
            /* Stores the else terminated. */
            int else_terminated = 0;

            if (condition.value == NULL || condition.type == NULL ||
                !__Type_Resolve__(emitter->semantic, condition.type, &condition_type) ||
                condition_type.__Kind__ != __Resolved_Type_Boolean__)
            {
                return __LLVM_Fail__("L2 conditional qualification requires a boolean condition");
            }
            if (current == NULL || (function = LLVMGetBasicBlockParent(current)) == NULL)
            {
                return __LLVM_Fail__("cannot locate function for L2 conditional");
            }
            then_block = LLVMAppendBasicBlockInContext(emitter->context, function, "if.then");
            else_block = LLVMAppendBasicBlockInContext(emitter->context, function, "if.else");
            LLVMBuildCondBr(emitter->builder, condition.value, then_block, else_block);

            LLVMPositionBuilderAtEnd(emitter->builder, then_block);
            if (!__LLVM_Emit_Statement_List__(
                    emitter, statement->__As__.__If__.__Then__, return_type, &then_terminated))
            {
                return 0;
            }
            then_exit = LLVMGetInsertBlock(emitter->builder);
            if (then_exit == NULL)
            {
                return __LLVM_Fail__("lost LLVM insertion block while lowering then branch");
            }
            if ((LLVMGetBasicBlockTerminator(then_exit) != NULL) != (then_terminated != 0))
            {
                return __LLVM_Fail__("inconsistent then-branch CFG termination state");
            }

            LLVMPositionBuilderAtEnd(emitter->builder, else_block);
            if (!__LLVM_Emit_Statement_List__(
                    emitter, statement->__As__.__If__.__Else__, return_type, &else_terminated))
            {
                return 0;
            }
            else_exit = LLVMGetInsertBlock(emitter->builder);
            if (else_exit == NULL)
            {
                return __LLVM_Fail__("lost LLVM insertion block while lowering else branch");
            }
            if ((LLVMGetBasicBlockTerminator(else_exit) != NULL) != (else_terminated != 0))
            {
                return __LLVM_Fail__("inconsistent else-branch CFG termination state");
            }

            if (then_terminated && else_terminated)
            {
                *path_terminated = 1;
                return 1;
            }

            merge_block = LLVMAppendBasicBlockInContext(emitter->context, function, "if.end");
            if (!then_terminated)
            {
                LLVMPositionBuilderAtEnd(emitter->builder, then_exit);
                LLVMBuildBr(emitter->builder, merge_block);
            }
            if (!else_terminated)
            {
                LLVMPositionBuilderAtEnd(emitter->builder, else_exit);
                LLVMBuildBr(emitter->builder, merge_block);
            }
            LLVMPositionBuilderAtEnd(emitter->builder, merge_block);
            return 1;
        }

        case __Ast_Statement_While__:
        {
            /* Stores the current. */
            LLVMBasicBlockRef current = LLVMGetInsertBlock(emitter->builder);
            /* Stores the function. */
            LLVMValueRef function;
            /* Stores the condition block. */
            LLVMBasicBlockRef condition_block;
            /* Stores the body block. */
            LLVMBasicBlockRef body_block;
            /* Stores the exit block. */
            LLVMBasicBlockRef exit_block;
            /* Stores the body exit. */
            LLVMBasicBlockRef body_exit;
            /* Stores the condition. */
            __LLVM_Value__ condition;
            /* Stores the condition type. */
            __Resolved_Type__ condition_type;
            /* Stores the body terminated. */
            int body_terminated = 0;

            if (current == NULL || (function = LLVMGetBasicBlockParent(current)) == NULL)
            {
                return __LLVM_Fail__("cannot locate function for L2.3 while");
            }
            condition_block =
                LLVMAppendBasicBlockInContext(emitter->context, function, "while.condition");
            body_block = LLVMAppendBasicBlockInContext(emitter->context, function, "while.body");
            exit_block = LLVMAppendBasicBlockInContext(emitter->context, function, "while.exit");
            LLVMBuildBr(emitter->builder, condition_block);

            LLVMPositionBuilderAtEnd(emitter->builder, condition_block);
            condition =
                __LLVM_Emit_Expression__(emitter, statement->__As__.__While__.__Condition__, NULL);
            if (condition.value == NULL || condition.type == NULL ||
                !__Type_Resolve__(emitter->semantic, condition.type, &condition_type) ||
                condition_type.__Kind__ != __Resolved_Type_Boolean__)
            {
                return __LLVM_Fail__("L2.3 while requires a boolean condition");
            }
            LLVMBuildCondBr(emitter->builder, condition.value, body_block, exit_block);

            LLVMPositionBuilderAtEnd(emitter->builder, body_block);
            if (!__LLVM_Emit_Statement_List__(
                    emitter, statement->__As__.__While__.__Body__, return_type, &body_terminated))
            {
                return 0;
            }
            body_exit = LLVMGetInsertBlock(emitter->builder);
            if (body_exit == NULL)
            {
                return __LLVM_Fail__("lost LLVM insertion block while lowering while body");
            }
            if ((LLVMGetBasicBlockTerminator(body_exit) != NULL) != (body_terminated != 0))
            {
                return __LLVM_Fail__("inconsistent while-body CFG termination state");
            }
            if (!body_terminated)
            {
                LLVMBuildBr(emitter->builder, condition_block);
            }
            LLVMPositionBuilderAtEnd(emitter->builder, exit_block);
            return 1;
        }

        case __Ast_Statement_Initialize_Record__:
        {
            /* Stores the destination. */
            __LLVM_Place__ destination =
                __LLVM_Emit_Place__(emitter, statement->__As__.__Record__.__Destination__);
            /* Stores the resolved. */
            __Resolved_Type__ resolved;
            /* References the declaration. */
            __Ast_Type_Declaration__ *declaration;
            /* Tracks the input index. */
            size_t input_index;
            if (destination.address == NULL || destination.type == NULL ||
                !__Type_Resolve__(emitter->semantic, destination.type, &resolved) ||
                resolved.__Kind__ != __Resolved_Type_Struct__ || resolved.__Named__ == NULL ||
                (declaration = resolved.__Named__->__Declaration__) == NULL ||
                declaration->__Kind__ != __Ast_Type_Decl_Struct__)
            {
                return __LLVM_Fail__(
                    "L2.4 record initialization requires canonical struct storage");
            }
            if (statement->__As__.__Record__.__Field_Count__ !=
                declaration->__As__.__Struct__.__Count__)
            {
                return __LLVM_Fail__("L2.4 record initializer disagrees with semantic field count");
            }
            for (input_index = 0U; input_index < statement->__As__.__Record__.__Field_Count__;
                 ++input_index)
            {
                /* References the input. */
                __Ast_Record_Input__ *input = &statement->__As__.__Record__.__Fields__[input_index];
                /* Tracks the field index. */
                size_t field_index = 0U;
                /* Stores the field offset. */
                size_t field_offset = 0U;
                /* References the field type. */
                __Ast_Type__ *field_type = NULL;
                /* Stores the field LLVM type. */
                LLVMTypeRef field_llvm_type;
                /* Stores the field address. */
                LLVMValueRef field_address;
                /* Stores the field value. */
                __LLVM_Value__ field_value;
                if (!__LLVM_Resolve_Struct_Field__(emitter,
                                                   destination.type,
                                                   input->__Name__,
                                                   &field_index,
                                                   &field_offset,
                                                   &field_type))
                {
                    return __LLVM_Fail__("L2.4 record initializer field is not canonical");
                }
                (void)field_offset;
                field_llvm_type = __LLVM_Type__(emitter, field_type);
                if (field_llvm_type == NULL)
                {
                    return 0;
                }
                field_address = LLVMBuildStructGEP2(emitter->builder,
                                                    destination.llvm_type,
                                                    destination.address,
                                                    (unsigned)field_index,
                                                    "record.field.init");
                field_value = __LLVM_Emit_Expression__(emitter, input->__Value__, field_type);
                if (field_value.value == NULL)
                {
                    return 0;
                }
                field_value = __LLVM_Coerce__(emitter, field_value, field_type);
                if (field_value.value == NULL)
                {
                    return 0;
                }
                LLVMBuildStore(emitter->builder, field_value.value, field_address);
            }
            return 1;
        }

        case __Ast_Statement_Match__:
            return __LLVM_Emit_Match__(emitter, statement, return_type, path_terminated);

        case __Ast_Statement_Initialize_Vector__:
        {
            /* Stores the destination. */
            __LLVM_Place__ destination =
                __LLVM_Emit_Place__(emitter, statement->__As__.__Aggregate__.__Destination__);
            /* Stores the resolved. */
            __Resolved_Type__ resolved;
            /* Stores the element LLVM type. */
            LLVMTypeRef element_llvm_type;
            /* Stores the element pointer type. */
            LLVMTypeRef element_pointer_type;
            /* Stores the data. */
            LLVMValueRef data;
            /* Stores the vector value. */
            LLVMValueRef vector_value;
            /* Stores the element size. */
            size_t element_size = 0U;
            /* Stores the element alignment. */
            size_t element_alignment = 1U;
            /* Stores the length. */
            size_t length = statement->__As__.__Aggregate__.__Value_Count__;
            /* Stores the capacity. */
            size_t capacity;
            /* Stores the backing size. */
            size_t backing_size = 0U;
            /* Tracks the index. */
            size_t index;
            if (destination.address == NULL || destination.type == NULL ||
                !__Type_Resolve__(emitter->semantic, destination.type, &resolved) ||
                resolved.__Kind__ != __Resolved_Type_Vector__ || resolved.__Inner__ == NULL ||
                !__Layout_Type__(
                    emitter->semantic, resolved.__Inner__, &element_size, &element_alignment))
            {
                return __LLVM_Fail__(
                    "L2.7 vector initialization requires canonical vector storage");
            }
            (void)element_alignment;
            capacity = __Memory_Vector_Initial_Capacity__(length);
            if (!__Memory_Element_Bytes__(element_size, capacity, &backing_size))
            {
                return __LLVM_Fail__("L2.7 vector backing byte size overflow");
            }
            element_llvm_type = __LLVM_Type__(emitter, resolved.__Inner__);
            if (element_llvm_type == NULL)
                return 0;
            element_pointer_type = LLVMPointerType(element_llvm_type, 0U);
            data =
                __LLVM_Allocate_Bytes__(emitter, backing_size, element_pointer_type, "vector.data");
            if (data == NULL)
                return 0;
            for (index = 0U; index < length; ++index)
            {
                /* Tracks the LLVM index. */
                LLVMValueRef llvm_index = LLVMConstInt(
                    LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)index, 0);
                /* Stores the element place. */
                LLVMValueRef element_place = LLVMBuildGEP2(emitter->builder,
                                                           element_llvm_type,
                                                           data,
                                                           &llvm_index,
                                                           1U,
                                                           "vector.element.init");
                /* Stores the element. */
                __LLVM_Value__ element =
                    __LLVM_Emit_Atom__(emitter,
                                       &statement->__As__.__Aggregate__.__Values__[index],
                                       resolved.__Inner__);
                if (element.value == NULL)
                    return 0;
                element = __LLVM_Coerce__(emitter, element, resolved.__Inner__);
                if (element.value == NULL)
                    return 0;
                LLVMBuildStore(emitter->builder, element.value, element_place);
            }
            vector_value = LLVMConstNull(destination.llvm_type);
            vector_value =
                LLVMBuildInsertValue(emitter->builder, vector_value, data, 0U, "vector.with.data");
            vector_value = LLVMBuildInsertValue(
                emitter->builder,
                vector_value,
                LLVMConstInt(
                    LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)length, 0),
                1U,
                "vector.with.length");
            vector_value = LLVMBuildInsertValue(
                emitter->builder,
                vector_value,
                LLVMConstInt(
                    LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)capacity, 0),
                2U,
                "vector.with.capacity");
            LLVMBuildStore(emitter->builder, vector_value, destination.address);
            return 1;
        }

        case __Ast_Statement_Initialize_Box__:
        {
            /* Stores the destination. */
            __LLVM_Place__ destination =
                __LLVM_Emit_Place__(emitter, statement->__As__.__Box__.__Destination__);
            /* Stores the resolved. */
            __Resolved_Type__ resolved;
            /* Stores the inner LLVM type. */
            LLVMTypeRef inner_llvm_type;
            /* Stores the pointer type. */
            LLVMTypeRef pointer_type;
            /* Stores the storage. */
            LLVMValueRef storage;
            /* Stores the inner size. */
            size_t inner_size = 0U;
            /* Stores the inner alignment. */
            size_t inner_alignment = 1U;
            /* Stores the boxed. */
            __LLVM_Value__ boxed;
            if (destination.address == NULL || destination.type == NULL ||
                !__Type_Resolve__(emitter->semantic, destination.type, &resolved) ||
                resolved.__Kind__ != __Resolved_Type_Box__ || resolved.__Inner__ == NULL ||
                !__Layout_Type__(
                    emitter->semantic, resolved.__Inner__, &inner_size, &inner_alignment) ||
                inner_size == 0U)
            {
                return __LLVM_Fail__("L2.6 box construction requires canonical box inner layout");
            }
            (void)inner_alignment;
            inner_llvm_type = __LLVM_Type__(emitter, resolved.__Inner__);
            if (inner_llvm_type == NULL)
                return 0;
            pointer_type = LLVMPointerType(inner_llvm_type, 0U);
            storage = __LLVM_Allocate_Bytes__(emitter, inner_size, pointer_type, "box.storage");
            if (storage == NULL)
                return 0;
            boxed = __LLVM_Emit_Atom__(
                emitter, &statement->__As__.__Box__.__Value__, resolved.__Inner__);
            if (boxed.value == NULL)
                return 0;
            boxed = __LLVM_Coerce__(emitter, boxed, resolved.__Inner__);
            if (boxed.value == NULL)
                return 0;
            LLVMBuildStore(emitter->builder, boxed.value, storage);
            LLVMBuildStore(emitter->builder, storage, destination.address);
            return 1;
        }
    }
    return __LLVM_Fail__("unknown L2 statement");
}
