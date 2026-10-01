/* Owns Stage0 LLVM function declaration and body lowering. */

#include "llvm/llvm_functions.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_statements.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "llvm/runtime/llvm_runtime_process.h"
#include "kernel/name/name.h"
#include "kernel/type/type.h"
#include <stdio.h>
#include <stdlib.h>

/* Finds the LLVM function by semantic. */
__LLVM_Function__ *__LLVM_Find_Function_By_Semantic__(__LLVM_Emitter__ *emitter,
                                                             __Semantic_Function_Entry__ *semantic)
{
    /* Tracks the index. */
    size_t index;
    for (index = 0U; index < emitter->function_count; ++index)
    {
        if (emitter->functions[index].semantic == semantic)
        {
            return &emitter->functions[index];
        }
    }
    return NULL;
}

/* Resolves the LLVM ordinary callee. */
__Semantic_Function_Entry__ *__LLVM_Resolve_Ordinary_Callee__(__LLVM_Emitter__ *emitter,
                                                                     __Ast_Expression__ *expression)
{
    /* References the function lvalue. */
    __Ast_Lvalue__ *function_lvalue;
    /* References the callee. */
    __Semantic_Function_Entry__ *callee = NULL;
    /* Stores the name. */
    __Text_Slice__ name;
    if (expression == NULL || expression->__Kind__ != __Ast_Expression_Call__ ||
        emitter->current_function == NULL)
    {
        return NULL;
    }
    function_lvalue = expression->__As__.__Call__.__Function__;
    if (function_lvalue == NULL || function_lvalue->__Kind__ != __Ast_Lvalue_Base__ ||
        function_lvalue->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__)
    {
        return NULL;
    }
    name = function_lvalue->__As__.__Base__.__As__.__Identifier__;
    if (__LLVM_Find_Local__(emitter, name) != NULL ||
        __Name_Find_Builtin_Function__(name) != __Name_Builtin_None__)
    {
        return NULL;
    }
    if (__Name_Resolve_Function__(
            emitter->semantic, emitter->current_function->__Unit__, name, &callee) !=
        __Name_Lookup_Found__)
    {
        return NULL;
    }
    return callee;
}

/* Returns the LLVM declare functions. */
int __LLVM_Declare_Functions__(__LLVM_Emitter__ *emitter)
{
    /* Tracks the index. */
    size_t index;
    emitter->function_count = emitter->semantic->__Functions__.__Count__;
    if (emitter->function_count == 0U)
    {
        return __LLVM_Fail__("L2.3 semantic context contains no functions");
    }
    emitter->functions =
        (__LLVM_Function__ *)calloc(emitter->function_count, sizeof(*emitter->functions));
    if (emitter->functions == NULL)
    {
        return __LLVM_Fail__("out of memory while declaring L2.3 functions");
    }

    for (index = 0U; index < emitter->function_count; ++index)
    {
        /* References the entry. */
        __Semantic_Function_Entry__ *entry =
            (__Semantic_Function_Entry__ *)__Vector_At__(&emitter->semantic->__Functions__, index);
        /* References the function. */
        __Ast_Function__ *function;
        /* Stores the return type. */
        LLVMTypeRef return_type;
        /* Stores the function type. */
        LLVMTypeRef function_type;
        /* References the parameter types. */
        LLVMTypeRef *parameter_types = NULL;
        /* Stores the symbol. */
        char symbol[64];
        /* Tracks the parameter index. */
        size_t parameter_index;

        if (entry == NULL || (function = entry->__Function__) == NULL)
        {
            return __LLVM_Fail__("L2.3 semantic function entry is incomplete");
        }
        emitter->semantic->__Active_Unit__ = entry->__Unit__;
        return_type = __LLVM_Type__(emitter, function->__Output__.__Type__);
        if (return_type == NULL)
        {
            return 0;
        }
        if (function->__Parameter_Count__ != 0U)
        {
            parameter_types =
                (LLVMTypeRef *)calloc(function->__Parameter_Count__, sizeof(*parameter_types));
            if (parameter_types == NULL)
            {
                return __LLVM_Fail__("out of memory while declaring L2.3 parameters");
            }
        }
        for (parameter_index = 0U; parameter_index < function->__Parameter_Count__;
             ++parameter_index)
        {
            parameter_types[parameter_index] =
                __LLVM_Type__(emitter, function->__Parameters__[parameter_index].__Slot__.__Type__);
            if (parameter_types[parameter_index] == NULL)
            {
                free(parameter_types);
                return 0;
            }
        }
        function_type = LLVMFunctionType(
            return_type, parameter_types, (unsigned)function->__Parameter_Count__, 0);
        free(parameter_types);
        if (function_type == NULL)
        {
            return __LLVM_Fail__("LLVM function Type creation failed for L2.3");
        }

        /* Keep SultanC function identity; emit the host C `main` separately. */
        snprintf(symbol, sizeof(symbol), "sultanc.fn.%zu", entry->__Index__);
        emitter->functions[index].semantic = entry;
        emitter->functions[index].type = function_type;
        emitter->functions[index].value = LLVMAddFunction(emitter->module, symbol, function_type);
        if (emitter->functions[index].value == NULL)
        {
            return __LLVM_Fail__("LLVM function declaration failed for L2.3");
        }
    }
    return 1;
}

