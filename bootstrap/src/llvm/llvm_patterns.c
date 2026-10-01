/* Owns Stage0 LLVM pattern and match lowering. */

#include "llvm/llvm_patterns.h"
#include "llvm/llvm_aggregates.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_expressions.h"
#include "llvm/llvm_statements.h"
#include "llvm/llvm_types.h"
#include "llvm/llvm_values.h"
#include "kernel/pattern/pattern.h"
#include "kernel/type/type.h"
#include <stdlib.h>
#include <string.h>

/* Binds the LLVM pattern value. */
static int __LLVM_Bind_Pattern_Value__(__LLVM_Emitter__ *emitter,
                                       __Ast_Type__ *value_type,
                                       __Ast_Pattern__ *pattern,
                                       LLVMValueRef storage,
                                       LLVMTypeRef llvm_type,
                                       int tagged_discriminated,
                                       size_t constructor_index)
{
    /* Tracks the index. */
    size_t index;
    if (pattern == NULL || value_type == NULL || storage == NULL || llvm_type == NULL)
    {
        return __LLVM_Fail__("L2.5 missing canonical pattern value");
    }
    switch (pattern->__Kind__)
    {
        case __Ast_Pattern_Wildcard__:
            return 1;

        case __Ast_Pattern_Binding__:
        {
            /* References the local. */
            __LLVM_Local__ *local =
                __LLVM_Add_Local__(emitter, pattern->__As__.__Binding__, value_type);
            /* Stores the loaded. */
            LLVMValueRef loaded;
            if (local == NULL)
            {
                return 0;
            }
            loaded = LLVMBuildLoad2(emitter->builder, llvm_type, storage, "match.binding.load");
            LLVMBuildStore(emitter->builder, loaded, local->address);
            return 1;
        }

        case __Ast_Pattern_Struct__:
            for (index = 0U; index < pattern->__As__.__Struct__.__Field_Count__; ++index)
            {
                /* References the field. */
                __Ast_Struct_Pattern_Field__ *field = &pattern->__As__.__Struct__.__Fields__[index];
                /* Tracks the field index. */
                size_t field_index = 0U;
                /* References the field type. */
                __Ast_Type__ *field_type = NULL;
                /* Stores the field LLVM type. */
                LLVMTypeRef field_llvm_type;
                /* Stores the field storage. */
                LLVMValueRef field_storage;
                if (!__LLVM_Resolve_Struct_Field__(
                        emitter, value_type, field->__Name__, &field_index, NULL, &field_type))
                {
                    return __LLVM_Fail__("L2.5 struct pattern field lost canonical identity");
                }
                field_llvm_type = __LLVM_Type__(emitter, field_type);
                if (field_llvm_type == NULL)
                {
                    return 0;
                }
                field_storage = LLVMBuildStructGEP2(emitter->builder,
                                                    llvm_type,
                                                    storage,
                                                    (unsigned)field_index,
                                                    "match.struct.field");
                if (!__LLVM_Bind_Pattern_Value__(emitter,
                                                 field_type,
                                                 field->__Pattern__,
                                                 field_storage,
                                                 field_llvm_type,
                                                 field->__Pattern__ != NULL &&
                                                     field->__Pattern__->__Kind__ ==
                                                         __Ast_Pattern_Enum__,
                                                 0U))
                {
                    return 0;
                }
            }
            return 1;

        case __Ast_Pattern_Enum__:
            if (!tagged_discriminated)
            {
                return __LLVM_Fail__(
                    "L2.5 nested refutable constructor pattern requires separate match dispatch");
            }
            {
                /* Stores the analysis. */
                __Pattern_Analysis__ analysis;
                /* Stores the error. */
                __Pattern_Error__ error;
                memset(&error, 0, sizeof(error));
                __Pattern_Analysis_Init__(&analysis);
                if (!__Pattern_Analyze__(
                        emitter->semantic, value_type, pattern, &analysis, &error) ||
                    !analysis.__Has_Top_Enum_Constructor__)
                {
                    __Pattern_Analysis_Destroy__(&analysis);
                    return __LLVM_Fail__(
                        "L2.5 bound enum pattern lacks canonical constructor identity");
                }
                constructor_index = analysis.__Top_Enum_Constructor_Index__;
                __Pattern_Analysis_Destroy__(&analysis);
            }
            for (index = 0U; index < pattern->__As__.__Enum__.__Payload_Count__; ++index)
            {
                /* References the payload type. */
                __Ast_Type__ *payload_type = NULL;
                /* Stores the payload LLVM type. */
                LLVMTypeRef payload_llvm_type = NULL;
                /* Stores the payload storage. */
                LLVMValueRef payload_storage =
                    __LLVM_Copy_Tagged_Payload_To_Storage__(emitter,
                                                            value_type,
                                                            llvm_type,
                                                            storage,
                                                            constructor_index,
                                                            index,
                                                            &payload_type,
                                                            &payload_llvm_type);
                if (payload_storage == NULL ||
                    !__LLVM_Bind_Pattern_Value__(
                        emitter,
                        payload_type,
                        pattern->__As__.__Enum__.__Payloads__[index],
                        payload_storage,
                        payload_llvm_type,
                        pattern->__As__.__Enum__.__Payloads__[index] != NULL &&
                            pattern->__As__.__Enum__.__Payloads__[index]->__Kind__ ==
                                __Ast_Pattern_Enum__,
                        0U))
                {
                    return 0;
                }
            }
            return 1;
    }
    return __LLVM_Fail__("L2.5 unknown canonical pattern kind");
}