/* Emits the LLVM function body. */
int __LLVM_Emit_Function_Body__(__LLVM_Emitter__ *emitter, __LLVM_Function__ *llvm_function)
{
    /* References the entry. */
    __Semantic_Function_Entry__ *entry;
    /* References the function. */
    __Ast_Function__ *function;
    /* Stores the entry block. */
    LLVMBasicBlockRef entry_block;
    /* Tracks the index. */
    size_t index;
    /* Stores the path terminated. */
    int path_terminated = 0;

    if (llvm_function == NULL || (entry = llvm_function->semantic) == NULL ||
        (function = entry->__Function__) == NULL || function->__Body__ == NULL)
    {
        return __LLVM_Fail__("L2.3 requires concrete scalar function bodies");
    }

    emitter->semantic->__Active_Unit__ = entry->__Unit__;
    emitter->current_function = entry;
    emitter->local_count = 0U;
    emitter->allocation_block =
        LLVMAppendBasicBlockInContext(emitter->context, llvm_function->value, "stack.allocations");
    entry_block = LLVMAppendBasicBlockInContext(emitter->context, llvm_function->value, "entry");
    emitter->body_entry_block = entry_block;
    if (emitter->allocation_block == NULL || entry_block == NULL)
    {
        return __LLVM_Fail__("LLVM function-entry block creation failed");
    }
    LLVMPositionBuilderAtEnd(emitter->allocation_builder, emitter->allocation_block);
    LLVMPositionBuilderAtEnd(emitter->builder, entry_block);

    for (index = 0U; index < function->__Parameter_Count__; ++index)
    {
        /* References the parameter. */
        __Ast_Function_Parameter__ *parameter = &function->__Parameters__[index];
        /* References the local. */
        __LLVM_Local__ *local =
            __LLVM_Add_Local__(emitter, parameter->__Name__, parameter->__Slot__.__Type__);
        /* Stores the incoming. */
        LLVMValueRef incoming;
        if (local == NULL)
        {
            return 0;
        }
        incoming = LLVMGetParam(llvm_function->value, (unsigned)index);
        if (incoming == NULL)
        {
            return __LLVM_Fail__("LLVM incoming parameter missing for L2.3 function");
        }
        LLVMBuildStore(emitter->builder, incoming, local->address);
    }

    if (!__LLVM_Emit_Statement_List__(
            emitter, function->__Body__, function->__Output__.__Type__, &path_terminated))
    {
        return 0;
    }
    if (!path_terminated)
    {
        /* Stores the last. */
        LLVMBasicBlockRef last = LLVMGetInsertBlock(emitter->builder);
        /* Stores the output resolved. */
        __Resolved_Type__ output_resolved;
        if (last == NULL)
            return __LLVM_Fail__("Direct LLVM lost the reachable function exit block");
        if (LLVMGetBasicBlockTerminator(last) == NULL)
        {
            if (!__Type_Resolve__(
                    emitter->semantic, function->__Output__.__Type__, &output_resolved) ||
                output_resolved.__Kind__ != __Resolved_Type_Void__)
                return __LLVM_Fail__("non-void SultanC function has a reachable unterminated exit");
            LLVMBuildRetVoid(emitter->builder);
        }
    }
    if (LLVMGetBasicBlockTerminator(emitter->allocation_block) != NULL)
    {
        return __LLVM_Fail__("Direct LLVM function-entry allocation owner was terminated early");
    }
    LLVMPositionBuilderAtEnd(emitter->allocation_builder, emitter->allocation_block);
    if (LLVMBuildBr(emitter->allocation_builder, entry_block) == NULL)
    {
        return __LLVM_Fail__("Direct LLVM could not connect function-entry allocations to body");
    }
    emitter->allocation_block = NULL;
    emitter->body_entry_block = NULL;
    return 1;
}