/* Emits the LLVM pattern condition. */
static LLVMValueRef __LLVM_Emit_Pattern_Condition__(__LLVM_Emitter__ *emitter,
                                                    __Ast_Type__ *value_type,
                                                    __Ast_Pattern__ *pattern,
                                                    LLVMValueRef storage,
                                                    LLVMTypeRef llvm_type)
{
    /* Stores the bool type. */
    LLVMTypeRef bool_type = LLVMIntTypeInContext(emitter->context, 1U);
    /* Stores the condition. */
    LLVMValueRef condition = LLVMConstInt(bool_type, 1U, 0);
    /* Tracks the index. */
    size_t index;
    if (value_type == NULL || pattern == NULL || storage == NULL || llvm_type == NULL)
    {
        __LLVM_Fail__("L2.5 pattern condition is missing canonical value information");
        return NULL;
    }
    if (pattern->__Kind__ == __Ast_Pattern_Wildcard__ ||
        pattern->__Kind__ == __Ast_Pattern_Binding__)
        return condition;

    if (pattern->__Kind__ == __Ast_Pattern_Struct__)
    {
        for (index = 0U; index < pattern->__As__.__Struct__.__Field_Count__; ++index)
        {
            /* References the field. */
            __Ast_Struct_Pattern_Field__ *field = &pattern->__As__.__Struct__.__Fields__[index];
            /* Tracks the field index. */
            size_t field_index = 0U;
            /* References the field type. */
            __Ast_Type__ *field_type = NULL;
            /* Stores the field LLVM type. */
            LLVMTypeRef field_llvm_type;
            /* Stores the field storage. */
            LLVMValueRef field_storage;
            /* Stores the field condition. */
            LLVMValueRef field_condition;
            if (!__LLVM_Resolve_Struct_Field__(
                    emitter, value_type, field->__Name__, &field_index, NULL, &field_type))
            {
                __LLVM_Fail__("L2.5 struct pattern condition lost canonical field identity");
                return NULL;
            }
            field_llvm_type = __LLVM_Type__(emitter, field_type);
            if (field_llvm_type == NULL)
                return NULL;
            field_storage = LLVMBuildStructGEP2(emitter->builder,
                                                llvm_type,
                                                storage,
                                                (unsigned)field_index,
                                                "match.condition.struct.field");
            field_condition = __LLVM_Emit_Pattern_Condition__(
                emitter, field_type, field->__Pattern__, field_storage, field_llvm_type);
            if (field_condition == NULL)
                return NULL;
            condition = LLVMBuildAnd(
                emitter->builder, condition, field_condition, "match.condition.struct.and");
        }
        return condition;
    }

    if (pattern->__Kind__ == __Ast_Pattern_Enum__)
    {
        /* Stores the analysis. */
        __Pattern_Analysis__ analysis;
        /* Stores the error. */
        __Pattern_Error__ error;
        /* Tracks the constructor index. */
        size_t constructor_index;
        /* Stores the tag address. */
        LLVMValueRef tag_address;
        /* Stores the tag value. */
        LLVMValueRef tag_value;
        /* Stores the tag matches. */
        LLVMValueRef tag_matches;
        /* Stores the result storage. */
        LLVMValueRef result_storage;
        /* Stores the function. */
        LLVMValueRef function;
        /* Tracks the matched block state. */
        LLVMBasicBlockRef matched_block;
        /* Stores the merge block. */
        LLVMBasicBlockRef merge_block;
        memset(&error, 0, sizeof(error));
        __Pattern_Analysis_Init__(&analysis);
        if (!__Pattern_Analyze__(emitter->semantic, value_type, pattern, &analysis, &error) ||
            !analysis.__Has_Top_Enum_Constructor__)
        {
            __Pattern_Analysis_Destroy__(&analysis);
            __LLVM_Fail__("L2.5 enum pattern condition lacks canonical constructor identity");
            return NULL;
        }
        constructor_index = analysis.__Top_Enum_Constructor_Index__;
        __Pattern_Analysis_Destroy__(&analysis);
        tag_address = LLVMBuildStructGEP2(
            emitter->builder, llvm_type, storage, 0U, "match.condition.tag.place");
        tag_value = LLVMBuildLoad2(emitter->builder,
                                   LLVMIntTypeInContext(emitter->context, 64U),
                                   tag_address,
                                   "match.condition.tag");
        tag_matches = LLVMBuildICmp(emitter->builder,
                                    LLVMIntEQ,
                                    tag_value,
                                    LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U),
                                                 (unsigned long long)constructor_index,
                                                 0),
                                    "match.condition.tag.equal");
        result_storage = __LLVM_Allocate_Stack__(emitter, bool_type, "match.condition.result");
        LLVMBuildStore(emitter->builder, LLVMConstInt(bool_type, 0U, 0), result_storage);
        function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
        if (function == NULL)
        {
            __LLVM_Fail__("L2.5 nested pattern condition lost function CFG");
            return NULL;
        }
        matched_block =
            LLVMAppendBasicBlockInContext(emitter->context, function, "match.condition.payload");
        merge_block =
            LLVMAppendBasicBlockInContext(emitter->context, function, "match.condition.merge");
        if (matched_block == NULL || merge_block == NULL)
        {
            __LLVM_Fail__("L2.5 nested pattern condition block creation failed");
            return NULL;
        }
        LLVMBuildCondBr(emitter->builder, tag_matches, matched_block, merge_block);
        LLVMPositionBuilderAtEnd(emitter->builder, matched_block);
        condition = LLVMConstInt(bool_type, 1U, 0);
        for (index = 0U; index < pattern->__As__.__Enum__.__Payload_Count__; ++index)
        {
            /* References the payload type. */
            __Ast_Type__ *payload_type = NULL;
            /* Stores the payload LLVM type. */
            LLVMTypeRef payload_llvm_type = NULL;
            /* Stores the payload storage. */
            LLVMValueRef payload_storage =
                __LLVM_Copy_Tagged_Payload_To_Storage__(emitter,
                                                        value_type,
                                                        llvm_type,
                                                        storage,
                                                        constructor_index,
                                                        index,
                                                        &payload_type,
                                                        &payload_llvm_type);
            /* Stores the payload condition. */
            LLVMValueRef payload_condition;
            if (payload_storage == NULL)
                return NULL;
            payload_condition =
                __LLVM_Emit_Pattern_Condition__(emitter,
                                                payload_type,
                                                pattern->__As__.__Enum__.__Payloads__[index],
                                                payload_storage,
                                                payload_llvm_type);
            if (payload_condition == NULL)
                return NULL;
            condition = LLVMBuildAnd(
                emitter->builder, condition, payload_condition, "match.condition.payload.and");
        }
        LLVMBuildStore(emitter->builder, condition, result_storage);
        LLVMBuildBr(emitter->builder, merge_block);
        LLVMPositionBuilderAtEnd(emitter->builder, merge_block);
        return LLVMBuildLoad2(emitter->builder, bool_type, result_storage, "match.condition.value");
    }

    __LLVM_Fail__("L2.5 unknown canonical pattern condition kind");
    return NULL;
}