/* Emits the LLVM process entry. */
int __LLVM_Emit_Process_Entry__(__LLVM_Emitter__ *emitter)
{
    /* References the language entry. */
    __LLVM_Function__ *language_entry;
    /* Stores the output resolved. */
    __Resolved_Type__ output_resolved;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the argv type. */
    LLVMTypeRef argv_type = LLVMPointerType(i8_pointer, 0U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[2];
    /* Stores the main type. */
    LLVMTypeRef main_type;
    /* Stores the main value. */
    LLVMValueRef main_value;
    /* Stores the block. */
    LLVMBasicBlockRef block;
    /* Stores the argument count. */
    LLVMValueRef argc;
    /* References the argument vector. */
    LLVMValueRef argv;
    /* Stores the user argc. */
    LLVMValueRef user_argc;
    /* Stores the call. */
    LLVMValueRef call;
    /* Stores the status. */
    LLVMValueRef status;

    language_entry = __LLVM_Find_Function_By_Semantic__(emitter, emitter->semantic->__Main__);
    if (language_entry == NULL || emitter->semantic->__Main__ == NULL ||
        emitter->semantic->__Main__->__Function__ == NULL ||
        !__Type_Resolve__(emitter->semantic,
                          emitter->semantic->__Main__->__Function__->__Output__.__Type__,
                          &output_resolved) ||
        (output_resolved.__Kind__ != __Resolved_Type_Signed_Integer__ &&
         output_resolved.__Kind__ != __Resolved_Type_Unsigned_Integer__) ||
        output_resolved.__Bits__ == 0U)
    {
        return __LLVM_Fail__("cannot bridge canonical SultanC entry to host process ABI");
    }

    if (!__LLVM_Ensure_Process_Globals__(emitter))
        return 0;

    parameters[0] = i32;
    parameters[1] = argv_type;
    main_type = LLVMFunctionType(i32, parameters, 2U, 0);
    main_value = LLVMAddFunction(emitter->module, "main", main_type);
    if (main_value == NULL)
        return __LLVM_Fail__("cannot create host process entry wrapper");
    block = LLVMAppendBasicBlockInContext(emitter->context, main_value, "entry");
    LLVMPositionBuilderAtEnd(emitter->builder, block);
    argc = LLVMGetParam(main_value, 0U);
    argv = LLVMGetParam(main_value, 1U);
    user_argc = LLVMBuildSub(emitter->builder,
                             LLVMBuildSExt(emitter->builder, argc, i64, "argc.i64"),
                             LLVMConstInt(i64, 1U, 0),
                             "user.argc");
    LLVMBuildStore(emitter->builder, user_argc, emitter->process_argc_global);
    LLVMBuildStore(emitter->builder, argv, emitter->process_argv_global);
    call = LLVMBuildCall2(emitter->builder,
                          language_entry->type,
                          language_entry->value,
                          NULL,
                          0U,
                          "language.entry.status");
    if (call == NULL)
        return __LLVM_Fail__("cannot call canonical SultanC entry");
    if (output_resolved.__Bits__ > 32U)
        status = LLVMBuildTrunc(emitter->builder, call, i32, "process.status");
    else if (output_resolved.__Bits__ < 32U)
        status = output_resolved.__Kind__ == __Resolved_Type_Signed_Integer__
                     ? LLVMBuildSExt(emitter->builder, call, i32, "process.status")
                     : LLVMBuildZExt(emitter->builder, call, i32, "process.status");
    else
        status = call;
    LLVMBuildRet(emitter->builder, status);
    return 1;
}