/* Releases the temporary LLVM match lowering buffers. */
static int __LLVM_Finish_Match__(LLVMBasicBlockRef *case_blocks,
                                 LLVMBasicBlockRef *arm_exits,
                                 size_t *tags,
                                 unsigned char *has_tag,
                                 unsigned char *needs_full_test,
                                 unsigned char *arm_terminated,
                                 int result)
{
    free(case_blocks);
    free(arm_exits);
    free(tags);
    free(has_tag);
    free(needs_full_test);
    free(arm_terminated);
    return result;
}

/* Emits the LLVM match. */
int __LLVM_Emit_Match__(__LLVM_Emitter__ *emitter,
                               __Ast_Statement__ *statement,
                               __Ast_Type__ *return_type,
                               int *path_terminated)
{
    /* References the value type. */
    __Ast_Type__ *value_type;
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    /* Stores the value. */
    __LLVM_Value__ value;
    /* Stores the LLVM type. */
    LLVMTypeRef llvm_type;
    /* Stores the storage. */
    LLVMValueRef storage;
    /* Stores the tag address. */
    LLVMValueRef tag_address;
    /* Stores the tag value. */
    LLVMValueRef tag_value;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the current. */
    LLVMBasicBlockRef current;
    /* Stores the dispatch block. */
    LLVMBasicBlockRef dispatch_block;
    /* Stores the merge block. */
    LLVMBasicBlockRef merge_block = NULL;
    /* References the case blocks. */
    LLVMBasicBlockRef *case_blocks = NULL;
    /* References the arm exits. */
    LLVMBasicBlockRef *arm_exits = NULL;
    /* References the tags. */
    size_t *tags = NULL;
    /* Tracks whether the tag is present. */
    unsigned char *has_tag = NULL;
    /* References the needs full test. */
    unsigned char *needs_full_test = NULL;
    /* References the arm terminated. */
    unsigned char *arm_terminated = NULL;
    /* Stores the case count. */
    size_t case_count;
    /* Tracks the index. */
    size_t index;
    /* Stores the live count. */
    size_t live_count = 0U;
    if (statement == NULL || statement->__Kind__ != __Ast_Statement_Match__ ||
        path_terminated == NULL)
    {
        return __LLVM_Fail__("L2.5 invalid match lowering request");
    }
    *path_terminated = 0;
    case_count = statement->__As__.__Match__.__Case_Count__;
    if (case_count == 0U)
    {
        return __LLVM_Fail__("L2.5 semantic match contains no cases");
    }
    value_type = __LLVM_Expression_Type__(emitter, statement->__As__.__Match__.__Value__);
    if (value_type == NULL || !__Type_Resolve__(emitter->semantic, value_type, &resolved))
    {
        return __LLVM_Fail__("Direct LLVM match requires a canonical resolved Type");
    }
    llvm_type = __LLVM_Type__(emitter, value_type);
    if (llvm_type == NULL)
    {
        return 0;
    }
    value = __LLVM_Emit_Expression__(emitter, statement->__As__.__Match__.__Value__, value_type);
    if (value.value == NULL)
    {
        return 0;
    }
    value = __LLVM_Coerce__(emitter, value, value_type);
    if (value.value == NULL)
    {
        return 0;
    }
    storage = __LLVM_Allocate_Stack__(emitter, llvm_type, "match.value.storage");
    LLVMBuildStore(emitter->builder, value.value, storage);
    tag_address = NULL;
    tag_value = NULL;
    if (resolved.__Kind__ == __Resolved_Type_Enum__ ||
        resolved.__Kind__ == __Resolved_Type_Option__ ||
        resolved.__Kind__ == __Resolved_Type_Result__)
    {
        tag_address =
            LLVMBuildStructGEP2(emitter->builder, llvm_type, storage, 0U, "match.tag.place");
        tag_value = LLVMBuildLoad2(emitter->builder,
                                   LLVMIntTypeInContext(emitter->context, 64U),
                                   tag_address,
                                   "match.tag");
    }
    current = LLVMGetInsertBlock(emitter->builder);
    if (current == NULL || (function = LLVMGetBasicBlockParent(current)) == NULL)
    {
        return __LLVM_Fail__("L2.5 cannot locate function for match CFG");
    }

    case_blocks = (LLVMBasicBlockRef *)calloc(case_count, sizeof(*case_blocks));
    arm_exits = (LLVMBasicBlockRef *)calloc(case_count, sizeof(*arm_exits));
    tags = (size_t *)calloc(case_count, sizeof(*tags));
    has_tag = (unsigned char *)calloc(case_count, sizeof(*has_tag));
    needs_full_test = (unsigned char *)calloc(case_count, sizeof(*needs_full_test));
    arm_terminated = (unsigned char *)calloc(case_count, sizeof(*arm_terminated));
    if (case_blocks == NULL || arm_exits == NULL || tags == NULL || has_tag == NULL ||
        needs_full_test == NULL || arm_terminated == NULL)
    {
        __LLVM_Fail__("out of memory while lowering L2.5 match");
        return __LLVM_Finish_Match__(
            case_blocks, arm_exits, tags, has_tag, needs_full_test, arm_terminated, 0);
    }

    for (index = 0U; index < case_count; ++index)
    {
        /* Stores the analysis. */
        __Pattern_Analysis__ analysis;
        /* Stores the error. */
        __Pattern_Error__ error;
        memset(&error, 0, sizeof(error));
        __Pattern_Analysis_Init__(&analysis);
        if (!__Pattern_Analyze__(emitter->semantic,
                                 value_type,
                                 statement->__As__.__Match__.__Cases__[index].__Pattern__,
                                 &analysis,
                                 &error))
        {
            __Pattern_Analysis_Destroy__(&analysis);
            __LLVM_Fail__("L2.5 canonical pattern analysis unexpectedly rejected semantic match");
            return __LLVM_Finish_Match__(
                case_blocks, arm_exits, tags, has_tag, needs_full_test, arm_terminated, 0);
        }
        if (analysis.__Has_Top_Enum_Constructor__)
        {
            has_tag[index] = 1U;
            tags[index] = analysis.__Top_Enum_Constructor_Index__;
            if (!analysis.__Top_Enum_Constructor_Fully_Covered__)
                needs_full_test[index] = 1U;
        }
        else if (!analysis.__Irrefutable__)
        {
            needs_full_test[index] = 1U;
        }
        __Pattern_Analysis_Destroy__(&analysis);
        case_blocks[index] = LLVMAppendBasicBlockInContext(emitter->context, function, "match.arm");
        if (case_blocks[index] == NULL)
        {
            __LLVM_Fail__("L2.5 could not create match arm block");
            return __LLVM_Finish_Match__(
                case_blocks, arm_exits, tags, has_tag, needs_full_test, arm_terminated, 0);
        }
    }

    dispatch_block = current;
    for (index = 0U; index < case_count; ++index)
    {
        LLVMPositionBuilderAtEnd(emitter->builder, dispatch_block);
        if (!has_tag[index] && !needs_full_test[index])
        {
            LLVMBuildBr(emitter->builder, case_blocks[index]);
            dispatch_block = NULL;
            break;
        }
        else
        {
            /* Stores the matches. */
            LLVMValueRef matches;
            /* Stores the next. */
            LLVMBasicBlockRef next;
            if (needs_full_test[index])
            {
                matches = __LLVM_Emit_Pattern_Condition__(
                    emitter,
                    value_type,
                    statement->__As__.__Match__.__Cases__[index].__Pattern__,
                    storage,
                    llvm_type);
                if (matches == NULL)
                    return __LLVM_Finish_Match__(case_blocks,
                                                arm_exits,
                                                tags,
                                                has_tag,
                                                needs_full_test,
                                                arm_terminated,
                                                0);
            }
            else
            {
                /* Stores the expected tag. */
                LLVMValueRef expected_tag;
                if (tag_value == NULL)
                {
                    __LLVM_Fail__("Direct LLVM constructor dispatch requires tagged storage");
                    return __LLVM_Finish_Match__(case_blocks,
                                                arm_exits,
                                                tags,
                                                has_tag,
                                                needs_full_test,
                                                arm_terminated,
                                                0);
                }
                expected_tag = LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U),
                                            (unsigned long long)tags[index],
                                            0);
                matches = LLVMBuildICmp(
                    emitter->builder, LLVMIntEQ, tag_value, expected_tag, "match.tag.equal");
            }
            next = LLVMAppendBasicBlockInContext(emitter->context, function, "match.dispatch.next");
            if (next == NULL)
            {
                __LLVM_Fail__("L2.5 could not create match dispatch block");
                return __LLVM_Finish_Match__(case_blocks,
                                            arm_exits,
                                            tags,
                                            has_tag,
                                            needs_full_test,
                                            arm_terminated,
                                            0);
            }
            LLVMBuildCondBr(emitter->builder, matches, case_blocks[index], next);
            dispatch_block = next;
        }
    }
    if (dispatch_block != NULL)
    {
        LLVMPositionBuilderAtEnd(emitter->builder, dispatch_block);
        LLVMBuildUnreachable(emitter->builder);
    }

    for (index = 0U; index < case_count; ++index)
    {
        /* Stores the saved local count. */
        size_t saved_local_count = emitter->local_count;
        /* Stores the terminated. */
        int terminated = 0;
        LLVMPositionBuilderAtEnd(emitter->builder, case_blocks[index]);
        if (!__LLVM_Bind_Pattern_Value__(emitter,
                                         value_type,
                                         statement->__As__.__Match__.__Cases__[index].__Pattern__,
                                         storage,
                                         llvm_type,
                                         has_tag[index] != 0,
                                         tags[index]))
        {
            emitter->local_count = saved_local_count;
            return __LLVM_Finish_Match__(
                case_blocks, arm_exits, tags, has_tag, needs_full_test, arm_terminated, 0);
        }
        if (!__LLVM_Emit_Statement_List__(emitter,
                                          statement->__As__.__Match__.__Cases__[index].__Body__,
                                          return_type,
                                          &terminated))
        {
            emitter->local_count = saved_local_count;
            return __LLVM_Finish_Match__(
                case_blocks, arm_exits, tags, has_tag, needs_full_test, arm_terminated, 0);
        }
        arm_exits[index] = LLVMGetInsertBlock(emitter->builder);
        if (arm_exits[index] == NULL ||
            ((LLVMGetBasicBlockTerminator(arm_exits[index]) != NULL) != (terminated != 0)))
        {
            emitter->local_count = saved_local_count;
            __LLVM_Fail__("L2.5 inconsistent match-arm CFG termination state");
            return __LLVM_Finish_Match__(
                case_blocks, arm_exits, tags, has_tag, needs_full_test, arm_terminated, 0);
        }
        arm_terminated[index] = terminated ? 1U : 0U;
        if (!terminated)
        {
            ++live_count;
        }
        emitter->local_count = saved_local_count;
    }

    if (live_count == 0U)
    {
        *path_terminated = 1;
        return __LLVM_Finish_Match__(
            case_blocks, arm_exits, tags, has_tag, needs_full_test, arm_terminated, 1);
    }

    merge_block = LLVMAppendBasicBlockInContext(emitter->context, function, "match.end");
    if (merge_block == NULL)
    {
        __LLVM_Fail__("L2.5 could not create match continuation");
        return __LLVM_Finish_Match__(
            case_blocks, arm_exits, tags, has_tag, needs_full_test, arm_terminated, 0);
    }
    for (index = 0U; index < case_count; ++index)
    {
        if (!arm_terminated[index])
        {
            LLVMPositionBuilderAtEnd(emitter->builder, arm_exits[index]);
            LLVMBuildBr(emitter->builder, merge_block);
        }
    }
    LLVMPositionBuilderAtEnd(emitter->builder, merge_block);
    return __LLVM_Finish_Match__(
        case_blocks, arm_exits, tags, has_tag, needs_full_test, arm_terminated, 1);
}
