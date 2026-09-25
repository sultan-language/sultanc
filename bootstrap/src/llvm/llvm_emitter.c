/* Emits Stage0 LLVM IR, runtime bridges, objects, and execution. */

#include "llvm/llvm_emitter.h"
#include "llvm/llvm_c_api.h"

#include "kernel/type/type.h"
#include "kernel/type/conversion.h"
#include "kernel/layout/layout.h"
#include "kernel/memory/memory.h"
#include "kernel/name/name.h"
#include "kernel/pattern/pattern.h"
#include "kernel/type/tagged.h"
#include "frontend/identifier_identity.h"

#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Defines the LLVM local structure. */
typedef struct
{
    /* Stores the name kind. */
    __Ast_Lvalue_Base_Kind__ name_kind;
    /* Stores the name. */
    __Text_Slice__ name;
    /* Stores the temporary. */
    __Temporary_Id__ temporary;
    /* References the type. */
    __Ast_Type__ *type;
    /* Stores the LLVM type. */
    LLVMTypeRef llvm_type;
    /* Stores the address. */
    LLVMValueRef address;
} __LLVM_Local__;

/* Defines the LLVM value structure. */
typedef struct
{
    /* Stores the value. */
    LLVMValueRef value;
    /* References the type. */
    __Ast_Type__ *type;
} __LLVM_Value__;

/* Defines the LLVM function structure. */
typedef struct
{
    /* References the semantic. */
    __Semantic_Function_Entry__ *semantic;
    /* Stores the value. */
    LLVMValueRef value;
    /* Stores the type. */
    LLVMTypeRef type;
} __LLVM_Function__;

/* Defines the LLVM aggregate type structure. */
typedef struct
{
    /* References the semantic. */
    __Semantic_Type_Entry__ *semantic;
    /* Stores the type. */
    LLVMTypeRef type;
} __LLVM_Aggregate_Type__;

/* Defines the LLVM place structure. */
typedef struct
{
    /* Stores the address. */
    LLVMValueRef address;
    /* Stores the LLVM type. */
    LLVMTypeRef llvm_type;
    /* References the type. */
    __Ast_Type__ *type;
} __LLVM_Place__;

/* Defines the LLVM emitter structure. */
typedef struct
{
    /* References the semantic. */
    __Semantic_Context__ *semantic;
    /* Stores the context. */
    LLVMContextRef context;
    /* Stores the module. */
    LLVMModuleRef module;
    /* Stores the builder. */
    LLVMBuilderRef builder;
    /* Stores the allocation builder. */
    LLVMBuilderRef allocation_builder;
    /* Stores the allocation block. */
    LLVMBasicBlockRef allocation_block;
    /* Stores the body entry block. */
    LLVMBasicBlockRef body_entry_block;
    /* References the locals. */
    __LLVM_Local__ *locals;
    /* Stores the local count. */
    size_t local_count;
    /* Stores the local capacity. */
    size_t local_capacity;
    /* References the functions. */
    __LLVM_Function__ *functions;
    /* Stores the function count. */
    size_t function_count;
    /* References the aggregate types. */
    __LLVM_Aggregate_Type__ *aggregate_types;
    /* Stores the aggregate type count. */
    size_t aggregate_type_count;
    /* References the current function. */
    __Semantic_Function_Entry__ *current_function;
    /* Stores the malloc function. */
    LLVMValueRef malloc_function;
    /* Stores the malloc type. */
    LLVMTypeRef malloc_type;
    /* Stores the realloc function. */
    LLVMValueRef realloc_function;
    /* Stores the realloc type. */
    LLVMTypeRef realloc_type;
    /* Stores the memcmp function. */
    LLVMValueRef memcmp_function;
    /* Stores the memcmp type. */
    LLVMTypeRef memcmp_type;
    /* Stores the strlen function. */
    LLVMValueRef strlen_function;
    /* Stores the strlen type. */
    LLVMTypeRef strlen_type;
    /* Stores the process argc global. */
    LLVMValueRef process_argc_global;
    /* Stores the process argv global. */
    LLVMValueRef process_argv_global;
    /* Stores the string literal count. */
    size_t string_literal_count;
    /* Tracks the failed state. */
    int failed;
} __LLVM_Emitter__;

/* Stores the LLVM error. */
static char __LLVM_Error__[512];
/* Stores the LLVM boolean type. */
static __Ast_Type__ __LLVM_Boolean_Type__ = {.__Kind__ = __Ast_Type_Boolean__};
/* Stores the LLVM integer type. */
static __Ast_Type__ __LLVM_Integer_Type__ = {.__Kind__ = __Ast_Type_Integer__};
/* Stores the LLVM string type. */
static __Ast_Type__ __LLVM_String_Type__ = {.__Kind__ = __Ast_Type_String__};
/* Stores the LLVM void type. */
static __Ast_Type__ __LLVM_Void_Type__ = {.__Kind__ = __Ast_Type_Void__};
/* Stores the LLVM result text integer type. */
static __Ast_Type__ __LLVM_Result_Text_Integer_Type__ = {
    .__Kind__ = __Ast_Type_Result__,
    .__As__.__Result__ = {&__LLVM_String_Type__, &__LLVM_Integer_Type__}};
/* Stores the LLVM result integer integer type. */
static __Ast_Type__ __LLVM_Result_Integer_Integer_Type__ = {
    .__Kind__ = __Ast_Type_Result__,
    .__As__.__Result__ = {&__LLVM_Integer_Type__, &__LLVM_Integer_Type__}};
/* Stores the LLVM u 8 type. */
static __Ast_Type__ __LLVM_U8_Type__ = {.__Kind__ = __Ast_Type_Machine__,
                                        .__As__.__Machine__ = __Machine_U8__};

/* Returns the LLVM invalid value. */
static __LLVM_Value__ __LLVM_Invalid_Value__(void);
/* Returns the LLVM coerce. */
static __LLVM_Value__
__LLVM_Coerce__(__LLVM_Emitter__ *emitter, __LLVM_Value__ source, __Ast_Type__ *target);
/* Emits the LLVM expression. */
static __LLVM_Value__ __LLVM_Emit_Expression__(__LLVM_Emitter__ *emitter,
                                               __Ast_Expression__ *expression,
                                               __Ast_Type__ *expected);
/* Emits the LLVM trap if. */
static int
__LLVM_Emit_Trap_If__(__LLVM_Emitter__ *emitter, LLVMValueRef condition, const char *reason);

/* Compares the LLVM text. */
static int __LLVM_Text_Equals__(__Text_Slice__ left, __Text_Slice__ right)
{
    return __Identifier_Identity_Equals__(left, right);
}

/* Records a failure for the LLVM. */
static int __LLVM_Fail__(const char *message)
{
    if (__LLVM_Error__[0] == '\0')
    {
        snprintf(__LLVM_Error__, sizeof(__LLVM_Error__), "%s", message);
    }
    return 0;
}

/* Records a failure for the LLVM message. */
static int __LLVM_Fail_Message__(const char *prefix, char *message)
{
    snprintf(__LLVM_Error__,
             sizeof(__LLVM_Error__),
             "%s%s%s",
             prefix,
             message != NULL ? ": " : "",
             message != NULL ? message : "");
    if (message != NULL)
    {
        LLVMDisposeMessage(message);
    }
    return 0;
}

/* Allocates the LLVM stack. */
static LLVMValueRef
__LLVM_Allocate_Stack__(__LLVM_Emitter__ *emitter, LLVMTypeRef type, const char *name)
{
    /* Stores the value. */
    LLVMValueRef value;
    if (emitter == NULL || emitter->allocation_builder == NULL ||
        emitter->allocation_block == NULL || type == NULL)
    {
        __LLVM_Fail__("Direct LLVM stack allocation has no active function-entry owner");
        return NULL;
    }
    value = LLVMBuildAlloca(emitter->allocation_builder, type, name);
    if (value == NULL)
    {
        __LLVM_Fail__("Direct LLVM function-entry stack allocation failed");
    }
    return value;
}

/* Resolves the LLVM integer. */
static int __LLVM_Resolve_Integer__(__LLVM_Emitter__ *emitter,
                                    __Ast_Type__ *type,
                                    unsigned *bits,
                                    int *is_signed)
{
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    if (type == NULL || !__Type_Resolve__(emitter->semantic, type, &resolved) ||
        (resolved.__Kind__ != __Resolved_Type_Signed_Integer__ &&
         resolved.__Kind__ != __Resolved_Type_Unsigned_Integer__))
    {
        return 0;
    }
    *bits = resolved.__Bits__;
    *is_signed = resolved.__Kind__ == __Resolved_Type_Signed_Integer__;
    return *bits != 0U;
}

/* Finds the LLVM local. */
static __LLVM_Local__ *__LLVM_Find_Local__(__LLVM_Emitter__ *emitter, __Text_Slice__ name);
/* Finds the LLVM local base. */
static __LLVM_Local__ *__LLVM_Find_Local_Base__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue);

/* Finds the LLVM aggregate type. */
static __LLVM_Aggregate_Type__ *__LLVM_Find_Aggregate_Type__(__LLVM_Emitter__ *emitter,
                                                             __Semantic_Type_Entry__ *semantic)
{
    /* Tracks the index. */
    size_t index;
    for (index = 0U; index < emitter->aggregate_type_count; ++index)
    {
        if (emitter->aggregate_types[index].semantic == semantic)
        {
            return &emitter->aggregate_types[index];
        }
    }
    return NULL;
}

/* Resolves the LLVM struct field. */
static int __LLVM_Resolve_Struct_Field__(__LLVM_Emitter__ *emitter,
                                         __Ast_Type__ *parent_type,
                                         __Text_Slice__ field_name,
                                         size_t *out_index,
                                         size_t *out_offset,
                                         __Ast_Type__ **out_type)
{
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    /* References the declaration. */
    __Ast_Type_Declaration__ *declaration;
    /* Tracks the index. */
    size_t index;
    /* Stores the canonical offset. */
    size_t canonical_offset = 0U;
    /* References the canonical type. */
    __Ast_Type__ *canonical_type = NULL;

    if (parent_type == NULL || !__Type_Resolve__(emitter->semantic, parent_type, &resolved) ||
        resolved.__Kind__ != __Resolved_Type_Struct__ || resolved.__Named__ == NULL ||
        resolved.__Named__->__Declaration__ == NULL)
    {
        return 0;
    }
    declaration = resolved.__Named__->__Declaration__;
    if (declaration->__Kind__ != __Ast_Type_Decl_Struct__ ||
        !__Layout_Struct_Field__(
            emitter->semantic, resolved.__Named__, field_name, &canonical_offset, &canonical_type))
    {
        return 0;
    }
    for (index = 0U; index < declaration->__As__.__Struct__.__Count__; ++index)
    {
        /* References the field. */
        __Ast_Struct_Field__ *field = &declaration->__As__.__Struct__.__Fields__[index];
        if (__Identifier_Identity_Equals__(field->__Name__, field_name))
        {
            if (field->__Slot__.__Type__ != canonical_type)
            {
                return __LLVM_Fail__("canonical struct field Type/layout disagreement");
            }
            if (out_index != NULL)
                *out_index = index;
            if (out_offset != NULL)
                *out_offset = canonical_offset;
            if (out_type != NULL)
                *out_type = canonical_type;
            return 1;
        }
    }
    return 0;
}

/* Checks whether the LLVM enum is payload free. */
static int __LLVM_Enum_Is_Payload_Free__(__Semantic_Type_Entry__ *entry)
{
    /* References the declaration. */
    __Ast_Type_Declaration__ *declaration;
    /* Tracks the index. */
    size_t index;
    if (entry == NULL || (declaration = entry->__Declaration__) == NULL ||
        declaration->__Kind__ != __Ast_Type_Decl_Enum__)
        return 0;
    for (index = 0U; index < declaration->__As__.__Enum__.__Count__; ++index)
    {
        if (declaration->__As__.__Enum__.__Constructors__[index].__Payload_Count__ != 0U)
            return 0;
    }
    return 1;
}

/* Returns the LLVM tagged layout. */
static int __LLVM_Tagged_Layout__(__LLVM_Emitter__ *emitter,
                                  __Ast_Type__ *type,
                                  size_t *payload_size,
                                  size_t *payload_offset,
                                  size_t *alignment)
{
    /* Stores the tag size. */
    size_t tag_size = 0U;
    /* Stores the size. */
    size_t size = 0U;
    /* Stores the offset. */
    size_t offset = 0U;
    /* Stores the align. */
    size_t align = 1U;
    if (!__Layout_Tagged_Storage__(emitter->semantic, type, &tag_size, &offset, &size, &align))
    {
        return __LLVM_Fail__("L2.5 could not consume canonical tagged layout");
    }
    /* Tagged storage uses an eight-byte tag followed by aligned payload bytes. */
    if (tag_size != 8U || offset != 8U || align > 8U || size > (size_t)UINT_MAX)
    {
        return __LLVM_Fail__(
            "L2.5 canonical tagged layout is not representable by current direct LLVM storage");
    }
    if (payload_size != NULL)
        *payload_size = size;
    if (payload_offset != NULL)
        *payload_offset = offset;
    if (alignment != NULL)
        *alignment = align;
    return 1;
}

/* Creates the LLVM tagged literal type. */
static LLVMTypeRef __LLVM_Create_Tagged_Literal_Type__(__LLVM_Emitter__ *emitter,
                                                       __Ast_Type__ *type)
{
    /* Stores the elements. */
    LLVMTypeRef elements[2];
    /* Stores the payload size. */
    size_t payload_size = 0U;
    if (!__LLVM_Tagged_Layout__(emitter, type, &payload_size, NULL, NULL))
    {
        return NULL;
    }
    elements[0] = LLVMIntTypeInContext(emitter->context, 64U);
    elements[1] = LLVMArrayType(LLVMIntTypeInContext(emitter->context, 8U), (unsigned)payload_size);
    return LLVMStructTypeInContext(emitter->context, elements, 2U, 0);
}

/* Returns the LLVM type. */
static LLVMTypeRef __LLVM_Type__(__LLVM_Emitter__ *emitter, __Ast_Type__ *type)
{
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    if (type == NULL || !__Type_Resolve__(emitter->semantic, type, &resolved))
    {
        __LLVM_Fail__("L2.5 could not resolve Type");
        return NULL;
    }
    if (resolved.__Kind__ == __Resolved_Type_Boolean__)
    {
        return LLVMIntTypeInContext(emitter->context, 1U);
    }
    if (resolved.__Kind__ == __Resolved_Type_Signed_Integer__ ||
        resolved.__Kind__ == __Resolved_Type_Unsigned_Integer__)
    {
        return LLVMIntTypeInContext(emitter->context, resolved.__Bits__);
    }
    if ((resolved.__Kind__ == __Resolved_Type_Struct__ ||
         resolved.__Kind__ == __Resolved_Type_Enum__) &&
        resolved.__Named__ != NULL)
    {
        /* References the aggregate. */
        __LLVM_Aggregate_Type__ *aggregate =
            __LLVM_Find_Aggregate_Type__(emitter, resolved.__Named__);
        if (aggregate != NULL)
        {
            return aggregate->type;
        }
        __LLVM_Fail__("L2.5 named aggregate Type has no canonical LLVM declaration");
        return NULL;
    }
    if (resolved.__Kind__ == __Resolved_Type_Option__ ||
        resolved.__Kind__ == __Resolved_Type_Result__)
    {
        return __LLVM_Create_Tagged_Literal_Type__(emitter, type);
    }
    if (resolved.__Kind__ == __Resolved_Type_Reference__ ||
        resolved.__Kind__ == __Resolved_Type_Box__)
    {
        /* Stores the inner. */
        LLVMTypeRef inner =
            resolved.__Inner__ != NULL ? __LLVM_Type__(emitter, resolved.__Inner__) : NULL;
        if (inner == NULL)
        {
            __LLVM_Fail__("L2.6 reference/box Type has no canonical inner Type");
            return NULL;
        }
        return LLVMPointerType(inner, 0U);
    }
    if (resolved.__Kind__ == __Resolved_Type_Vector__ ||
        resolved.__Kind__ == __Resolved_Type_String__)
    {
        /* Stores the elements. */
        LLVMTypeRef elements[3];
        /* Stores the element. */
        LLVMTypeRef element = NULL;
        /* Stores the size. */
        size_t size = 0U, alignment = 0U;
        /* Stores the data offset. */
        size_t data_offset, length_offset, capacity_offset;
        if (resolved.__Kind__ == __Resolved_Type_Vector__)
        {
            element =
                resolved.__Inner__ != NULL ? __LLVM_Type__(emitter, resolved.__Inner__) : NULL;
            data_offset = __Layout_Vector_Data_Offset__();
            length_offset = __Layout_Vector_Length_Offset__();
            capacity_offset = __Layout_Vector_Capacity_Offset__();
        }
        else
        {
            element = LLVMIntTypeInContext(emitter->context, 8U);
            data_offset = __Layout_String_Data_Offset__();
            length_offset = __Layout_String_Length_Offset__();
            capacity_offset = __Layout_String_Capacity_Offset__();
        }
        if (element == NULL || !__Layout_Type__(emitter->semantic, type, &size, &alignment) ||
            size != 24U || alignment != 8U || data_offset != 0U || length_offset != 8U ||
            capacity_offset != 16U)
        {
            __LLVM_Fail__("L2.7 sequence Type disagrees with canonical Stage0 layout");
            return NULL;
        }
        elements[0] = LLVMPointerType(element, 0U);
        elements[1] = LLVMIntTypeInContext(emitter->context, 64U);
        elements[2] = LLVMIntTypeInContext(emitter->context, 64U);
        return LLVMStructTypeInContext(emitter->context, elements, 3U, 0);
    }
    if (resolved.__Kind__ == __Resolved_Type_Void__)
    {
        return LLVMVoidTypeInContext(emitter->context);
    }
    __LLVM_Fail__("Direct LLVM Type is outside the accepted L2.1-L2.7 profile");
    return NULL;
}

/* Allocates the LLVM bytes. */
static LLVMValueRef __LLVM_Allocate_Bytes__(__LLVM_Emitter__ *emitter,
                                            size_t byte_count,
                                            LLVMTypeRef target_pointer_type,
                                            const char *name)
{
    /* Stores the byte type. */
    LLVMTypeRef byte_type = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the byte pointer. */
    LLVMTypeRef byte_pointer = LLVMPointerType(byte_type, 0U);
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    /* Stores the allocated value. */
    LLVMValueRef allocated;
    /* Tracks the failed state. */
    LLVMValueRef failed;

    if (byte_count == 0U)
    {
        return LLVMConstNull(target_pointer_type);
    }
    if (emitter->malloc_function == NULL)
    {
        /* Stores the parameter values. */
        LLVMTypeRef params[1];
        params[0] = LLVMIntTypeInContext(emitter->context, 64U);
        emitter->malloc_type = LLVMFunctionType(byte_pointer, params, 1U, 0);
        emitter->malloc_function = LLVMGetNamedFunction(emitter->module, "malloc");
        if (emitter->malloc_function == NULL)
        {
            emitter->malloc_function =
                LLVMAddFunction(emitter->module, "malloc", emitter->malloc_type);
        }
        if (emitter->malloc_function == NULL)
        {
            __LLVM_Fail__("L2.6/L2.7 could not declare canonical heap allocation boundary");
            return NULL;
        }
    }
    arguments[0] = LLVMConstInt(
        LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)byte_count, 0);
    allocated = LLVMBuildCall2(emitter->builder,
                               emitter->malloc_type,
                               emitter->malloc_function,
                               arguments,
                               1U,
                               "heap.allocate");
    if (allocated == NULL)
    {
        __LLVM_Fail__("L2.6/L2.7 heap allocation call failed");
        return NULL;
    }
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           allocated,
                           LLVMConstNull(byte_pointer),
                           "heap.allocate.failed");
    if (!__LLVM_Emit_Trap_If__(emitter, failed, "heap.allocate.failure"))
    {
        return NULL;
    }
    return LLVMBuildPointerCast(emitter->builder, allocated, target_pointer_type, name);
}

/* Returns the LLVM reallocate bytes. */
static LLVMValueRef __LLVM_Reallocate_Bytes__(__LLVM_Emitter__ *emitter,
                                              LLVMValueRef pointer,
                                              LLVMValueRef byte_count,
                                              LLVMTypeRef target_pointer_type,
                                              const char *name)
{
    /* Stores the byte type. */
    LLVMTypeRef byte_type = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the byte pointer. */
    LLVMTypeRef byte_pointer = LLVMPointerType(byte_type, 0U);
    /* Stores the call arguments. */
    LLVMValueRef arguments[2];
    /* Stores the allocated value. */
    LLVMValueRef allocated;
    /* Tracks the failed state. */
    LLVMValueRef failed;

    if (pointer == NULL || byte_count == NULL || target_pointer_type == NULL)
    {
        __LLVM_Fail__("L2.7 vector reallocation is missing canonical storage facts");
        return NULL;
    }
    if (emitter->realloc_function == NULL)
    {
        /* Stores the parameter values. */
        LLVMTypeRef params[2];
        params[0] = byte_pointer;
        params[1] = LLVMIntTypeInContext(emitter->context, 64U);
        emitter->realloc_type = LLVMFunctionType(byte_pointer, params, 2U, 0);
        emitter->realloc_function = LLVMGetNamedFunction(emitter->module, "realloc");
        if (emitter->realloc_function == NULL)
            emitter->realloc_function =
                LLVMAddFunction(emitter->module, "realloc", emitter->realloc_type);
        if (emitter->realloc_function == NULL)
        {
            __LLVM_Fail__("L2.7 could not declare canonical vector reallocation boundary");
            return NULL;
        }
    }
    arguments[0] =
        LLVMBuildPointerCast(emitter->builder, pointer, byte_pointer, "vector.realloc.bytes");
    arguments[1] = byte_count;
    allocated = LLVMBuildCall2(emitter->builder,
                               emitter->realloc_type,
                               emitter->realloc_function,
                               arguments,
                               2U,
                               "vector.reallocate");
    if (allocated == NULL)
    {
        __LLVM_Fail__("L2.7 vector reallocation call failed");
        return NULL;
    }
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           allocated,
                           LLVMConstNull(byte_pointer),
                           "vector.reallocate.failed");
    if (!__LLVM_Emit_Trap_If__(emitter, failed, "vector.reallocate.failure"))
        return NULL;
    return LLVMBuildPointerCast(emitter->builder, allocated, target_pointer_type, name);
}

/* Compares the LLVM text values. */
static LLVMValueRef __LLVM_Compare_Text_Values__(__LLVM_Emitter__ *emitter,
                                                 LLVMValueRef left,
                                                 LLVMValueRef right,
                                                 __Ast_Binary_Operation__ operation)
{
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the left data. */
    LLVMValueRef left_data =
        LLVMBuildExtractValue(emitter->builder, left, 0U, "text.cmp.left.data");
    /* Stores the right data. */
    LLVMValueRef right_data =
        LLVMBuildExtractValue(emitter->builder, right, 0U, "text.cmp.right.data");
    /* Stores the left length. */
    LLVMValueRef left_length =
        LLVMBuildExtractValue(emitter->builder, left, 1U, "text.cmp.left.length");
    /* Stores the right length. */
    LLVMValueRef right_length =
        LLVMBuildExtractValue(emitter->builder, right, 1U, "text.cmp.right.length");
    /* Tracks whether the left is shorter. */
    LLVMValueRef left_shorter = LLVMBuildICmp(
        emitter->builder, LLVMIntULT, left_length, right_length, "text.cmp.left.shorter");
    /* Stores the common length. */
    LLVMValueRef common_length = LLVMBuildSelect(
        emitter->builder, left_shorter, left_length, right_length, "text.cmp.common.length");
    /* Stores the call arguments. */
    LLVMValueRef args[3];
    /* Stores the comparison result. */
    LLVMValueRef compare;
    /* Stores the zero i32 constant. */
    LLVMValueRef zero32 = LLVMConstInt(i32, 0U, 0);
    /* Tracks whether the prefix is equal. */
    LLVMValueRef prefix_equal;
    /* Tracks whether the length is equal. */
    LLVMValueRef length_equal;

    (void)i64;
    if (emitter->memcmp_function == NULL)
    {
        /* Stores the parameter values. */
        LLVMTypeRef params[3];
        params[0] = i8_pointer;
        params[1] = i8_pointer;
        params[2] = LLVMIntTypeInContext(emitter->context, 64U);
        emitter->memcmp_type = LLVMFunctionType(i32, params, 3U, 0);
        emitter->memcmp_function = LLVMGetNamedFunction(emitter->module, "memcmp");
        if (emitter->memcmp_function == NULL)
            emitter->memcmp_function =
                LLVMAddFunction(emitter->module, "memcmp", emitter->memcmp_type);
        if (emitter->memcmp_function == NULL)
        {
            __LLVM_Fail__("L2.7 could not declare bytewise text comparison boundary");
            return NULL;
        }
    }
    args[0] = LLVMBuildPointerCast(emitter->builder, left_data, i8_pointer, "text.cmp.left.bytes");
    args[1] =
        LLVMBuildPointerCast(emitter->builder, right_data, i8_pointer, "text.cmp.right.bytes");
    args[2] = common_length;
    compare = LLVMBuildCall2(emitter->builder,
                             emitter->memcmp_type,
                             emitter->memcmp_function,
                             args,
                             3U,
                             "text.cmp.bytes");
    if (compare == NULL)
        return NULL;
    prefix_equal =
        LLVMBuildICmp(emitter->builder, LLVMIntEQ, compare, zero32, "text.cmp.prefix.equal");
    length_equal = LLVMBuildICmp(
        emitter->builder, LLVMIntEQ, left_length, right_length, "text.cmp.length.equal");

    switch (operation)
    {
        case __Binary_Equal__:
            return LLVMBuildAnd(emitter->builder, prefix_equal, length_equal, "text.equal");
        case __Binary_Not_Equal__:
        {
            /* Tracks whether the prefix differs. */
            LLVMValueRef prefix_not_equal = LLVMBuildICmp(
                emitter->builder, LLVMIntNE, compare, zero32, "text.cmp.prefix.not.equal");
            /* Tracks whether the length differs. */
            LLVMValueRef length_not_equal = LLVMBuildICmp(emitter->builder,
                                                          LLVMIntNE,
                                                          left_length,
                                                          right_length,
                                                          "text.cmp.length.not.equal");
            return LLVMBuildOr(
                emitter->builder, prefix_not_equal, length_not_equal, "text.not.equal");
        }
        case __Binary_Less_Than__:
        case __Binary_Less_Or_Equal__:
        case __Binary_Greater_Than__:
        case __Binary_Greater_Or_Equal__:
        {
            /* Stores the strict predicate. */
            LLVMIntPredicate strict_predicate =
                (operation == __Binary_Less_Than__ || operation == __Binary_Less_Or_Equal__)
                    ? LLVMIntSLT
                    : LLVMIntSGT;
            /* Stores the length predicate. */
            LLVMIntPredicate length_predicate;
            /* Stores the prefix strict. */
            LLVMValueRef prefix_strict = LLVMBuildICmp(
                emitter->builder, strict_predicate, compare, zero32, "text.cmp.prefix.order");
            /* Stores the length order. */
            LLVMValueRef length_order;
            /* Stores the equal prefix length order. */
            LLVMValueRef equal_prefix_length_order;
            if (operation == __Binary_Less_Than__)
                length_predicate = LLVMIntULT;
            else if (operation == __Binary_Less_Or_Equal__)
                length_predicate = LLVMIntULE;
            else if (operation == __Binary_Greater_Than__)
                length_predicate = LLVMIntUGT;
            else
                length_predicate = LLVMIntUGE;
            length_order = LLVMBuildICmp(emitter->builder,
                                         length_predicate,
                                         left_length,
                                         right_length,
                                         "text.cmp.length.order");
            equal_prefix_length_order = LLVMBuildAnd(
                emitter->builder, prefix_equal, length_order, "text.cmp.equal.prefix.length.order");
            return LLVMBuildOr(
                emitter->builder, prefix_strict, equal_prefix_length_order, "text.cmp.order");
        }
        default:
            __LLVM_Fail__("L2.7 text comparison received a non-comparison operation");
            return NULL;
    }
}

/* Emits the LLVM string literal. */
static __LLVM_Value__
__LLVM_Emit_String_Literal__(__LLVM_Emitter__ *emitter, __Text_Slice__ text, __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* References the type. */
    __Ast_Type__ *type = expected != NULL ? expected : &__LLVM_String_Type__;
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    /* Stores the string type. */
    LLVMTypeRef string_type;
    /* Stores the byte type. */
    LLVMTypeRef byte_type;
    /* Stores the array type. */
    LLVMTypeRef array_type;
    /* Stores the global. */
    LLVMValueRef global;
    /* Stores the initializer. */
    LLVMValueRef initializer;
    /* Stores the indices. */
    LLVMValueRef indices[2];
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the value. */
    LLVMValueRef value;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the name. */
    char name[64];
    /* Stores the array length. */
    unsigned array_length;

    if (!__Type_Resolve__(emitter->semantic, type, &resolved) ||
        resolved.__Kind__ != __Resolved_Type_String__ || text.__Length__ > (size_t)UINT_MAX)
    {
        __LLVM_Fail__("L2.7 string literal requires canonical text Type");
        return result;
    }
    string_type = __LLVM_Type__(emitter, type);
    byte_type = LLVMIntTypeInContext(emitter->context, 8U);
    if (string_type == NULL)
    {
        return result;
    }
    array_length = text.__Length__ == 0U ? 1U : (unsigned)text.__Length__;
    array_type = LLVMArrayType(byte_type, array_length);
    snprintf(name, sizeof(name), "sultanc.text.%zu", emitter->string_literal_count++);
    global = LLVMAddGlobal(emitter->module, array_type, name);
    if (global == NULL)
    {
        __LLVM_Fail__("L2.7 could not materialize text literal data");
        return result;
    }
    if (text.__Length__ == 0U)
    {
        initializer = LLVMConstNull(array_type);
    }
    else
    {
        initializer =
            LLVMConstStringInContext(emitter->context, text.__Data__, (unsigned)text.__Length__, 1);
    }
    LLVMSetInitializer(global, initializer);
    LLVMSetGlobalConstant(global, 1);
    indices[0] = LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), 0U, 0);
    indices[1] = LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), 0U, 0);
    data = LLVMBuildGEP2(emitter->builder, array_type, global, indices, 2U, "text.data");
    length = LLVMConstInt(
        LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)text.__Length__, 0);
    value = LLVMConstNull(string_type);
    value = LLVMBuildInsertValue(emitter->builder, value, data, 0U, "text.with.data");
    value = LLVMBuildInsertValue(emitter->builder, value, length, 1U, "text.with.length");
    value = LLVMBuildInsertValue(emitter->builder,
                                 value,
                                 LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), 0U, 0),
                                 2U,
                                 "text.with.capacity");
    result.value = value;
    result.type = type;
    return result;
}

/* Returns the LLVM ensure process globals. */
static int __LLVM_Ensure_Process_Globals__(__LLVM_Emitter__ *emitter)
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
static LLVMValueRef __LLVM_Strlen__(__LLVM_Emitter__ *emitter, LLVMValueRef data)
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
static __LLVM_Value__ __LLVM_Emit_Argument_Count__(__LLVM_Emitter__ *emitter,
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

/* Returns the LLVM bootstrap host identity code. */
static int __LLVM_Bootstrap_Host_Identity_Code__(__Name_Builtin_Function__ builtin,
                                                 unsigned long long *out)
{
    if (out == NULL)
        return 0;
    switch (builtin)
    {
        case __Name_Builtin_Host_Architecture__:
#if defined(__aarch64__) || defined(_M_ARM64)
            *out = 1ULL;
            return 1;
#elif defined(__x86_64__) || defined(_M_X64)
            *out = 2ULL;
            return 1;
#else
            return __LLVM_Fail__("unsupported Bootstrap host architecture");
#endif
        case __Name_Builtin_Host_Platform__:
#if defined(__APPLE__)
            *out = 1ULL;
            return 1;
#elif defined(__linux__)
            *out = 2ULL;
            return 1;
#else
            return __LLVM_Fail__("unsupported Bootstrap host platform");
#endif
        case __Name_Builtin_Host_Environment__:
#if defined(__APPLE__)
            *out = 0ULL;
            return 1;
#elif defined(__linux__) && defined(__GLIBC__)
            *out = 1ULL;
            return 1;
#else
            return __LLVM_Fail__("unsupported Bootstrap host environment");
#endif
        default:
            return __LLVM_Fail__("non-host builtin used for Bootstrap host identity");
    }
}

/* Emits the LLVM host identity. */
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
    if (!__LLVM_Bootstrap_Host_Identity_Code__(builtin, &code))
        return result;
    i64 = LLVMIntTypeInContext(emitter->context, 64U);
    result.value = LLVMConstInt(i64, code, 0);
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM argument. */
static __LLVM_Value__ __LLVM_Emit_Argument__(__LLVM_Emitter__ *emitter,
                                             __Ast_Expression__ *expression,
                                             __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Tracks the index. */
    __LLVM_Value__ index;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the argv type. */
    LLVMTypeRef argv_type = LLVMPointerType(i8_pointer, 0U);
    /* Stores the string type. */
    LLVMTypeRef string_type = __LLVM_Type__(emitter, &__LLVM_String_Type__);
    /* Stores the argument count. */
    LLVMValueRef argc;
    /* References the argument vector. */
    LLVMValueRef argv;
    /* Stores the negative. */
    LLVMValueRef negative;
    /* Stores the too large. */
    LLVMValueRef too_large;
    /* Stores the invalid. */
    LLVMValueRef invalid;
    /* Tracks the host index. */
    LLVMValueRef host_index;
    /* Stores the slot. */
    LLVMValueRef slot;
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the value. */
    LLVMValueRef value;
    if (expression->__As__.__Call__.__Argument_Count__ != 1U ||
        !__LLVM_Ensure_Process_Globals__(emitter) || string_type == NULL)
    {
        __LLVM_Fail__("argument builtin disagrees with canonical semantics");
        return result;
    }
    index = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
    if (index.value == NULL)
        return result;
    index = __LLVM_Coerce__(emitter, index, &__LLVM_Integer_Type__);
    if (index.value == NULL)
        return result;
    argc = LLVMBuildLoad2(emitter->builder, i64, emitter->process_argc_global, "argument.count");
    negative = LLVMBuildICmp(emitter->builder,
                             LLVMIntSLT,
                             index.value,
                             LLVMConstInt(i64, 0U, 0),
                             "argument.index.negative");
    too_large =
        LLVMBuildICmp(emitter->builder, LLVMIntUGE, index.value, argc, "argument.index.large");
    invalid = LLVMBuildOr(emitter->builder, negative, too_large, "argument.index.invalid");
    if (!__LLVM_Emit_Trap_If__(emitter, invalid, "argument.index.out.of.range"))
        return result;
    argv = LLVMBuildLoad2(emitter->builder, argv_type, emitter->process_argv_global, "argv");
    host_index = LLVMBuildAdd(
        emitter->builder, index.value, LLVMConstInt(i64, 1U, 0), "argument.host.index");
    slot = LLVMBuildGEP2(emitter->builder, i8_pointer, argv, &host_index, 1U, "argument.slot");
    data = LLVMBuildLoad2(emitter->builder, i8_pointer, slot, "argument.data");
    length = __LLVM_Strlen__(emitter, data);
    if (length == NULL)
        return result;
    value = LLVMConstNull(string_type);
    value = LLVMBuildInsertValue(emitter->builder, value, data, 0U, "argument.with.data");
    value = LLVMBuildInsertValue(emitter->builder, value, length, 1U, "argument.with.length");
    value = LLVMBuildInsertValue(
        emitter->builder, value, LLVMConstInt(i64, 0U, 0), 2U, "argument.with.capacity");
    result.value = value;
    result.type = &__LLVM_String_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Finds the LLVM function by semantic. */
static __LLVM_Function__ *__LLVM_Find_Function_By_Semantic__(__LLVM_Emitter__ *emitter,
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

/* Returns the LLVM builtin call identity. */
static __Name_Builtin_Function__ __LLVM_Builtin_Call_Identity__(__LLVM_Emitter__ *emitter,
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
    if (unit == NULL || !__Name_Resolve_Module_Alias_Name__(unit, name, &resolved_name) ||
        __Identifier_Identity_Equals__(resolved_name, name))
        return __Name_Builtin_None__;
    return __Name_Find_Builtin_Function__(resolved_name);
}

/* Resolves the LLVM ordinary callee. */
static __Semantic_Function_Entry__ *__LLVM_Resolve_Ordinary_Callee__(__LLVM_Emitter__ *emitter,
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

/* Finds the LLVM local. */
static __LLVM_Local__ *__LLVM_Find_Local__(__LLVM_Emitter__ *emitter, __Text_Slice__ name)
{
    /* Tracks the index. */
    size_t index;
    for (index = emitter->local_count; index > 0U; --index)
    {
        /* References the local. */
        __LLVM_Local__ *local = &emitter->locals[index - 1U];
        if (local->name_kind == __Ast_Lvalue_Base_Identifier__ &&
            __LLVM_Text_Equals__(local->name, name))
        {
            return local;
        }
    }
    return NULL;
}

/* Finds the LLVM local base. */
static __LLVM_Local__ *__LLVM_Find_Local_Base__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue)
{
    /* Tracks the index. */
    size_t index;
    if (lvalue == NULL || lvalue->__Kind__ != __Ast_Lvalue_Base__)
        return NULL;
    for (index = emitter->local_count; index > 0U; --index)
    {
        /* References the local. */
        __LLVM_Local__ *local = &emitter->locals[index - 1U];
        if (local->name_kind != lvalue->__As__.__Base__.__Kind__)
            continue;
        if (local->name_kind == __Ast_Lvalue_Base_Identifier__)
        {
            if (__LLVM_Text_Equals__(local->name, lvalue->__As__.__Base__.__As__.__Identifier__))
                return local;
        }
        else if (local->temporary == lvalue->__As__.__Base__.__As__.__Temporary__)
        {
            return local;
        }
    }
    return NULL;
}

/* Adds the LLVM local identity. */
static __LLVM_Local__ *__LLVM_Add_Local_Identity__(__LLVM_Emitter__ *emitter,
                                                   __Ast_Lvalue_Base_Kind__ name_kind,
                                                   __Text_Slice__ name,
                                                   __Temporary_Id__ temporary,
                                                   __Ast_Type__ *type)
{
    /* References the local. */
    __LLVM_Local__ *local;
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    /* Stores the LLVM type. */
    LLVMTypeRef llvm_type = __LLVM_Type__(emitter, type);
    if (llvm_type == NULL || !__Type_Resolve__(emitter->semantic, type, &resolved))
    {
        return NULL;
    }
    if (emitter->local_count == emitter->local_capacity)
    {
        /* Stores the capacity. */
        size_t capacity = emitter->local_capacity == 0U ? 8U : emitter->local_capacity * 2U;
        /* References the grown. */
        __LLVM_Local__ *grown =
            (__LLVM_Local__ *)realloc(emitter->locals, capacity * sizeof(*grown));
        if (grown == NULL)
        {
            __LLVM_Fail__("out of memory while recording LLVM locals");
            return NULL;
        }
        emitter->locals = grown;
        emitter->local_capacity = capacity;
    }
    local = &emitter->locals[emitter->local_count++];
    memset(local, 0, sizeof(*local));
    local->name_kind = name_kind;
    local->name = name;
    local->temporary = temporary;
    local->type = type;
    local->llvm_type = llvm_type;
    if (resolved.__Kind__ != __Resolved_Type_Void__)
    {
        local->address = __LLVM_Allocate_Stack__(emitter, llvm_type, "local");
    }
    return local;
}

/* Adds the LLVM local. */
static __LLVM_Local__ *
__LLVM_Add_Local__(__LLVM_Emitter__ *emitter, __Text_Slice__ name, __Ast_Type__ *type)
{
    return __LLVM_Add_Local_Identity__(emitter, __Ast_Lvalue_Base_Identifier__, name, 0U, type);
}

/* Returns the LLVM invalid value. */
static __LLVM_Value__ __LLVM_Invalid_Value__(void)
{
    /* Stores the value. */
    __LLVM_Value__ value;
    memset(&value, 0, sizeof(value));
    return value;
}

/* Returns the LLVM coerce. */
static __LLVM_Value__
__LLVM_Coerce__(__LLVM_Emitter__ *emitter, __LLVM_Value__ source, __Ast_Type__ *target)
{
    /* Stores the source bits. */
    unsigned source_bits, target_bits;
    /* Tracks whether the source is signed. */
    int source_signed, target_signed;
    /* Stores the target type. */
    LLVMTypeRef target_type;
    /* Stores the operation result. */
    __LLVM_Value__ result = source;
    if (source.value == NULL || source.type == NULL || target == NULL)
    {
        return __LLVM_Invalid_Value__();
    }
    if (__Type_Compatible__(emitter->semantic, source.type, target))
    {
        result.type = target;
        return result;
    }
    {
        /* Stores the source resolved. */
        __Resolved_Type__ source_resolved;
        /* Stores the target resolved. */
        __Resolved_Type__ target_resolved;
        /* Stores the conversion. */
        __Type_Conversion_Class__ conversion =
            __Type_Conversion_Classify__(emitter->semantic, source.type, target);
        if (__Type_Resolve__(emitter->semantic, source.type, &source_resolved) &&
            __Type_Resolve__(emitter->semantic, target, &target_resolved) &&
            source_resolved.__Kind__ == __Resolved_Type_Reference__ &&
            target_resolved.__Kind__ == __Resolved_Type_Reference__ &&
            conversion == __Type_Conversion_Implicit_Safe__)
        {
            /* Stores the target pointer. */
            LLVMTypeRef target_pointer = __LLVM_Type__(emitter, target);
            if (target_pointer == NULL)
                return __LLVM_Invalid_Value__();
            result.value = LLVMBuildPointerCast(
                emitter->builder, source.value, target_pointer, "reference.coerce");
            result.type = target;
            return result;
        }
    }
    if (!__LLVM_Resolve_Integer__(emitter, source.type, &source_bits, &source_signed) ||
        !__LLVM_Resolve_Integer__(emitter, target, &target_bits, &target_signed))
    {
        __LLVM_Fail__("Direct LLVM conversion is outside canonical representable conversions");
        return __LLVM_Invalid_Value__();
    }
    (void)target_signed;
    target_type = LLVMIntTypeInContext(emitter->context, target_bits);
    if (source_bits < target_bits)
    {
        result.value = source_signed
                           ? LLVMBuildSExt(emitter->builder, source.value, target_type, "sext")
                           : LLVMBuildZExt(emitter->builder, source.value, target_type, "zext");
    }
    else if (source_bits > target_bits)
    {
        result.value = LLVMBuildTrunc(emitter->builder, source.value, target_type, "trunc");
    }
    result.type = target;
    return result;
}

/* Emits the LLVM trap if. */
static int
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

/* Emits the LLVM expression. */
static __LLVM_Value__ __LLVM_Emit_Expression__(__LLVM_Emitter__ *emitter,
                                               __Ast_Expression__ *expression,
                                               __Ast_Type__ *expected);
/* Emits the LLVM lvalue. */
static __LLVM_Value__ __LLVM_Emit_Lvalue__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue);
/* Emits the LLVM atom. */
static __LLVM_Value__
__LLVM_Emit_Atom__(__LLVM_Emitter__ *emitter, __Ast_Atom__ *atom, __Ast_Type__ *expected);

/* Returns the LLVM lvalue type. */
static __Ast_Type__ *__LLVM_Lvalue_Type__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue)
{
    /* References the local. */
    __LLVM_Local__ *local;
    /* References the parent type. */
    __Ast_Type__ *parent_type;
    /* References the field type. */
    __Ast_Type__ *field_type = NULL;
    if (lvalue == NULL)
    {
        return NULL;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Base__)
    {
        local = __LLVM_Find_Local_Base__(emitter, lvalue);
        return local != NULL ? local->type : NULL;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Field__)
    {
        parent_type = __LLVM_Lvalue_Type__(emitter, lvalue->__As__.__Field__.__Parent__);
        if (parent_type == NULL ||
            !__LLVM_Resolve_Struct_Field__(
                emitter, parent_type, lvalue->__As__.__Field__.__Field__, NULL, NULL, &field_type))
        {
            return NULL;
        }
        return field_type;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Dereference__)
    {
        /* Stores the resolved. */
        __Resolved_Type__ resolved;
        parent_type = __LLVM_Lvalue_Type__(emitter, lvalue->__As__.__Dereference_Parent__);
        if (parent_type == NULL || !__Type_Resolve__(emitter->semantic, parent_type, &resolved) ||
            (resolved.__Kind__ != __Resolved_Type_Reference__ &&
             resolved.__Kind__ != __Resolved_Type_Box__))
        {
            return NULL;
        }
        return resolved.__Inner__;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Index__)
    {
        /* Stores the resolved. */
        __Resolved_Type__ resolved;
        parent_type = __LLVM_Lvalue_Type__(emitter, lvalue->__As__.__Index__.__Parent__);
        if (parent_type == NULL || !__Type_Resolve__(emitter->semantic, parent_type, &resolved))
        {
            return NULL;
        }
        if (resolved.__Kind__ == __Resolved_Type_String__)
            return &__LLVM_U8_Type__;
        if (resolved.__Kind__ == __Resolved_Type_Vector__ ||
            resolved.__Kind__ == __Resolved_Type_Box__)
            return resolved.__Inner__;
    }
    return NULL;
}

/* Returns the LLVM invalid place. */
static __LLVM_Place__ __LLVM_Invalid_Place__(void)
{
    /* Stores the place. */
    __LLVM_Place__ place;
    memset(&place, 0, sizeof(place));
    return place;
}

/* Emits the LLVM place. */
static __LLVM_Place__ __LLVM_Emit_Place__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue)
{
    /* Stores the place. */
    __LLVM_Place__ place = __LLVM_Invalid_Place__();
    if (lvalue == NULL)
    {
        __LLVM_Fail__("missing L2.4 aggregate place");
        return place;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Base__)
    {
        /* References the local. */
        __LLVM_Local__ *local = __LLVM_Find_Local_Base__(emitter, lvalue);
        if (local == NULL || local->address == NULL)
        {
            __LLVM_Fail__("L2.7 could not find value-producing local storage");
            return place;
        }
        place.address = local->address;
        place.llvm_type = local->llvm_type;
        place.type = local->type;
        return place;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Field__)
    {
        /* Stores the parent. */
        __LLVM_Place__ parent = __LLVM_Emit_Place__(emitter, lvalue->__As__.__Field__.__Parent__);
        /* Tracks the field index. */
        size_t field_index = 0U;
        /* Stores the field offset. */
        size_t field_offset = 0U;
        /* References the field type. */
        __Ast_Type__ *field_type = NULL;
        /* Stores the field LLVM type. */
        LLVMTypeRef field_llvm_type;
        if (parent.address == NULL || parent.type == NULL ||
            !__LLVM_Resolve_Struct_Field__(emitter,
                                           parent.type,
                                           lvalue->__As__.__Field__.__Field__,
                                           &field_index,
                                           &field_offset,
                                           &field_type))
        {
            __LLVM_Fail__("L2.4 field place is not a canonical struct field");
            return __LLVM_Invalid_Place__();
        }
        (void)field_offset; /* LLVM GEP uses the resolved static member index. */
        field_llvm_type = __LLVM_Type__(emitter, field_type);
        if (field_llvm_type == NULL)
        {
            return __LLVM_Invalid_Place__();
        }
        place.address = LLVMBuildStructGEP2(emitter->builder,
                                            parent.llvm_type,
                                            parent.address,
                                            (unsigned)field_index,
                                            "field.place");
        place.llvm_type = field_llvm_type;
        place.type = field_type;
        return place;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Dereference__)
    {
        /* Stores the pointer. */
        __LLVM_Value__ pointer =
            __LLVM_Emit_Lvalue__(emitter, lvalue->__As__.__Dereference_Parent__);
        /* Stores the resolved. */
        __Resolved_Type__ resolved;
        /* Stores the inner type. */
        LLVMTypeRef inner_type;
        if (pointer.value == NULL || pointer.type == NULL ||
            !__Type_Resolve__(emitter->semantic, pointer.type, &resolved) ||
            (resolved.__Kind__ != __Resolved_Type_Reference__ &&
             resolved.__Kind__ != __Resolved_Type_Box__) ||
            resolved.__Inner__ == NULL)
        {
            __LLVM_Fail__("L2.6 dereference requires canonical reference/box Type");
            return place;
        }
        inner_type = __LLVM_Type__(emitter, resolved.__Inner__);
        if (inner_type == NULL)
            return place;
        place.address = pointer.value;
        place.llvm_type = inner_type;
        place.type = resolved.__Inner__;
        return place;
    }
    if (lvalue->__Kind__ == __Ast_Lvalue_Index__)
    {
        /* Stores the parent. */
        __LLVM_Place__ parent = __LLVM_Emit_Place__(emitter, lvalue->__As__.__Index__.__Parent__);
        /* Stores the resolved. */
        __Resolved_Type__ resolved;
        /* References the element type. */
        __Ast_Type__ *element_type = NULL;
        /* Stores the element LLVM type. */
        LLVMTypeRef element_llvm_type;
        /* Tracks the index. */
        __LLVM_Value__ index;
        /* Stores the index 64. */
        LLVMValueRef index64;
        /* Stores the base. */
        LLVMValueRef base = NULL;
        if (parent.address == NULL || parent.type == NULL ||
            !__Type_Resolve__(emitter->semantic, parent.type, &resolved))
        {
            __LLVM_Fail__("L2.7 index parent has no canonical Type");
            return place;
        }
        index = __LLVM_Emit_Atom__(
            emitter, &lvalue->__As__.__Index__.__Index__, &__LLVM_Integer_Type__);
        if (index.value == NULL)
            return place;
        index = __LLVM_Coerce__(emitter, index, &__LLVM_Integer_Type__);
        if (index.value == NULL)
            return place;
        index64 = index.value;

        if (resolved.__Kind__ == __Resolved_Type_Vector__ ||
            resolved.__Kind__ == __Resolved_Type_String__)
        {
            /* Stores the data place. */
            LLVMValueRef data_place;
            /* Stores the length place. */
            LLVMValueRef length_place;
            /* Stores the length. */
            LLVMValueRef length;
            /* Stores the negative. */
            LLVMValueRef negative;
            /* Stores the too large. */
            LLVMValueRef too_large;
            /* Stores the invalid. */
            LLVMValueRef invalid;
            /* Stores the data offset. */
            size_t data_offset = resolved.__Kind__ == __Resolved_Type_Vector__
                                     ? __Layout_Vector_Data_Offset__()
                                     : __Layout_String_Data_Offset__();
            /* Stores the length offset. */
            size_t length_offset = resolved.__Kind__ == __Resolved_Type_Vector__
                                       ? __Layout_Vector_Length_Offset__()
                                       : __Layout_String_Length_Offset__();
            if (data_offset != 0U || length_offset != 8U)
            {
                __LLVM_Fail__("L2.7 canonical sequence projection is not representable");
                return place;
            }
            data_place = LLVMBuildStructGEP2(
                emitter->builder, parent.llvm_type, parent.address, 0U, "sequence.data.place");
            length_place = LLVMBuildStructGEP2(
                emitter->builder, parent.llvm_type, parent.address, 1U, "sequence.length.place");
            base = LLVMBuildLoad2(
                emitter->builder,
                resolved.__Kind__ == __Resolved_Type_String__
                    ? LLVMPointerType(LLVMIntTypeInContext(emitter->context, 8U), 0U)
                    : LLVMPointerType(__LLVM_Type__(emitter, resolved.__Inner__), 0U),
                data_place,
                "sequence.data");
            length = LLVMBuildLoad2(emitter->builder,
                                    LLVMIntTypeInContext(emitter->context, 64U),
                                    length_place,
                                    "sequence.length");
            negative =
                LLVMBuildICmp(emitter->builder,
                              LLVMIntSLT,
                              index64,
                              LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), 0U, 0),
                              "sequence.index.negative");
            too_large = LLVMBuildICmp(
                emitter->builder, LLVMIntUGE, index64, length, "sequence.index.out.of.range");
            invalid = LLVMBuildOr(emitter->builder, negative, too_large, "sequence.index.invalid");
            if (!__LLVM_Emit_Trap_If__(emitter, invalid, "sequence.bounds.failure"))
                return __LLVM_Invalid_Place__();
            element_type = resolved.__Kind__ == __Resolved_Type_String__ ? &__LLVM_U8_Type__
                                                                         : resolved.__Inner__;
        }
        else if (resolved.__Kind__ == __Resolved_Type_Box__)
        {
            base = LLVMBuildLoad2(emitter->builder, parent.llvm_type, parent.address, "box.data");
            element_type = resolved.__Inner__;
        }
        else
        {
            __LLVM_Fail__("L2.7 indexing requires canonical vector/text/box Type");
            return place;
        }
        element_llvm_type = __LLVM_Type__(emitter, element_type);
        if (base == NULL || element_llvm_type == NULL)
            return place;
        place.address = LLVMBuildGEP2(
            emitter->builder, element_llvm_type, base, &index64, 1U, "sequence.element.place");
        place.llvm_type = element_llvm_type;
        place.type = element_type;
        return place;
    }
    __LLVM_Fail__("place is outside the accepted L2.1-L2.7 profile");
    return place;
}

/* Emits the LLVM lvalue. */
static __LLVM_Value__ __LLVM_Emit_Lvalue__(__LLVM_Emitter__ *emitter, __Ast_Lvalue__ *lvalue)
{
    /* Stores the place. */
    __LLVM_Place__ place = __LLVM_Emit_Place__(emitter, lvalue);
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    if (place.address == NULL || place.llvm_type == NULL || place.type == NULL)
    {
        return result;
    }
    result.value = LLVMBuildLoad2(emitter->builder, place.llvm_type, place.address, "load");
    result.type = place.type;
    return result;
}

/* Emits the LLVM atom. */
static __LLVM_Value__
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

/* Returns the LLVM concrete integer type. */
static __Ast_Type__ *__LLVM_Concrete_Integer_Type__(__LLVM_Emitter__ *emitter,
                                                    __Ast_Type__ *candidate)
{
    /* Stores the bits. */
    unsigned bits;
    /* Tracks whether the value is signed. */
    int is_signed;

    return candidate != NULL && __LLVM_Resolve_Integer__(emitter, candidate, &bits, &is_signed)
               ? candidate
               : NULL;
}

/* Returns the LLVM expression integer type. */
static __Ast_Type__ *__LLVM_Expression_Integer_Type__(__LLVM_Emitter__ *emitter,
                                                      __Ast_Expression__ *expression)
{
    /* References the lvalue. */
    __Ast_Lvalue__ *lvalue;
    /* References the type. */
    __Ast_Type__ *type;

    if (expression == NULL)
    {
        return NULL;
    }
    switch (expression->__Kind__)
    {
        case __Ast_Expression_Atom__:
            if (expression->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__)
            {
                return NULL;
            }
            lvalue = expression->__As__.__Atom__.__As__.__Lvalue__;
            return __LLVM_Lvalue_Type__(emitter, lvalue);

        case __Ast_Expression_Conversion__:
            return expression->__As__.__Conversion__.__Target_Type__;

        case __Ast_Expression_Binary__:
            type =
                __LLVM_Expression_Integer_Type__(emitter, expression->__As__.__Binary__.__Left__);
            return type != NULL ? type
                                : __LLVM_Expression_Integer_Type__(
                                      emitter, expression->__As__.__Binary__.__Right__);

        case __Ast_Expression_Call__:
        {
            /* References the callee. */
            __Semantic_Function_Entry__ *callee =
                __LLVM_Resolve_Ordinary_Callee__(emitter, expression);
            if (callee != NULL)
                return callee->__Function__->__Output__.__Type__;
            switch (__LLVM_Builtin_Call_Identity__(emitter, expression))
            {
                case __Name_Builtin_Length__:
                case __Name_Builtin_Open_File_Read__:
                case __Name_Builtin_Read_File_Byte__:
                case __Name_Builtin_Read_File_Segment__:
                case __Name_Builtin_Create_File_Write__:
                case __Name_Builtin_Write_File_Segment__:
                case __Name_Builtin_Close_File__:
                case __Name_Builtin_Open_Directory__:
                case __Name_Builtin_Read_Directory_Entry__:
                case __Name_Builtin_Close_Directory__:
                case __Name_Builtin_Read_Stdin_Byte__:
                case __Name_Builtin_Read_Stdin_Segment__:
                case __Name_Builtin_Host_Architecture__:
                case __Name_Builtin_Host_Platform__:
                case __Name_Builtin_Host_Environment__:
                case __Name_Builtin_Argument_Count__:
                case __Name_Builtin_Stdout_Write__:
                case __Name_Builtin_Stderr_Write__:
                case __Name_Builtin_Path_Type__:
                case __Name_Builtin_Path_Size__:
                case __Name_Builtin_Path_Modified_Time__:
                    return &__LLVM_Integer_Type__;
                default:
                    return NULL;
            }
        }

        case __Ast_Expression_Unary__:
            return __LLVM_Expression_Integer_Type__(emitter,
                                                    expression->__As__.__Unary__.__Operand__);
    }
    return NULL;
}

/* Returns the LLVM expression type. */
static __Ast_Type__ *__LLVM_Expression_Type__(__LLVM_Emitter__ *emitter,
                                              __Ast_Expression__ *expression)
{
    if (expression == NULL)
    {
        return NULL;
    }
    if (expression->__Contextual_Type__ != NULL)
    {
        return expression->__Contextual_Type__;
    }
    switch (expression->__Kind__)
    {
        case __Ast_Expression_Atom__:
            if (expression->__As__.__Atom__.__Kind__ == __Ast_Atom_Lvalue__)
            {
                return __LLVM_Lvalue_Type__(emitter, expression->__As__.__Atom__.__As__.__Lvalue__);
            }
            if (expression->__As__.__Atom__.__Kind__ == __Ast_Atom_Literal__ &&
                expression->__As__.__Atom__.__As__.__Literal__ != NULL &&
                expression->__As__.__Atom__.__As__.__Literal__->__Kind__ == __Ast_Literal_String__)
            {
                return &__LLVM_String_Type__;
            }
            return NULL;
        case __Ast_Expression_Conversion__:
            return expression->__As__.__Conversion__.__Target_Type__;
        case __Ast_Expression_Call__:
        {
            /* References the callee. */
            __Semantic_Function_Entry__ *callee =
                __LLVM_Resolve_Ordinary_Callee__(emitter, expression);
            /* Stores the builtin. */
            __Name_Builtin_Function__ builtin;
            if (callee != NULL)
                return callee->__Function__->__Output__.__Type__;
            builtin = __LLVM_Builtin_Call_Identity__(emitter, expression);
            switch (builtin)
            {
                case __Name_Builtin_Length__:
                case __Name_Builtin_Open_File_Read__:
                case __Name_Builtin_Read_File_Byte__:
                case __Name_Builtin_Read_File_Segment__:
                case __Name_Builtin_Create_File_Write__:
                case __Name_Builtin_Write_File_Segment__:
                case __Name_Builtin_Close_File__:
                case __Name_Builtin_Open_Directory__:
                case __Name_Builtin_Read_Directory_Entry__:
                case __Name_Builtin_Close_Directory__:
                case __Name_Builtin_Read_Stdin_Byte__:
                case __Name_Builtin_Read_Stdin_Segment__:
                case __Name_Builtin_Host_Architecture__:
                case __Name_Builtin_Host_Platform__:
                case __Name_Builtin_Host_Environment__:
                case __Name_Builtin_Argument_Count__:
                case __Name_Builtin_Stdout_Write__:
                case __Name_Builtin_Stderr_Write__:
                case __Name_Builtin_Path_Type__:
                case __Name_Builtin_Path_Size__:
                case __Name_Builtin_Path_Modified_Time__:
                    return &__LLVM_Integer_Type__;
                case __Name_Builtin_Argument__:
                case __Name_Builtin_Text_From_Bytes__:
                    return &__LLVM_String_Type__;
                    return &__LLVM_Result_Text_Integer_Type__;
                case __Name_Builtin_Write_Executable_Bytes__:
                    return &__LLVM_Result_Integer_Integer_Type__;
                case __Name_Builtin_Append__:
                case __Name_Builtin_Process_Exit__:
                    return &__LLVM_Void_Type__;
                default:
                    return expression->__Contextual_Type__;
            }
        }
        case __Ast_Expression_Binary__:
            return __LLVM_Expression_Integer_Type__(emitter, expression);
        case __Ast_Expression_Unary__:
        {
            /* Stores the operation. */
            __Ast_Unary_Operation__ operation = expression->__As__.__Unary__.__Operation__;
            /* References the operand type. */
            __Ast_Type__ *operand_type;
            /* Stores the resolved. */
            __Resolved_Type__ resolved;
            if (operation == __Unary_Not__)
                return &__LLVM_Boolean_Type__;
            if (operation == __Unary_Address__ || operation == __Unary_Address_Mutable__)
                return expression->__Contextual_Type__;
            operand_type =
                __LLVM_Expression_Type__(emitter, expression->__As__.__Unary__.__Operand__);
            if (operation == __Unary_Dereference__ && operand_type != NULL &&
                __Type_Resolve__(emitter->semantic, operand_type, &resolved) &&
                (resolved.__Kind__ == __Resolved_Type_Reference__ ||
                 resolved.__Kind__ == __Resolved_Type_Box__))
                return resolved.__Inner__;
            return operand_type;
        }
    }
    return NULL;
}

/* Returns the LLVM tagged byte address. */
static LLVMValueRef __LLVM_Tagged_Byte_Address__(__LLVM_Emitter__ *emitter,
                                                 __Ast_Type__ *tagged_type,
                                                 LLVMTypeRef llvm_tagged_type,
                                                 LLVMValueRef storage,
                                                 size_t canonical_offset,
                                                 size_t access_size,
                                                 const char *name)
{
    /* Stores the payload size. */
    size_t payload_size = 0U;
    /* Stores the payload offset. */
    size_t payload_offset = 0U;
    /* Stores the byte type. */
    LLVMTypeRef byte_type;
    /* Stores the payload array type. */
    LLVMTypeRef payload_array_type;
    /* Stores the payload storage. */
    LLVMValueRef payload_storage;
    /* Stores the indices. */
    LLVMValueRef indices[2];
    /* Stores the relative. */
    size_t relative;

    if (!__LLVM_Tagged_Layout__(emitter, tagged_type, &payload_size, &payload_offset, NULL) ||
        canonical_offset < payload_offset)
    {
        __LLVM_Fail__("L2.5 tagged payload offset is outside canonical storage");
        return NULL;
    }
    relative = canonical_offset - payload_offset;
    if (relative > payload_size || access_size > payload_size - relative)
    {
        __LLVM_Fail__("L2.5 tagged payload access exceeds canonical payload storage");
        return NULL;
    }
    byte_type = LLVMIntTypeInContext(emitter->context, 8U);
    payload_array_type = LLVMArrayType(byte_type, (unsigned)payload_size);
    payload_storage = LLVMBuildStructGEP2(
        emitter->builder, llvm_tagged_type, storage, 1U, "tagged.payload.storage");
    if (payload_storage == NULL)
    {
        __LLVM_Fail__("L2.5 could not address tagged payload storage");
        return NULL;
    }
    indices[0] = LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), 0U, 0);
    indices[1] =
        LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)relative, 0);
    return LLVMBuildGEP2(emitter->builder, payload_array_type, payload_storage, indices, 2U, name);
}

/* Emits the LLVM tagged construct. */
static __LLVM_Value__ __LLVM_Emit_Tagged_Construct__(__LLVM_Emitter__ *emitter,
                                                     __Ast_Type__ *target_type,
                                                     size_t constructor_index,
                                                     __Ast_Expression__ **arguments,
                                                     size_t argument_count)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the LLVM type. */
    LLVMTypeRef llvm_type;
    /* Stores the storage. */
    LLVMValueRef storage;
    /* Stores the tag address. */
    LLVMValueRef tag_address;
    /* Tracks the payload index. */
    size_t payload_index;
    /* Stores the payload capacity. */
    size_t payload_capacity = 0U;

    if (target_type == NULL ||
        !__LLVM_Tagged_Layout__(emitter, target_type, &payload_capacity, NULL, NULL))
    {
        return result;
    }
    llvm_type = __LLVM_Type__(emitter, target_type);
    if (llvm_type == NULL)
    {
        return result;
    }
    storage = __LLVM_Allocate_Stack__(emitter, llvm_type, "tagged.construct");
    if (storage == NULL)
    {
        __LLVM_Fail__("L2.5 tagged construction storage failed");
        return result;
    }
    LLVMBuildStore(emitter->builder, LLVMConstNull(llvm_type), storage);
    tag_address = LLVMBuildStructGEP2(emitter->builder, llvm_type, storage, 0U, "tagged.tag.place");
    LLVMBuildStore(emitter->builder,
                   LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U),
                                (unsigned long long)constructor_index,
                                0),
                   tag_address);

    for (payload_index = 0U; payload_index < argument_count; ++payload_index)
    {
        /* Stores the canonical offset. */
        size_t canonical_offset = 0U;
        /* Stores the payload size. */
        size_t payload_size = 0U;
        /* Stores the payload alignment. */
        size_t payload_alignment = 1U;
        /* References the payload type. */
        __Ast_Type__ *payload_type = NULL;
        /* Stores the payload LLVM type. */
        LLVMTypeRef payload_llvm_type;
        /* Stores the payload value. */
        __LLVM_Value__ payload_value;
        /* Stores the payload temp. */
        LLVMValueRef payload_temp;
        /* Stores the destination. */
        LLVMValueRef destination;
        /* Stores the byte count. */
        LLVMValueRef byte_count;

        if (!__Layout_Tagged_Payload__(emitter->semantic,
                                       target_type,
                                       constructor_index,
                                       payload_index,
                                       &canonical_offset,
                                       &payload_type) ||
            payload_type == NULL ||
            !__Layout_Type__(emitter->semantic, payload_type, &payload_size, &payload_alignment))
        {
            __LLVM_Fail__("L2.5 constructor payload lacks canonical static layout");
            return __LLVM_Invalid_Value__();
        }
        payload_value = __LLVM_Emit_Expression__(emitter, arguments[payload_index], payload_type);
        if (payload_value.value == NULL)
        {
            return __LLVM_Invalid_Value__();
        }
        payload_value = __LLVM_Coerce__(emitter, payload_value, payload_type);
        if (payload_value.value == NULL)
        {
            return __LLVM_Invalid_Value__();
        }
        payload_llvm_type = __LLVM_Type__(emitter, payload_type);
        if (payload_llvm_type == NULL)
        {
            return __LLVM_Invalid_Value__();
        }
        payload_temp = __LLVM_Allocate_Stack__(emitter, payload_llvm_type, "tagged.payload.temp");
        LLVMBuildStore(emitter->builder, payload_value.value, payload_temp);
        destination = __LLVM_Tagged_Byte_Address__(emitter,
                                                   target_type,
                                                   llvm_type,
                                                   storage,
                                                   canonical_offset,
                                                   payload_size,
                                                   "tagged.payload.place");
        if (destination == NULL)
        {
            return __LLVM_Invalid_Value__();
        }
        byte_count = LLVMConstInt(
            LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)payload_size, 0);
        if (LLVMBuildMemCpy(emitter->builder,
                            destination,
                            (unsigned)payload_alignment,
                            payload_temp,
                            (unsigned)payload_alignment,
                            byte_count) == NULL)
        {
            __LLVM_Fail__("L2.5 payload copy into tagged storage failed");
            return __LLVM_Invalid_Value__();
        }
    }
    (void)payload_capacity;
    result.value = LLVMBuildLoad2(emitter->builder, llvm_type, storage, "tagged.value");
    result.type = target_type;
    return result;
}

/* Returns the LLVM declare runtime function. */
static LLVMValueRef __LLVM_Declare_Runtime_Function__(__LLVM_Emitter__ *emitter,
                                                      const char *name,
                                                      LLVMTypeRef return_type,
                                                      LLVMTypeRef *parameters,
                                                      unsigned parameter_count,
                                                      LLVMTypeRef *out_function_type)
{
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    function_type = LLVMFunctionType(return_type, parameters, parameter_count, 0);
    function = LLVMGetNamedFunction(emitter->module, name);
    if (function == NULL)
        function = LLVMAddFunction(emitter->module, name, function_type);
    if (function == NULL)
    {
        __LLVM_Fail__("L2.8 could not declare native runtime function");
        return NULL;
    }
    if (out_function_type != NULL)
        *out_function_type = function_type;
    return function;
}

/* Returns the LLVM runtime malloc. */
static LLVMValueRef
__LLVM_Runtime_Malloc__(__LLVM_Emitter__ *emitter, LLVMValueRef byte_count, const char *name)
{
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    if (byte_count == NULL)
        return NULL;
    if (emitter->malloc_function == NULL)
    {
        /* Stores the parameters. */
        LLVMTypeRef parameters[1] = {LLVMIntTypeInContext(emitter->context, 64U)};
        emitter->malloc_type = LLVMFunctionType(i8_pointer, parameters, 1U, 0);
        emitter->malloc_function = LLVMGetNamedFunction(emitter->module, "malloc");
        if (emitter->malloc_function == NULL)
            emitter->malloc_function =
                LLVMAddFunction(emitter->module, "malloc", emitter->malloc_type);
        if (emitter->malloc_function == NULL)
        {
            __LLVM_Fail__("L2.8 could not declare native runtime allocation");
            return NULL;
        }
    }
    arguments[0] = byte_count;
    return LLVMBuildCall2(
        emitter->builder, emitter->malloc_type, emitter->malloc_function, arguments, 1U, name);
}

/* Returns the LLVM runtime c string. */
static LLVMValueRef
__LLVM_Runtime_C_String__(__LLVM_Emitter__ *emitter, LLVMValueRef text, const char *name)
{
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the allocation size. */
    LLVMValueRef allocation_size;
    /* Stores the buffer. */
    LLVMValueRef buffer;
    /* Tracks the failed state. */
    LLVMValueRef failed;
    /* Stores the end. */
    LLVMValueRef end;
    /* Stores the indices. */
    LLVMValueRef indices[1];
    if (text == NULL)
        return NULL;
    data = LLVMBuildExtractValue(emitter->builder, text, 0U, "runtime.path.data");
    length = LLVMBuildExtractValue(emitter->builder, text, 1U, "runtime.path.length");
    allocation_size =
        LLVMBuildAdd(emitter->builder, length, LLVMConstInt(i64, 1U, 0), "runtime.path.size");
    buffer = __LLVM_Runtime_Malloc__(emitter, allocation_size, name);
    if (buffer == NULL)
        return NULL;
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           buffer,
                           LLVMConstNull(i8_pointer),
                           "runtime.path.allocate.failed");
    if (!__LLVM_Emit_Trap_If__(emitter, failed, "runtime.path.allocate.failure"))
        return NULL;
    if (LLVMBuildMemCpy(emitter->builder, buffer, 1U, data, 1U, length) == NULL)
        return NULL;
    indices[0] = length;
    end = LLVMBuildGEP2(emitter->builder, i8, buffer, indices, 1U, "runtime.path.end");
    if (end == NULL)
        return NULL;
    LLVMBuildStore(emitter->builder, LLVMConstInt(i8, 0U, 0), end);
    return buffer;
}

/* Returns the LLVM runtime literal c string. */
static LLVMValueRef
__LLVM_Runtime_Literal_C_String__(__LLVM_Emitter__ *emitter, const char *literal, const char *name)
{
    /* Stores the length. */
    size_t length;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8;
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer;
    /* Stores the array type. */
    LLVMTypeRef array_type;
    /* Stores the storage. */
    LLVMValueRef storage;
    /* Stores the constant. */
    LLVMValueRef constant;
    if (literal == NULL)
        return NULL;
    length = strlen(literal);
    if (length > UINT_MAX - 1U)
        return NULL;
    i8 = LLVMIntTypeInContext(emitter->context, 8U);
    i8_pointer = LLVMPointerType(i8, 0U);
    array_type = LLVMArrayType(i8, (unsigned)length + 1U);
    storage = __LLVM_Allocate_Stack__(emitter, array_type, name);
    if (storage == NULL)
        return NULL;
    constant = LLVMConstStringInContext(emitter->context, literal, (unsigned)length, 0);
    if (constant == NULL)
        return NULL;
    LLVMBuildStore(emitter->builder, constant, storage);
    return LLVMBuildPointerCast(emitter->builder, storage, i8_pointer, name);
}

/* Releases the LLVM runtime. */
static int __LLVM_Runtime_Free__(__LLVM_Emitter__ *emitter, LLVMValueRef pointer)
{
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[1] = {i8_pointer};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    function = __LLVM_Declare_Runtime_Function__(
        emitter, "free", LLVMVoidTypeInContext(emitter->context), parameters, 1U, &function_type);
    if (function == NULL)
        return 0;
    arguments[0] = pointer;
    return LLVMBuildCall2(emitter->builder, function_type, function, arguments, 1U, "") != NULL;
}

/* Returns the LLVM make text value. */
static __LLVM_Value__ __LLVM_Make_Text_Value__(__LLVM_Emitter__ *emitter,
                                               LLVMValueRef data,
                                               LLVMValueRef length,
                                               LLVMValueRef capacity,
                                               __Ast_Type__ *target_type)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* References the type. */
    __Ast_Type__ *type = target_type != NULL ? target_type : &__LLVM_String_Type__;
    /* Stores the LLVM type. */
    LLVMTypeRef llvm_type = __LLVM_Type__(emitter, type);
    /* Stores the value. */
    LLVMValueRef value;
    if (llvm_type == NULL || data == NULL || length == NULL || capacity == NULL)
        return result;
    value = LLVMConstNull(llvm_type);
    value = LLVMBuildInsertValue(emitter->builder, value, data, 0U, "runtime.text.data");
    value = LLVMBuildInsertValue(emitter->builder, value, length, 1U, "runtime.text.length");
    value = LLVMBuildInsertValue(emitter->builder, value, capacity, 2U, "runtime.text.capacity");
    result.value = value;
    result.type = type;
    return result;
}

/* Returns the LLVM store tagged payload value. */
static int __LLVM_Store_Tagged_Payload_Value__(__LLVM_Emitter__ *emitter,
                                               __Ast_Type__ *tagged_type,
                                               LLVMTypeRef tagged_llvm_type,
                                               LLVMValueRef tagged_storage,
                                               size_t constructor_index,
                                               size_t payload_index,
                                               __LLVM_Value__ payload)
{
    /* Stores the canonical offset. */
    size_t canonical_offset = 0U;
    /* Stores the payload size. */
    size_t payload_size = 0U;
    /* Stores the payload alignment. */
    size_t payload_alignment = 1U;
    /* References the payload type. */
    __Ast_Type__ *payload_type = NULL;
    /* Stores the payload LLVM type. */
    LLVMTypeRef payload_llvm_type;
    /* Stores the payload temp. */
    LLVMValueRef payload_temp;
    /* Stores the destination. */
    LLVMValueRef destination;
    /* Stores the byte count. */
    LLVMValueRef byte_count;

    if (!__Layout_Tagged_Payload__(emitter->semantic,
                                   tagged_type,
                                   constructor_index,
                                   payload_index,
                                   &canonical_offset,
                                   &payload_type) ||
        payload_type == NULL ||
        !__Layout_Type__(emitter->semantic, payload_type, &payload_size, &payload_alignment))
        return __LLVM_Fail__("L2.8 runtime Result payload lacks canonical tagged layout");
    payload = __LLVM_Coerce__(emitter, payload, payload_type);
    if (payload.value == NULL)
        return 0;
    payload_llvm_type = __LLVM_Type__(emitter, payload_type);
    if (payload_llvm_type == NULL)
        return 0;
    payload_temp =
        __LLVM_Allocate_Stack__(emitter, payload_llvm_type, "runtime.result.payload.temp");
    LLVMBuildStore(emitter->builder, payload.value, payload_temp);
    destination = __LLVM_Tagged_Byte_Address__(emitter,
                                               tagged_type,
                                               tagged_llvm_type,
                                               tagged_storage,
                                               canonical_offset,
                                               payload_size,
                                               "runtime.result.payload.place");
    if (destination == NULL)
        return 0;
    byte_count = LLVMConstInt(
        LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)payload_size, 0);
    if (LLVMBuildMemCpy(emitter->builder,
                        destination,
                        (unsigned)payload_alignment,
                        payload_temp,
                        (unsigned)payload_alignment,
                        byte_count) == NULL)
        return __LLVM_Fail__("L2.8 runtime Result payload copy failed");
    return 1;
}

/* Writes the LLVM tagged arm. */
static int __LLVM_Write_Tagged_Arm__(__LLVM_Emitter__ *emitter,
                                     __Ast_Type__ *tagged_type,
                                     LLVMTypeRef tagged_llvm_type,
                                     LLVMValueRef tagged_storage,
                                     size_t constructor_index,
                                     __LLVM_Value__ payload)
{
    /* Stores the tag address. */
    LLVMValueRef tag_address = LLVMBuildStructGEP2(
        emitter->builder, tagged_llvm_type, tagged_storage, 0U, "runtime.result.tag.place");
    if (tag_address == NULL)
        return __LLVM_Fail__("L2.8 runtime Result tag place failed");
    LLVMBuildStore(emitter->builder,
                   LLVMConstInt(LLVMIntTypeInContext(emitter->context, 64U),
                                (unsigned long long)constructor_index,
                                0),
                   tag_address);
    return __LLVM_Store_Tagged_Payload_Value__(
        emitter, tagged_type, tagged_llvm_type, tagged_storage, constructor_index, 0U, payload);
}

/* Builds the LLVM runtime result. */
static __LLVM_Value__ __LLVM_Build_Runtime_Result__(__LLVM_Emitter__ *emitter,
                                                    __Ast_Type__ *result_type,
                                                    LLVMValueRef status,
                                                    __LLVM_Value__ success_payload)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    /* Stores the LLVM type. */
    LLVMTypeRef llvm_type;
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the storage. */
    LLVMValueRef storage;
    /* Tracks whether the value is error. */
    LLVMValueRef is_error;
    /* Stores the function. */
    LLVMValueRef function;
    /* Tracks the success block state. */
    LLVMBasicBlockRef success_block;
    /* Stores the error block. */
    LLVMBasicBlockRef error_block;
    /* Stores the merge block. */
    LLVMBasicBlockRef merge_block;
    /* Stores the error payload. */
    __LLVM_Value__ error_payload;

    if (result_type == NULL || status == NULL ||
        !__Type_Resolve__(emitter->semantic, result_type, &resolved) ||
        resolved.__Kind__ != __Resolved_Type_Result__)
    {
        __LLVM_Fail__("L2.8 native runtime primitive requires canonical Result contextual Type");
        return result;
    }
    llvm_type = __LLVM_Type__(emitter, result_type);
    if (llvm_type == NULL)
        return result;
    storage = __LLVM_Allocate_Stack__(emitter, llvm_type, "runtime.result.storage");
    LLVMBuildStore(emitter->builder, LLVMConstNull(llvm_type), storage);
    is_error = LLVMBuildICmp(
        emitter->builder, LLVMIntSLT, status, LLVMConstInt(i64, 0U, 0), "runtime.result.is.error");
    function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    if (function == NULL)
    {
        __LLVM_Fail__("L2.8 runtime Result lost function CFG");
        return result;
    }
    success_block = LLVMAppendBasicBlockInContext(emitter->context, function, "runtime.result.ok");
    error_block = LLVMAppendBasicBlockInContext(emitter->context, function, "runtime.result.err");
    merge_block = LLVMAppendBasicBlockInContext(emitter->context, function, "runtime.result.merge");
    LLVMBuildCondBr(emitter->builder, is_error, error_block, success_block);

    LLVMPositionBuilderAtEnd(emitter->builder, success_block);
    if (!__LLVM_Write_Tagged_Arm__(emitter, result_type, llvm_type, storage, 0U, success_payload))
        return __LLVM_Invalid_Value__();
    LLVMBuildBr(emitter->builder, merge_block);

    LLVMPositionBuilderAtEnd(emitter->builder, error_block);
    error_payload.value = status;
    error_payload.type = &__LLVM_Integer_Type__;
    if (!__LLVM_Write_Tagged_Arm__(emitter, result_type, llvm_type, storage, 1U, error_payload))
        return __LLVM_Invalid_Value__();
    LLVMBuildBr(emitter->builder, merge_block);

    LLVMPositionBuilderAtEnd(emitter->builder, merge_block);
    result.value = LLVMBuildLoad2(emitter->builder, llvm_type, storage, "runtime.result.value");
    result.type = result_type;
    return result;
}

/* Emits the LLVM runtime open file service. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Open_File_Service__(__LLVM_Emitter__ *emitter,
                                                              __Ast_Expression__ *expression,
                                                              __Ast_Type__ *expected,
                                                              int create_for_write)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the path. */
    __LLVM_Value__ path;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[3] = {i8_pointer, i32, i32};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the C path string. */
    LLVMValueRef c_path;
    /* Stores the call arguments. */
    LLVMValueRef arguments[3];
    /* Stores the descriptor 32. */
    LLVMValueRef descriptor32;
    /* Stores the descriptor 64. */
    LLVMValueRef descriptor64;
    /* Tracks the failed state. */
    LLVMValueRef failed;
#if defined(__APPLE__)
    const unsigned write_flags = 1U | 512U | 1024U;
#else
    /* Stores the write flags. */
    const unsigned write_flags = 1U | 64U | 512U;
#endif

    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("low-level open service disagrees with canonical builtin arity");
        return result;
    }
    path = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_String_Type__);
    if (path.value == NULL)
        return result;
    c_path = __LLVM_Runtime_C_String__(emitter, path.value, "runtime.service.path");
    if (c_path == NULL)
        return result;
    function =
        __LLVM_Declare_Runtime_Function__(emitter, "open", i32, parameters, 3U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] = c_path;
    arguments[1] = LLVMConstInt(i32, create_for_write ? write_flags : 0U, 0);
    arguments[2] = LLVMConstInt(i32, 0666U, 0);
    descriptor32 = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 3U, "runtime.service.open");
    if (!__LLVM_Runtime_Free__(emitter, c_path))
        return result;
    descriptor64 =
        LLVMBuildSExt(emitter->builder, descriptor32, i64, "runtime.service.open.descriptor");
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           descriptor32,
                           LLVMConstInt(i32, 0U, 0),
                           "runtime.service.open.failed");
    result.value = LLVMBuildSelect(emitter->builder,
                                   failed,
                                   LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                   descriptor64,
                                   "runtime.service.open.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime path metadata service. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Path_Metadata_Service__(
    __LLVM_Emitter__ *emitter,
    __Ast_Expression__ *expression,
    __Ast_Type__ *expected,
    __Name_Builtin_Function__ builtin)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the path. */
    __LLVM_Value__ path;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the stat buffer type. */
    LLVMTypeRef stat_type = LLVMArrayType(i8, 144U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[2] = {i8_pointer, i8_pointer};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the C path string. */
    LLVMValueRef c_path;
    /* Stores the stat buffer. */
    LLVMValueRef stat_buffer;
    /* Stores the stat bytes pointer. */
    LLVMValueRef stat_bytes;
    /* Stores the call arguments. */
    LLVMValueRef arguments[2];
    /* Stores the call status. */
    LLVMValueRef status;
    /* Tracks a failed call. */
    LLVMValueRef failed;
    /* Stores the service value. */
    LLVMValueRef value = NULL;
#if defined(__APPLE__)
    const unsigned mode_offset = 4U;
    const unsigned size_offset = 96U;
    const unsigned mtime_seconds_offset = 48U;
    const unsigned mtime_nanoseconds_offset = 56U;
#else
    const unsigned mode_offset = 24U;
    const unsigned size_offset = 48U;
    const unsigned mtime_seconds_offset = 88U;
    const unsigned mtime_nanoseconds_offset = 96U;
#endif

    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("low-level path metadata service disagrees with canonical builtin arity");
        return result;
    }
    path = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_String_Type__);
    if (path.value == NULL)
        return result;
    c_path = __LLVM_Runtime_C_String__(emitter, path.value, "runtime.metadata.path");
    if (c_path == NULL)
        return result;

    stat_buffer = __LLVM_Allocate_Stack__(emitter, stat_type, "runtime.metadata.stat");
    if (stat_buffer == NULL)
        return result;
    LLVMBuildStore(emitter->builder, LLVMConstNull(stat_type), stat_buffer);
    stat_bytes = LLVMBuildPointerCast(
        emitter->builder, stat_buffer, i8_pointer, "runtime.metadata.stat.bytes");
    function = __LLVM_Declare_Runtime_Function__(
        emitter, "lstat", i32, parameters, 2U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] = c_path;
    arguments[1] = stat_bytes;
    status = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 2U, "runtime.metadata.status");
    if (!__LLVM_Runtime_Free__(emitter, c_path))
        return result;
    failed = LLVMBuildICmp(
        emitter->builder, LLVMIntSLT, status, LLVMConstInt(i32, 0U, 0), "runtime.metadata.failed");

    if (builtin == __Name_Builtin_Path_Type__)
    {
        /* Stores the mode field pointer. */
        LLVMValueRef mode_index = LLVMConstInt(i64, mode_offset, 0);
        LLVMValueRef mode_bytes = LLVMBuildGEP2(
            emitter->builder, i8, stat_bytes, &mode_index, 1U, "runtime.metadata.mode.bytes");
#if defined(__APPLE__)
        LLVMTypeRef i16 = LLVMIntTypeInContext(emitter->context, 16U);
        LLVMValueRef mode_pointer = LLVMBuildPointerCast(
            emitter->builder, mode_bytes, LLVMPointerType(i16, 0U), "runtime.metadata.mode.ptr");
        LLVMValueRef mode = LLVMBuildZExt(
            emitter->builder,
            LLVMBuildLoad2(emitter->builder, i16, mode_pointer, "runtime.metadata.mode16"),
            i64,
            "runtime.metadata.mode");
#else
        LLVMValueRef mode_pointer = LLVMBuildPointerCast(
            emitter->builder, mode_bytes, LLVMPointerType(i32, 0U), "runtime.metadata.mode.ptr");
        LLVMValueRef mode = LLVMBuildZExt(
            emitter->builder,
            LLVMBuildLoad2(emitter->builder, i32, mode_pointer, "runtime.metadata.mode32"),
            i64,
            "runtime.metadata.mode");
#endif
        LLVMValueRef kind = LLVMBuildAnd(
            emitter->builder, mode, LLVMConstInt(i64, 0xF000U, 0), "runtime.metadata.kind");
        LLVMValueRef regular = LLVMBuildICmp(
            emitter->builder, LLVMIntEQ, kind, LLVMConstInt(i64, 0x8000U, 0), "runtime.metadata.regular");
        LLVMValueRef directory = LLVMBuildICmp(
            emitter->builder, LLVMIntEQ, kind, LLVMConstInt(i64, 0x4000U, 0), "runtime.metadata.directory");
        LLVMValueRef symlink = LLVMBuildICmp(
            emitter->builder, LLVMIntEQ, kind, LLVMConstInt(i64, 0xA000U, 0), "runtime.metadata.symlink");
        value = LLVMConstInt(i64, 4U, 0);
        value = LLVMBuildSelect(
            emitter->builder, symlink, LLVMConstInt(i64, 3U, 0), value, "runtime.metadata.kind.symlink");
        value = LLVMBuildSelect(
            emitter->builder, directory, LLVMConstInt(i64, 2U, 0), value, "runtime.metadata.kind.directory");
        value = LLVMBuildSelect(
            emitter->builder, regular, LLVMConstInt(i64, 1U, 0), value, "runtime.metadata.kind.regular");
    }
    else if (builtin == __Name_Builtin_Path_Size__)
    {
        /* Stores the size field pointer. */
        LLVMValueRef size_index = LLVMConstInt(i64, size_offset, 0);
        LLVMValueRef size_bytes = LLVMBuildGEP2(
            emitter->builder, i8, stat_bytes, &size_index, 1U, "runtime.metadata.size.bytes");
        LLVMValueRef size_pointer = LLVMBuildPointerCast(
            emitter->builder, size_bytes, LLVMPointerType(i64, 0U), "runtime.metadata.size.ptr");
        value = LLVMBuildLoad2(emitter->builder, i64, size_pointer, "runtime.metadata.size");
    }
    else if (builtin == __Name_Builtin_Path_Modified_Time__)
    {
        /* Stores the modified-time field pointers. */
        LLVMValueRef seconds_index = LLVMConstInt(i64, mtime_seconds_offset, 0);
        LLVMValueRef nanoseconds_index = LLVMConstInt(i64, mtime_nanoseconds_offset, 0);
        LLVMValueRef seconds_bytes = LLVMBuildGEP2(
            emitter->builder, i8, stat_bytes, &seconds_index, 1U, "runtime.metadata.mtime.sec.bytes");
        LLVMValueRef nanoseconds_bytes = LLVMBuildGEP2(
            emitter->builder, i8, stat_bytes, &nanoseconds_index, 1U, "runtime.metadata.mtime.nsec.bytes");
        LLVMValueRef seconds_pointer = LLVMBuildPointerCast(
            emitter->builder, seconds_bytes, LLVMPointerType(i64, 0U), "runtime.metadata.mtime.sec.ptr");
        LLVMValueRef nanoseconds_pointer = LLVMBuildPointerCast(
            emitter->builder, nanoseconds_bytes, LLVMPointerType(i64, 0U), "runtime.metadata.mtime.nsec.ptr");
        LLVMValueRef seconds = LLVMBuildLoad2(
            emitter->builder, i64, seconds_pointer, "runtime.metadata.mtime.sec");
        LLVMValueRef nanoseconds = LLVMBuildLoad2(
            emitter->builder, i64, nanoseconds_pointer, "runtime.metadata.mtime.nsec");
        value = LLVMBuildAdd(
            emitter->builder,
            LLVMBuildMul(
                emitter->builder,
                seconds,
                LLVMConstInt(i64, 1000000000ULL, 0),
                "runtime.metadata.mtime.seconds.ns"),
            nanoseconds,
            "runtime.metadata.mtime.ns");
    }
    else
    {
        __LLVM_Fail__("unknown Bootstrap path metadata builtin");
        return result;
    }

    result.value = LLVMBuildSelect(
        emitter->builder,
        failed,
        LLVMConstInt(i64, (unsigned long long)-2LL, 1),
        value,
        "runtime.metadata.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime open directory service. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Open_Directory_Service__(__LLVM_Emitter__ *emitter,
                                                                   __Ast_Expression__ *expression,
                                                                   __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the path. */
    __LLVM_Value__ path;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[1] = {i8_pointer};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the C path string. */
    LLVMValueRef c_path;
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    /* Stores the directory. */
    LLVMValueRef directory;
    /* Tracks the failed state. */
    LLVMValueRef failed;
    /* Stores the handle. */
    LLVMValueRef handle;

    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("low-level directory-open service disagrees with canonical builtin arity");
        return result;
    }
    path = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_String_Type__);
    if (path.value == NULL)
        return result;
    c_path = __LLVM_Runtime_C_String__(emitter, path.value, "runtime.directory.path");
    if (c_path == NULL)
        return result;
    function = __LLVM_Declare_Runtime_Function__(
        emitter, "opendir", i8_pointer, parameters, 1U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] = c_path;
    directory = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 1U, "runtime.directory.open");
    if (!__LLVM_Runtime_Free__(emitter, c_path))
        return result;
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           directory,
                           LLVMConstNull(i8_pointer),
                           "runtime.directory.open.failed");
    handle = LLVMBuildPtrToInt(emitter->builder, directory, i64, "runtime.directory.handle");
    result.value = LLVMBuildSelect(emitter->builder,
                                   failed,
                                   LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                   handle,
                                   "runtime.directory.open.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime read directory entry service. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Read_Directory_Entry_Service__(
    __LLVM_Emitter__ *emitter,
    __Ast_Expression__ *expression,
    __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the handle. */
    __LLVM_Value__ handle;
    /* Stores the bytes. */
    __LLVM_Value__ bytes;
    /* Stores the offset. */
    __LLVM_Value__ offset;
    /* Stores the requested. */
    __LLVM_Value__ requested;
    /* References the bytes type. */
    __Ast_Type__ *bytes_type;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the readdir parameters. */
    LLVMTypeRef readdir_parameters[1] = {i8_pointer};
    /* Stores the readdir type. */
    LLVMTypeRef readdir_type;
    /* Stores the readdir function. */
    LLVMValueRef readdir_function;
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the invalid. */
    LLVMValueRef invalid;
    /* Stores the handle invalid. */
    LLVMValueRef handle_invalid;
    /* Stores the negative offset. */
    LLVMValueRef negative_offset;
    /* Stores the negative count. */
    LLVMValueRef negative_count;
    /* Stores the offset too large. */
    LLVMValueRef offset_too_large;
    /* Stores the remaining. */
    LLVMValueRef remaining;
    /* Stores the count too large. */
    LLVMValueRef count_too_large;
    /* Stores the status slot. */
    LLVMValueRef status_slot;
    /* Stores the current function. */
    LLVMValueRef current_function;
    /* Stores the invalid block. */
    LLVMBasicBlockRef invalid_block;
    /* Stores the read block. */
    LLVMBasicBlockRef read_block;
    /* Stores the EOF block. */
    LLVMBasicBlockRef eof_block;
    /* Stores the entry block. */
    LLVMBasicBlockRef entry_block;
    /* Stores the copy block. */
    LLVMBasicBlockRef copy_block;
    /* Stores the name error block. */
    LLVMBasicBlockRef name_error_block;
    /* Tracks the done block state. */
    LLVMBasicBlockRef done_block;
    /* Stores the directory pointer. */
    LLVMValueRef directory_pointer;
    /* Stores the readdir arguments. */
    LLVMValueRef readdir_args[1];
    /* Stores the entry. */
    LLVMValueRef entry;
    /* Tracks whether the value is EOF. */
    LLVMValueRef is_eof;
    /* Stores the name pointer. */
    LLVMValueRef name_pointer;
    /* Stores the name length. */
    LLVMValueRef name_length;
    /* Stores the name too long. */
    LLVMValueRef name_too_long;
    /* Stores the name encoding too long. */
    LLVMValueRef name_encoding_too_long;
    /* Stores the invalid name. */
    LLVMValueRef invalid_name;
    /* Stores the destination. */
    LLVMValueRef destination;
    /* Stores the type pointer. */
    LLVMValueRef type_pointer;
    /* Stores the raw type. */
    LLVMValueRef raw_type;
    /* Stores the kind. */
    LLVMValueRef kind;
    /* Stores the encoded kind. */
    LLVMValueRef encoded_kind;
    /* Stores the encoded result. */
    LLVMValueRef encoded_result;
#if defined(__APPLE__)
    const unsigned directory_name_offset = 21U;
    const unsigned directory_type_offset = 20U;
#else
    /* Stores the directory name offset. */
    const unsigned directory_name_offset = 19U;
    /* Stores the directory type offset. */
    const unsigned directory_type_offset = 18U;
#endif

    if (expression->__As__.__Call__.__Argument_Count__ != 4U)
    {
        __LLVM_Fail__("low-level directory-read service disagrees with canonical builtin arity");
        return result;
    }

    handle = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
    bytes_type = __LLVM_Expression_Type__(emitter, expression->__As__.__Call__.__Arguments__[1]);
    if (bytes_type == NULL)
        return result;
    bytes = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[1], bytes_type);
    offset = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[2], &__LLVM_Integer_Type__);
    requested = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[3], &__LLVM_Integer_Type__);
    if (handle.value == NULL || bytes.value == NULL || offset.value == NULL || requested.value == NULL)
        return result;
    handle = __LLVM_Coerce__(emitter, handle, &__LLVM_Integer_Type__);
    offset = __LLVM_Coerce__(emitter, offset, &__LLVM_Integer_Type__);
    requested = __LLVM_Coerce__(emitter, requested, &__LLVM_Integer_Type__);
    if (handle.value == NULL || offset.value == NULL || requested.value == NULL)
        return result;

    data = LLVMBuildPointerCast(
        emitter->builder,
        LLVMBuildExtractValue(emitter->builder, bytes.value, 0U, "runtime.directory.bytes.data"),
        i8_pointer,
        "runtime.directory.bytes");
    length = LLVMBuildExtractValue(
        emitter->builder, bytes.value, 1U, "runtime.directory.bytes.length");
    handle_invalid = LLVMBuildICmp(emitter->builder,
                                   LLVMIntSLE,
                                   handle.value,
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.directory.handle.invalid");
    negative_offset = LLVMBuildICmp(emitter->builder,
                                    LLVMIntSLT,
                                    offset.value,
                                    LLVMConstInt(i64, 0U, 0),
                                    "runtime.directory.offset.negative");
    negative_count = LLVMBuildICmp(emitter->builder,
                                   LLVMIntSLT,
                                   requested.value,
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.directory.count.negative");
    offset_too_large = LLVMBuildICmp(emitter->builder,
                                     LLVMIntUGT,
                                     offset.value,
                                     length,
                                     "runtime.directory.offset.large");
    remaining = LLVMBuildSub(emitter->builder, length, offset.value, "runtime.directory.remaining");
    count_too_large = LLVMBuildICmp(emitter->builder,
                                    LLVMIntUGT,
                                    requested.value,
                                    remaining,
                                    "runtime.directory.count.large");
    invalid = LLVMBuildOr(emitter->builder,
                          handle_invalid,
                          negative_offset,
                          "runtime.directory.invalid.handle.offset");
    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          negative_count,
                          "runtime.directory.invalid.count.sign");
    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          offset_too_large,
                          "runtime.directory.invalid.offset");
    invalid = LLVMBuildOr(emitter->builder,
                          invalid,
                          count_too_large,
                          "runtime.directory.invalid.count");

    status_slot = __LLVM_Allocate_Stack__(emitter, i64, "runtime.directory.status");
    if (status_slot == NULL)
        return result;
    current_function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    if (current_function == NULL)
        return result;
    invalid_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.invalid");
    read_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.read");
    eof_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.eof");
    entry_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.entry");
    copy_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.copy");
    name_error_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.name.error");
    done_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.directory.done");
    LLVMBuildCondBr(emitter->builder, invalid, invalid_block, read_block);

    LLVMPositionBuilderAtEnd(emitter->builder, invalid_block);
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, (unsigned long long)-2LL, 1), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, read_block);
    readdir_function = __LLVM_Declare_Runtime_Function__(
        emitter, "readdir", i8_pointer, readdir_parameters, 1U, &readdir_type);
    if (readdir_function == NULL)
        return result;
    directory_pointer = LLVMBuildIntToPtr(
        emitter->builder, handle.value, i8_pointer, "runtime.directory.pointer");
    readdir_args[0] = directory_pointer;
    entry = LLVMBuildCall2(
        emitter->builder, readdir_type, readdir_function, readdir_args, 1U, "runtime.directory.entry.ptr");
    is_eof = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           entry,
                           LLVMConstNull(i8_pointer),
                           "runtime.directory.entry.eof");
    LLVMBuildCondBr(emitter->builder, is_eof, eof_block, entry_block);

    LLVMPositionBuilderAtEnd(emitter->builder, eof_block);
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, 0U, 0), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, entry_block);
    name_pointer = LLVMBuildGEP2(emitter->builder,
                                 i8,
                                 entry,
                                 &(LLVMValueRef){LLVMConstInt(i64, directory_name_offset, 0)},
                                 1U,
                                 "runtime.directory.entry.name");
    name_length = __LLVM_Strlen__(emitter, name_pointer);
    if (name_length == NULL)
        return result;
    name_too_long = LLVMBuildICmp(emitter->builder,
                                  LLVMIntUGT,
                                  name_length,
                                  requested.value,
                                  "runtime.directory.name.too.long");
    name_encoding_too_long = LLVMBuildICmp(emitter->builder,
                                           LLVMIntUGE,
                                           name_length,
                                           LLVMConstInt(i64, 4096U, 0),
                                           "runtime.directory.name.encoding.too.long");
    invalid_name = LLVMBuildOr(emitter->builder,
                               name_too_long,
                               name_encoding_too_long,
                               "runtime.directory.name.invalid");
    LLVMBuildCondBr(emitter->builder, invalid_name, name_error_block, copy_block);

    LLVMPositionBuilderAtEnd(emitter->builder, name_error_block);
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, (unsigned long long)-2LL, 1), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, copy_block);
    destination = LLVMBuildGEP2(
        emitter->builder, i8, data, &offset.value, 1U, "runtime.directory.destination");
    if (LLVMBuildMemCpy(emitter->builder, destination, 1U, name_pointer, 1U, name_length) == NULL)
        return result;
    {
        /* Tracks the type index. */
        LLVMValueRef type_index = LLVMConstInt(i64, directory_type_offset, 0);
        /* Tracks whether the value is regular. */
        LLVMValueRef is_regular;
        /* Tracks whether the value is directory. */
        LLVMValueRef is_directory;
        /* Tracks whether the value is symlink. */
        LLVMValueRef is_symlink;
        /* Stores the one. */
        LLVMValueRef one = LLVMConstInt(i64, 1U, 0);
        /* Stores the two. */
        LLVMValueRef two = LLVMConstInt(i64, 2U, 0);
        /* Stores the three. */
        LLVMValueRef three = LLVMConstInt(i64, 3U, 0);
        /* Stores the zero. */
        LLVMValueRef zero = LLVMConstInt(i64, 0U, 0);
        type_pointer = LLVMBuildGEP2(
            emitter->builder, i8, entry, &type_index, 1U, "runtime.directory.entry.type.ptr");
        raw_type = LLVMBuildLoad2(emitter->builder, i8, type_pointer, "runtime.directory.entry.type");
        is_regular = LLVMBuildICmp(emitter->builder,
                                   LLVMIntEQ,
                                   raw_type,
                                   LLVMConstInt(i8, 8U, 0),
                                   "runtime.directory.type.regular");
        is_directory = LLVMBuildICmp(emitter->builder,
                                     LLVMIntEQ,
                                     raw_type,
                                     LLVMConstInt(i8, 4U, 0),
                                     "runtime.directory.type.directory");
        is_symlink = LLVMBuildICmp(emitter->builder,
                                   LLVMIntEQ,
                                   raw_type,
                                   LLVMConstInt(i8, 10U, 0),
                                   "runtime.directory.type.symlink");
        kind = LLVMBuildSelect(emitter->builder, is_symlink, three, zero, "runtime.directory.kind.symlink");
        kind = LLVMBuildSelect(emitter->builder, is_directory, two, kind, "runtime.directory.kind.directory");
        kind = LLVMBuildSelect(emitter->builder, is_regular, one, kind, "runtime.directory.kind.regular");
    }
    encoded_kind = LLVMBuildShl(
        emitter->builder, kind, LLVMConstInt(i64, 12U, 0), "runtime.directory.kind.encoded");
    encoded_result = LLVMBuildAdd(
        emitter->builder, encoded_kind, name_length, "runtime.directory.entry.result");
    LLVMBuildStore(emitter->builder, encoded_result, status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    result.value = LLVMBuildLoad2(
        emitter->builder, i64, status_slot, "runtime.directory.status.value");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime close directory service. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Close_Directory_Service__(__LLVM_Emitter__ *emitter,
                                                                    __Ast_Expression__ *expression,
                                                                    __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the handle. */
    __LLVM_Value__ handle;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[1] = {i8_pointer};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    /* Stores the status. */
    LLVMValueRef status;
    /* Tracks the failed state. */
    LLVMValueRef failed;

    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("low-level directory-close service disagrees with canonical builtin arity");
        return result;
    }
    handle = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
    if (handle.value == NULL)
        return result;
    handle = __LLVM_Coerce__(emitter, handle, &__LLVM_Integer_Type__);
    if (handle.value == NULL)
        return result;
    function = __LLVM_Declare_Runtime_Function__(
        emitter, "closedir", i32, parameters, 1U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] = LLVMBuildIntToPtr(
        emitter->builder, handle.value, i8_pointer, "runtime.directory.close.pointer");
    status = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 1U, "runtime.directory.close.status");
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           status,
                           LLVMConstInt(i32, 0U, 0),
                           "runtime.directory.close.failed");
    result.value = LLVMBuildSelect(emitter->builder,
                                   failed,
                                   LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.directory.close.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime read byte service. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Read_Byte_Service__(__LLVM_Emitter__ *emitter,
                                                              __Ast_Expression__ *expression,
                                                              __Ast_Type__ *expected,
                                                              int stdin_mode)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the descriptor. */
    __LLVM_Value__ descriptor;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[3] = {i32, i8_pointer, i64};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the byte slot. */
    LLVMValueRef byte_slot;
    /* Stores the descriptor 32. */
    LLVMValueRef descriptor32;
    /* Stores the call arguments. */
    LLVMValueRef arguments[3];
    /* Stores the count. */
    LLVMValueRef count;
    /* Stores the byte. */
    LLVMValueRef byte;
    /* Stores the byte 64. */
    LLVMValueRef byte64;
    /* Tracks whether the value is one. */
    LLVMValueRef is_one;
    /* Tracks whether the value is EOF. */
    LLVMValueRef is_eof;
    /* Tracks the non success state. */
    LLVMValueRef non_success;

    if (expression->__As__.__Call__.__Argument_Count__ != (stdin_mode ? 0U : 1U))
    {
        __LLVM_Fail__("low-level read-byte service disagrees with canonical builtin arity");
        return result;
    }
    if (stdin_mode)
    {
        descriptor32 = LLVMConstInt(i32, 0U, 0);
    }
    else
    {
        descriptor = __LLVM_Emit_Expression__(
            emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
        if (descriptor.value == NULL)
            return result;
        descriptor = __LLVM_Coerce__(emitter, descriptor, &__LLVM_Integer_Type__);
        if (descriptor.value == NULL)
            return result;
        descriptor32 =
            LLVMBuildTrunc(emitter->builder, descriptor.value, i32, "runtime.service.read.fd");
    }
    byte_slot = __LLVM_Allocate_Stack__(emitter, i8, "runtime.service.read.byte");
    if (byte_slot == NULL)
        return result;
    function =
        __LLVM_Declare_Runtime_Function__(emitter, "read", i64, parameters, 3U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] = descriptor32;
    arguments[1] = LLVMBuildPointerCast(
        emitter->builder, byte_slot, i8_pointer, "runtime.service.read.buffer");
    arguments[2] = LLVMConstInt(i64, 1U, 0);
    count = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 3U, "runtime.service.read.count");
    byte = LLVMBuildLoad2(emitter->builder, i8, byte_slot, "runtime.service.read.value");
    byte64 = LLVMBuildZExt(emitter->builder, byte, i64, "runtime.service.read.byte64");
    is_one = LLVMBuildICmp(
        emitter->builder, LLVMIntEQ, count, LLVMConstInt(i64, 1U, 0), "runtime.service.read.one");
    is_eof = LLVMBuildICmp(
        emitter->builder, LLVMIntEQ, count, LLVMConstInt(i64, 0U, 0), "runtime.service.read.eof");
    non_success = LLVMBuildSelect(emitter->builder,
                                  is_eof,
                                  LLVMConstInt(i64, (unsigned long long)-1LL, 1),
                                  LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                  "runtime.service.read.non.success");
    result.value = LLVMBuildSelect(
        emitter->builder, is_one, byte64, non_success, "runtime.service.read.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime read segment service. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Read_Segment_Service__(__LLVM_Emitter__ *emitter,
                                                                 __Ast_Expression__ *expression,
                                                                 __Ast_Type__ *expected,
                                                                 int stdin_mode)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the descriptor. */
    __LLVM_Value__ descriptor = __LLVM_Invalid_Value__();
    /* Stores the bytes. */
    __LLVM_Value__ bytes;
    /* Stores the offset. */
    __LLVM_Value__ offset;
    /* Stores the requested. */
    __LLVM_Value__ requested;
    /* References the bytes type. */
    __Ast_Type__ *bytes_type;
    /* Tracks the bytes index. */
    size_t bytes_index = stdin_mode ? 0U : 1U;
    /* Tracks the offset index. */
    size_t offset_index = stdin_mode ? 1U : 2U;
    /* Tracks the count index. */
    size_t count_index = stdin_mode ? 2U : 3U;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[3] = {i32, i8_pointer, i64};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the descriptor 32. */
    LLVMValueRef descriptor32;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the negative offset. */
    LLVMValueRef negative_offset;
    /* Stores the negative count. */
    LLVMValueRef negative_count;
    /* Stores the offset too large. */
    LLVMValueRef offset_too_large;
    /* Stores the remaining. */
    LLVMValueRef remaining;
    /* Stores the count too large. */
    LLVMValueRef count_too_large;
    /* Stores the invalid. */
    LLVMValueRef invalid;
    /* Stores the status slot. */
    LLVMValueRef status_slot;
    /* Stores the current function. */
    LLVMValueRef current_function;
    /* Stores the invalid block. */
    LLVMBasicBlockRef invalid_block;
    /* Stores the read block. */
    LLVMBasicBlockRef read_block;
    /* Tracks the done block state. */
    LLVMBasicBlockRef done_block;
    /* Stores the pointer. */
    LLVMValueRef pointer;
    /* Stores the call arguments. */
    LLVMValueRef args[3];
    /* Stores the read count. */
    LLVMValueRef read_count;
    /* Tracks the failed state. */
    LLVMValueRef failed;

    if (expression->__As__.__Call__.__Argument_Count__ != (stdin_mode ? 3U : 4U))
    {
        __LLVM_Fail__("low-level read-segment service disagrees with canonical builtin arity");
        return result;
    }

    if (stdin_mode)
    {
        descriptor32 = LLVMConstInt(i32, 0U, 0);
    }
    else
    {
        descriptor = __LLVM_Emit_Expression__(
            emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
        if (descriptor.value == NULL)
            return result;
        descriptor = __LLVM_Coerce__(emitter, descriptor, &__LLVM_Integer_Type__);
        if (descriptor.value == NULL)
            return result;
        descriptor32 = LLVMBuildTrunc(
            emitter->builder, descriptor.value, i32, "runtime.service.read.segment.fd");
    }

    bytes_type =
        __LLVM_Expression_Type__(emitter, expression->__As__.__Call__.__Arguments__[bytes_index]);
    if (bytes_type == NULL)
        return result;
    bytes = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[bytes_index], bytes_type);
    offset = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[offset_index], &__LLVM_Integer_Type__);
    requested = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[count_index], &__LLVM_Integer_Type__);
    if (bytes.value == NULL || offset.value == NULL || requested.value == NULL)
        return result;
    offset = __LLVM_Coerce__(emitter, offset, &__LLVM_Integer_Type__);
    requested = __LLVM_Coerce__(emitter, requested, &__LLVM_Integer_Type__);
    if (offset.value == NULL || requested.value == NULL)
        return result;

    data = LLVMBuildPointerCast(
        emitter->builder,
        LLVMBuildExtractValue(
            emitter->builder, bytes.value, 0U, "runtime.service.read.segment.data"),
        i8_pointer,
        "runtime.service.read.segment.bytes");
    length = LLVMBuildExtractValue(
        emitter->builder, bytes.value, 1U, "runtime.service.read.segment.length");
    negative_offset = LLVMBuildICmp(emitter->builder,
                                    LLVMIntSLT,
                                    offset.value,
                                    LLVMConstInt(i64, 0U, 0),
                                    "runtime.service.read.segment.offset.negative");
    negative_count = LLVMBuildICmp(emitter->builder,
                                   LLVMIntSLT,
                                   requested.value,
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.service.read.segment.count.negative");
    offset_too_large = LLVMBuildICmp(emitter->builder,
                                     LLVMIntUGT,
                                     offset.value,
                                     length,
                                     "runtime.service.read.segment.offset.large");
    remaining = LLVMBuildSub(
        emitter->builder, length, offset.value, "runtime.service.read.segment.remaining");
    count_too_large = LLVMBuildICmp(emitter->builder,
                                    LLVMIntUGT,
                                    requested.value,
                                    remaining,
                                    "runtime.service.read.segment.count.large");
    invalid = LLVMBuildOr(emitter->builder,
                          negative_offset,
                          negative_count,
                          "runtime.service.read.segment.invalid.sign");
    invalid = LLVMBuildOr(
        emitter->builder, invalid, offset_too_large, "runtime.service.read.segment.invalid.offset");
    invalid = LLVMBuildOr(
        emitter->builder, invalid, count_too_large, "runtime.service.read.segment.invalid.count");

    status_slot = __LLVM_Allocate_Stack__(emitter, i64, "runtime.service.read.segment.status");
    if (status_slot == NULL)
        return result;
    current_function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    if (current_function == NULL)
        return result;
    invalid_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.read.segment.invalid");
    read_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.read.segment.valid");
    done_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.read.segment.done");
    LLVMBuildCondBr(emitter->builder, invalid, invalid_block, read_block);

    LLVMPositionBuilderAtEnd(emitter->builder, invalid_block);
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, (unsigned long long)-2LL, 1), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, read_block);
    pointer = LLVMBuildGEP2(
        emitter->builder, i8, data, &offset.value, 1U, "runtime.service.read.segment.pointer");
    function =
        __LLVM_Declare_Runtime_Function__(emitter, "read", i64, parameters, 3U, &function_type);
    if (function == NULL)
        return result;
    args[0] = descriptor32;
    args[1] = pointer;
    args[2] = requested.value;
    read_count = LLVMBuildCall2(
        emitter->builder, function_type, function, args, 3U, "runtime.service.read.segment.count");
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           read_count,
                           LLVMConstInt(i64, 0U, 0),
                           "runtime.service.read.segment.failed");
    read_count = LLVMBuildSelect(emitter->builder,
                                 failed,
                                 LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                 read_count,
                                 "runtime.service.read.segment.result");
    LLVMBuildStore(emitter->builder, read_count, status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    result.value = LLVMBuildLoad2(
        emitter->builder, i64, status_slot, "runtime.service.read.segment.status.value");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime close file service. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Close_File_Service__(__LLVM_Emitter__ *emitter,
                                                               __Ast_Expression__ *expression,
                                                               __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the descriptor. */
    __LLVM_Value__ descriptor;
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[1] = {i32};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the call arguments. */
    LLVMValueRef arguments[1];
    /* Stores the status. */
    LLVMValueRef status;
    /* Tracks the failed state. */
    LLVMValueRef failed;
    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("low-level close service disagrees with canonical builtin arity");
        return result;
    }
    descriptor = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
    if (descriptor.value == NULL)
        return result;
    descriptor = __LLVM_Coerce__(emitter, descriptor, &__LLVM_Integer_Type__);
    if (descriptor.value == NULL)
        return result;
    function =
        __LLVM_Declare_Runtime_Function__(emitter, "close", i32, parameters, 1U, &function_type);
    if (function == NULL)
        return result;
    arguments[0] =
        LLVMBuildTrunc(emitter->builder, descriptor.value, i32, "runtime.service.close.fd");
    status = LLVMBuildCall2(
        emitter->builder, function_type, function, arguments, 1U, "runtime.service.close.status");
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           status,
                           LLVMConstInt(i32, 0U, 0),
                           "runtime.service.close.failed");
    result.value = LLVMBuildSelect(emitter->builder,
                                   failed,
                                   LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.service.close.result");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime write segment service. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Write_Segment_Service__(__LLVM_Emitter__ *emitter,
                                                                  __Ast_Expression__ *expression,
                                                                  __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the descriptor. */
    __LLVM_Value__ descriptor;
    /* Stores the bytes. */
    __LLVM_Value__ bytes;
    /* Stores the offset. */
    __LLVM_Value__ offset;
    /* Stores the requested. */
    __LLVM_Value__ requested;
    /* References the bytes type. */
    __Ast_Type__ *bytes_type;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the parameters. */
    LLVMTypeRef parameters[3] = {i32, i8_pointer, i64};
    /* Stores the function type. */
    LLVMTypeRef function_type;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the negative offset. */
    LLVMValueRef negative_offset;
    /* Stores the negative count. */
    LLVMValueRef negative_count;
    /* Stores the offset too large. */
    LLVMValueRef offset_too_large;
    /* Stores the remaining. */
    LLVMValueRef remaining;
    /* Stores the count too large. */
    LLVMValueRef count_too_large;
    /* Stores the invalid. */
    LLVMValueRef invalid;
    /* Stores the status slot. */
    LLVMValueRef status_slot;
    /* Stores the current function. */
    LLVMValueRef current_function;
    /* Stores the invalid block. */
    LLVMBasicBlockRef invalid_block;
    /* Stores the write block. */
    LLVMBasicBlockRef write_block;
    /* Tracks the done block state. */
    LLVMBasicBlockRef done_block;
    /* Stores the pointer. */
    LLVMValueRef pointer;
    /* Stores the call arguments. */
    LLVMValueRef args[3];
    /* Stores the written. */
    LLVMValueRef written;
    /* Tracks the failed state. */
    LLVMValueRef failed;

    if (expression->__As__.__Call__.__Argument_Count__ != 4U)
    {
        __LLVM_Fail__("low-level write-segment service disagrees with canonical builtin arity");
        return result;
    }
    descriptor = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_Integer_Type__);
    bytes_type = __LLVM_Expression_Type__(emitter, expression->__As__.__Call__.__Arguments__[1]);
    if (descriptor.value == NULL || bytes_type == NULL)
        return result;
    bytes =
        __LLVM_Emit_Expression__(emitter, expression->__As__.__Call__.__Arguments__[1], bytes_type);
    offset = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[2], &__LLVM_Integer_Type__);
    requested = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[3], &__LLVM_Integer_Type__);
    if (bytes.value == NULL || offset.value == NULL || requested.value == NULL)
        return result;
    descriptor = __LLVM_Coerce__(emitter, descriptor, &__LLVM_Integer_Type__);
    offset = __LLVM_Coerce__(emitter, offset, &__LLVM_Integer_Type__);
    requested = __LLVM_Coerce__(emitter, requested, &__LLVM_Integer_Type__);
    if (descriptor.value == NULL || offset.value == NULL || requested.value == NULL)
        return result;
    data = LLVMBuildPointerCast(
        emitter->builder,
        LLVMBuildExtractValue(emitter->builder, bytes.value, 0U, "runtime.service.write.data"),
        i8_pointer,
        "runtime.service.write.bytes");
    length =
        LLVMBuildExtractValue(emitter->builder, bytes.value, 1U, "runtime.service.write.length");
    negative_offset = LLVMBuildICmp(emitter->builder,
                                    LLVMIntSLT,
                                    offset.value,
                                    LLVMConstInt(i64, 0U, 0),
                                    "runtime.service.write.offset.negative");
    negative_count = LLVMBuildICmp(emitter->builder,
                                   LLVMIntSLT,
                                   requested.value,
                                   LLVMConstInt(i64, 0U, 0),
                                   "runtime.service.write.count.negative");
    offset_too_large = LLVMBuildICmp(
        emitter->builder, LLVMIntUGT, offset.value, length, "runtime.service.write.offset.large");
    remaining =
        LLVMBuildSub(emitter->builder, length, offset.value, "runtime.service.write.remaining");
    count_too_large = LLVMBuildICmp(emitter->builder,
                                    LLVMIntUGT,
                                    requested.value,
                                    remaining,
                                    "runtime.service.write.count.large");
    invalid = LLVMBuildOr(
        emitter->builder, negative_offset, negative_count, "runtime.service.write.invalid.sign");
    invalid = LLVMBuildOr(
        emitter->builder, invalid, offset_too_large, "runtime.service.write.invalid.offset");
    invalid = LLVMBuildOr(
        emitter->builder, invalid, count_too_large, "runtime.service.write.invalid.count");
    status_slot = __LLVM_Allocate_Stack__(emitter, i64, "runtime.service.write.status");
    if (status_slot == NULL)
        return result;
    current_function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    if (current_function == NULL)
        return result;
    invalid_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.write.invalid");
    write_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.write.valid");
    done_block = LLVMAppendBasicBlockInContext(
        emitter->context, current_function, "runtime.service.write.done");
    LLVMBuildCondBr(emitter->builder, invalid, invalid_block, write_block);

    LLVMPositionBuilderAtEnd(emitter->builder, invalid_block);
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, (unsigned long long)-2LL, 1), status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, write_block);
    pointer = LLVMBuildGEP2(
        emitter->builder, i8, data, &offset.value, 1U, "runtime.service.write.pointer");
    function =
        __LLVM_Declare_Runtime_Function__(emitter, "write", i64, parameters, 3U, &function_type);
    if (function == NULL)
        return result;
    args[0] = LLVMBuildTrunc(emitter->builder, descriptor.value, i32, "runtime.service.write.fd");
    args[1] = pointer;
    args[2] = requested.value;
    written = LLVMBuildCall2(
        emitter->builder, function_type, function, args, 3U, "runtime.service.write.count");
    failed = LLVMBuildICmp(emitter->builder,
                           LLVMIntSLT,
                           written,
                           LLVMConstInt(i64, 0U, 0),
                           "runtime.service.write.failed");
    written = LLVMBuildSelect(emitter->builder,
                              failed,
                              LLVMConstInt(i64, (unsigned long long)-2LL, 1),
                              written,
                              "runtime.service.write.result");
    LLVMBuildStore(emitter->builder, written, status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    result.value =
        LLVMBuildLoad2(emitter->builder, i64, status_slot, "runtime.service.write.status.value");
    result.type = &__LLVM_Integer_Type__;
    return expected != NULL ? __LLVM_Coerce__(emitter, result, expected) : result;
}

/* Emits the LLVM runtime write executable bytes. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Write_Executable_Bytes__(__LLVM_Emitter__ *emitter,
                                                                   __Ast_Expression__ *expression,
                                                                   __Ast_Type__ *result_type)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* Stores the path. */
    __LLVM_Value__ path;
    /* Stores the contents. */
    __LLVM_Value__ contents;
    /* References the content type. */
    __Ast_Type__ *content_type;
    /* Stores the content resolved. */
    __Resolved_Type__ content_resolved;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i32 type. */
    LLVMTypeRef i32 = LLVMIntTypeInContext(emitter->context, 32U);
    /* Stores the LLVM i64 type. */
    LLVMTypeRef i64 = LLVMIntTypeInContext(emitter->context, 64U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the fopen parameters. */
    LLVMTypeRef fopen_parameters[2] = {i8_pointer, i8_pointer};
    /* Stores the fopen type. */
    LLVMTypeRef fopen_type;
    /* Stores the fopen function. */
    LLVMValueRef fopen_function;
    /* Stores the fwrite parameters. */
    LLVMTypeRef fwrite_parameters[4] = {i8_pointer, i64, i64, i8_pointer};
    /* Stores the fwrite type. */
    LLVMTypeRef fwrite_type;
    /* Stores the fwrite function. */
    LLVMValueRef fwrite_function;
    /* Stores the fclose parameters. */
    LLVMTypeRef fclose_parameters[1] = {i8_pointer};
    /* Stores the fclose type. */
    LLVMTypeRef fclose_type;
    /* Stores the fclose function. */
    LLVMValueRef fclose_function;
    /* Stores the unlink parameters. */
    LLVMTypeRef unlink_parameters[1] = {i8_pointer};
    /* Stores the unlink type. */
    LLVMTypeRef unlink_type = NULL;
    /* Stores the unlink function. */
    LLVMValueRef unlink_function = NULL;
    /* Stores the chmod parameters. */
    LLVMTypeRef chmod_parameters[2] = {i8_pointer, i32};
    /* Stores the chmod type. */
    LLVMTypeRef chmod_type = NULL;
    /* Stores the chmod function. */
    LLVMValueRef chmod_function = NULL;
    /* Stores the C path string. */
    LLVMValueRef c_path;
    /* Stores the mode. */
    LLVMValueRef mode;
    /* Stores the file. */
    LLVMValueRef file;
    /* Tracks whether the file pointer is null. */
    LLVMValueRef file_is_null;
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the status slot. */
    LLVMValueRef status_slot;
    /* Stores the function. */
    LLVMValueRef function;
    /* Stores the write block. */
    LLVMBasicBlockRef write_block;
    /* Tracks the done block state. */
    LLVMBasicBlockRef done_block;
    /* Stores the written. */
    LLVMValueRef written;
    /* Stores the close result. */
    LLVMValueRef close_result;
    /* Stores the complete. */
    LLVMValueRef complete;
    /* Stores the closed. */
    LLVMValueRef closed;
    /* Tracks the success condition state. */
    LLVMValueRef success_condition;
    /* Stores the status. */
    LLVMValueRef status;
    /* Tracks the success state. */
    __LLVM_Value__ success;

    if (expression->__As__.__Call__.__Argument_Count__ != 2U)
    {
        __LLVM_Fail__("write_executable_bytes disagrees with canonical builtin arity");
        return result;
    }
    path = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], &__LLVM_String_Type__);
    if (path.value == NULL)
        return result;
    content_type = __LLVM_Expression_Type__(emitter, expression->__As__.__Call__.__Arguments__[1]);
    if (content_type == NULL ||
        !__Type_Resolve__(emitter->semantic, content_type, &content_resolved) ||
        content_resolved.__Kind__ != __Resolved_Type_Vector__)
    {
        __LLVM_Fail__("write_executable_bytes content disagrees with canonical semantic Type");
        return result;
    }
    contents = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[1], content_type);
    if (contents.value == NULL)
        return result;
    c_path = __LLVM_Runtime_C_String__(emitter, path.value, "runtime.write.path");
    if (c_path == NULL)
        return result;

    fopen_function = __LLVM_Declare_Runtime_Function__(
        emitter, "fopen", i8_pointer, fopen_parameters, 2U, &fopen_type);
    fwrite_function = __LLVM_Declare_Runtime_Function__(
        emitter, "fwrite", i64, fwrite_parameters, 4U, &fwrite_type);
    fclose_function = __LLVM_Declare_Runtime_Function__(
        emitter, "fclose", i32, fclose_parameters, 1U, &fclose_type);
    unlink_function = __LLVM_Declare_Runtime_Function__(
        emitter, "unlink", i32, unlink_parameters, 1U, &unlink_type);
    chmod_function =
        __LLVM_Declare_Runtime_Function__(emitter, "chmod", i32, chmod_parameters, 2U, &chmod_type);
    if (fopen_function == NULL || fwrite_function == NULL || fclose_function == NULL ||
        unlink_function == NULL || chmod_function == NULL)
        return result;

    data = LLVMBuildPointerCast(
        emitter->builder,
        LLVMBuildExtractValue(emitter->builder, contents.value, 0U, "runtime.write.data"),
        i8_pointer,
        "runtime.write.bytes");
    length = LLVMBuildExtractValue(emitter->builder, contents.value, 1U, "runtime.write.length");
    status_slot = __LLVM_Allocate_Stack__(emitter, i64, "runtime.write.status");
    LLVMBuildStore(emitter->builder, LLVMConstInt(i64, (unsigned long long)-1LL, 1), status_slot);
    {
        /* Stores the call arguments. */
        LLVMValueRef arguments[1] = {c_path};
        (void)LLVMBuildCall2(emitter->builder,
                             unlink_type,
                             unlink_function,
                             arguments,
                             1U,
                             "runtime.write.unlink.old");
    }
    mode = __LLVM_Runtime_Literal_C_String__(emitter, "wbx", "runtime.write.mode");
    {
        /* Stores the call arguments. */
        LLVMValueRef arguments[2] = {c_path, mode};
        file = LLVMBuildCall2(
            emitter->builder, fopen_type, fopen_function, arguments, 2U, "runtime.write.file");
    }
    file_is_null = LLVMBuildICmp(
        emitter->builder, LLVMIntEQ, file, LLVMConstNull(i8_pointer), "runtime.write.open.failed");
    function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(emitter->builder));
    if (function == NULL)
        return result;
    write_block = LLVMAppendBasicBlockInContext(emitter->context, function, "runtime.write.opened");
    done_block = LLVMAppendBasicBlockInContext(emitter->context, function, "runtime.write.done");
    LLVMBuildCondBr(emitter->builder, file_is_null, done_block, write_block);

    LLVMPositionBuilderAtEnd(emitter->builder, write_block);
    {
        /* Stores the call arguments. */
        LLVMValueRef arguments[4] = {data, LLVMConstInt(i64, 1U, 0), length, file};
        written = LLVMBuildCall2(
            emitter->builder, fwrite_type, fwrite_function, arguments, 4U, "runtime.write.count");
    }
    {
        /* Stores the call arguments. */
        LLVMValueRef arguments[1] = {file};
        close_result = LLVMBuildCall2(emitter->builder,
                                      fclose_type,
                                      fclose_function,
                                      arguments,
                                      1U,
                                      "runtime.write.close.status");
    }
    complete =
        LLVMBuildICmp(emitter->builder, LLVMIntEQ, written, length, "runtime.write.complete");
    closed = LLVMBuildICmp(emitter->builder,
                           LLVMIntEQ,
                           close_result,
                           LLVMConstInt(i32, 0U, 0),
                           "runtime.write.closed");
    success_condition = LLVMBuildAnd(emitter->builder, complete, closed, "runtime.write.success");
    {
        /* Stores the call arguments. */
        LLVMValueRef arguments[2] = {c_path, LLVMConstInt(i32, 0755U, 0)};
        /* Stores the chmod result. */
        LLVMValueRef chmod_result = LLVMBuildCall2(emitter->builder,
                                                   chmod_type,
                                                   chmod_function,
                                                   arguments,
                                                   2U,
                                                   "runtime.write.chmod.status");
        /* Stores the chmod ok. */
        LLVMValueRef chmod_ok = LLVMBuildICmp(emitter->builder,
                                              LLVMIntEQ,
                                              chmod_result,
                                              LLVMConstInt(i32, 0U, 0),
                                              "runtime.write.chmod.ok");
        success_condition = LLVMBuildAnd(
            emitter->builder, success_condition, chmod_ok, "runtime.write.executable.success");
    }
    status = LLVMBuildSelect(emitter->builder,
                             success_condition,
                             written,
                             LLVMConstInt(i64, (unsigned long long)-1LL, 1),
                             "runtime.write.result");
    LLVMBuildStore(emitter->builder, status, status_slot);
    LLVMBuildBr(emitter->builder, done_block);

    LLVMPositionBuilderAtEnd(emitter->builder, done_block);
    if (!__LLVM_Runtime_Free__(emitter, c_path))
        return result;
    status = LLVMBuildLoad2(emitter->builder, i64, status_slot, "runtime.write.status.value");
    success.value = status;
    success.type = &__LLVM_Integer_Type__;
    return __LLVM_Build_Runtime_Result__(emitter, result_type, status, success);
}

/* Emits the LLVM runtime stream. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Stream__(__LLVM_Emitter__ *emitter,
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

/* Emits the LLVM runtime exit. */
static __LLVM_Value__ __LLVM_Emit_Runtime_Exit__(__LLVM_Emitter__ *emitter,
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

/* Maps the bytes to the LLVM emit text. */
static __LLVM_Value__ __LLVM_Emit_Text_From_Bytes__(__LLVM_Emitter__ *emitter,
                                                    __Ast_Expression__ *expression,
                                                    __Ast_Type__ *expected)
{
    /* Stores the operation result. */
    __LLVM_Value__ result = __LLVM_Invalid_Value__();
    /* References the source type. */
    __Ast_Type__ *source_type;
    /* Stores the resolved. */
    __Resolved_Type__ resolved;
    /* Stores the source. */
    __LLVM_Value__ source;
    /* Stores the LLVM i8 type. */
    LLVMTypeRef i8 = LLVMIntTypeInContext(emitter->context, 8U);
    /* Stores the LLVM i8 pointer type. */
    LLVMTypeRef i8_pointer = LLVMPointerType(i8, 0U);
    /* Stores the data. */
    LLVMValueRef data;
    /* Stores the length. */
    LLVMValueRef length;
    /* Stores the capacity. */
    LLVMValueRef capacity;
    if (expression->__As__.__Call__.__Argument_Count__ != 1U)
    {
        __LLVM_Fail__("text_from_bytes disagrees with canonical builtin arity");
        return result;
    }
    source_type = __LLVM_Expression_Type__(emitter, expression->__As__.__Call__.__Arguments__[0]);
    if (source_type == NULL || !__Type_Resolve__(emitter->semantic, source_type, &resolved) ||
        resolved.__Kind__ != __Resolved_Type_Vector__)
    {
        __LLVM_Fail__("text_from_bytes requires canonical byte-vector Type");
        return result;
    }
    source = __LLVM_Emit_Expression__(
        emitter, expression->__As__.__Call__.__Arguments__[0], source_type);
    if (source.value == NULL)
        return result;
    data = LLVMBuildPointerCast(
        emitter->builder,
        LLVMBuildExtractValue(emitter->builder, source.value, 0U, "text.from.bytes.data"),
        i8_pointer,
        "text.from.bytes.pointer");
    length = LLVMBuildExtractValue(emitter->builder, source.value, 1U, "text.from.bytes.length");
    capacity =
        LLVMBuildExtractValue(emitter->builder, source.value, 2U, "text.from.bytes.capacity");
    result = __LLVM_Make_Text_Value__(
        emitter, data, length, capacity, expected != NULL ? expected : &__LLVM_String_Type__);
    return result;
}

/* Resolves the LLVM builtin tagged construct. */
static int __LLVM_Resolve_Builtin_Tagged_Construct__(__Ast_Type__ *target_type,
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

/* Emits the LLVM expression. */
static __LLVM_Value__ __LLVM_Emit_Expression__(__LLVM_Emitter__ *emitter,
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
            left = __LLVM_Emit_Expression__(
                emitter, expression->__As__.__Conversion__.__Operand__, NULL);
            if (left.value == NULL)
            {
                /* Untyped integer literals are valid when the explicit target supplies context. */
                if (expression->__As__.__Conversion__.__Operand__->__Kind__ ==
                    __Ast_Expression_Atom__)
                {
                    left = __LLVM_Emit_Atom__(
                        emitter,
                        &expression->__As__.__Conversion__.__Operand__->__As__.__Atom__,
                        expression->__As__.__Conversion__.__Target_Type__);
                }
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
            if (builtin == __Name_Builtin_Host_Architecture__ ||
                builtin == __Name_Builtin_Host_Platform__ ||
                builtin == __Name_Builtin_Host_Environment__)
                return __LLVM_Emit_Host_Identity__(emitter, builtin, expected);
            if (builtin == __Name_Builtin_Argument_Count__)
                return __LLVM_Emit_Argument_Count__(emitter, expected);
            if (builtin == __Name_Builtin_Argument__)
                return __LLVM_Emit_Argument__(emitter, expression, expected);
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
                    (vector_expression = expression->__As__.__Call__.__Arguments__[0]) == NULL ||
                    vector_expression->__Kind__ != __Ast_Expression_Atom__ ||
                    vector_expression->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
                    (vector_lvalue = vector_expression->__As__.__Atom__.__As__.__Lvalue__) == NULL)
                {
                    __LLVM_Fail__("L2.7 append disagrees with canonical builtin semantics");
                    return result;
                }
                vector_place = __LLVM_Emit_Place__(emitter, vector_lvalue);
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
            if (builtin == __Name_Builtin_Open_File_Read__)
                return __LLVM_Emit_Runtime_Open_File_Service__(emitter, expression, expected, 0);
            if (builtin == __Name_Builtin_Read_File_Byte__)
                return __LLVM_Emit_Runtime_Read_Byte_Service__(emitter, expression, expected, 0);
            if (builtin == __Name_Builtin_Read_File_Segment__)
                return __LLVM_Emit_Runtime_Read_Segment_Service__(emitter, expression, expected, 0);
            if (builtin == __Name_Builtin_Create_File_Write__)
                return __LLVM_Emit_Runtime_Open_File_Service__(emitter, expression, expected, 1);
            if (builtin == __Name_Builtin_Write_File_Segment__)
                return __LLVM_Emit_Runtime_Write_Segment_Service__(emitter, expression, expected);
            if (builtin == __Name_Builtin_Close_File__)
                return __LLVM_Emit_Runtime_Close_File_Service__(emitter, expression, expected);
            if (builtin == __Name_Builtin_Open_Directory__)
                return __LLVM_Emit_Runtime_Open_Directory_Service__(emitter, expression, expected);
            if (builtin == __Name_Builtin_Read_Directory_Entry__)
                return __LLVM_Emit_Runtime_Read_Directory_Entry_Service__(emitter, expression, expected);
            if (builtin == __Name_Builtin_Close_Directory__)
                return __LLVM_Emit_Runtime_Close_Directory_Service__(emitter, expression, expected);
            if (builtin == __Name_Builtin_Read_Stdin_Byte__)
                return __LLVM_Emit_Runtime_Read_Byte_Service__(emitter, expression, expected, 1);
            if (builtin == __Name_Builtin_Read_Stdin_Segment__)
                return __LLVM_Emit_Runtime_Read_Segment_Service__(emitter, expression, expected, 1);
            if (builtin == __Name_Builtin_Write_Executable_Bytes__)
                return __LLVM_Emit_Runtime_Write_Executable_Bytes__(
                    emitter, expression, tagged_target);
            if (builtin == __Name_Builtin_Stdout_Write__)
                return __LLVM_Emit_Runtime_Stream__(emitter, expression, expected, 1);
            if (builtin == __Name_Builtin_Stderr_Write__)
                return __LLVM_Emit_Runtime_Stream__(emitter, expression, expected, 2);
            if (builtin == __Name_Builtin_Process_Exit__)
                return __LLVM_Emit_Runtime_Exit__(emitter, expression);
            if (builtin == __Name_Builtin_Text_From_Bytes__)
                return __LLVM_Emit_Text_From_Bytes__(emitter, expression, expected);
            if (builtin == __Name_Builtin_Path_Type__ ||
                builtin == __Name_Builtin_Path_Size__ ||
                builtin == __Name_Builtin_Path_Modified_Time__)
                return __LLVM_Emit_Runtime_Path_Metadata_Service__(
                    emitter, expression, expected, builtin);
            if (builtin != __Name_Builtin_None__)
            {
                __LLVM_Fail__("canonical builtin is outside the native runtime lowering boundary");
                return result;
            }
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
                    target_resolved.__Named__ != enum_type || enum_constructor == NULL ||
                    enum_constructor->__Payload_Count__ !=
                        expression->__As__.__Call__.__Argument_Count__)
                {
                    __LLVM_Fail__(
                        "L2.5 enum construction disagrees with canonical semantic identity");
                    return result;
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
                __LLVM_Fail__("L2.3 call is not an ordinary canonical SultanC function");
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
                    resolved.__Kind__ != __Resolved_Type_Reference__)
                {
                    __LLVM_Fail__(
                        "L2.6 address creation requires canonical semantic reference Type");
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

/* Copies the LLVM tagged payload to storage. */
static LLVMValueRef __LLVM_Copy_Tagged_Payload_To_Storage__(__LLVM_Emitter__ *emitter,
                                                            __Ast_Type__ *tagged_type,
                                                            LLVMTypeRef llvm_tagged_type,
                                                            LLVMValueRef tagged_storage,
                                                            size_t constructor_index,
                                                            size_t payload_index,
                                                            __Ast_Type__ **out_payload_type,
                                                            LLVMTypeRef *out_payload_llvm_type)
{
    /* Stores the canonical offset. */
    size_t canonical_offset = 0U;
    /* Stores the payload size. */
    size_t payload_size = 0U;
    /* Stores the payload alignment. */
    size_t payload_alignment = 1U;
    /* References the payload type. */
    __Ast_Type__ *payload_type = NULL;
    /* Stores the payload LLVM type. */
    LLVMTypeRef payload_llvm_type;
    /* Stores the source. */
    LLVMValueRef source;
    /* Stores the destination. */
    LLVMValueRef destination;
    /* Stores the byte count. */
    LLVMValueRef byte_count;

    if (!__Layout_Tagged_Payload__(emitter->semantic,
                                   tagged_type,
                                   constructor_index,
                                   payload_index,
                                   &canonical_offset,
                                   &payload_type) ||
        payload_type == NULL ||
        !__Layout_Type__(emitter->semantic, payload_type, &payload_size, &payload_alignment))
    {
        __LLVM_Fail__("L2.5 matched payload lacks canonical layout");
        return NULL;
    }
    payload_llvm_type = __LLVM_Type__(emitter, payload_type);
    if (payload_llvm_type == NULL)
    {
        return NULL;
    }
    source = __LLVM_Tagged_Byte_Address__(emitter,
                                          tagged_type,
                                          llvm_tagged_type,
                                          tagged_storage,
                                          canonical_offset,
                                          payload_size,
                                          "match.payload.source");
    if (source == NULL)
    {
        return NULL;
    }
    destination = __LLVM_Allocate_Stack__(emitter, payload_llvm_type, "match.payload.value");
    byte_count = LLVMConstInt(
        LLVMIntTypeInContext(emitter->context, 64U), (unsigned long long)payload_size, 0);
    if (LLVMBuildMemCpy(emitter->builder,
                        destination,
                        (unsigned)payload_alignment,
                        source,
                        (unsigned)payload_alignment,
                        byte_count) == NULL)
    {
        __LLVM_Fail__("L2.5 payload extraction copy failed");
        return NULL;
    }
    if (out_payload_type != NULL)
        *out_payload_type = payload_type;
    if (out_payload_llvm_type != NULL)
        *out_payload_llvm_type = payload_llvm_type;
    return destination;
}

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

/* Emits the LLVM statement. */
static int __LLVM_Emit_Statement__(__LLVM_Emitter__ *emitter,
                                   __Ast_Statement__ *statement,
                                   __Ast_Type__ *return_type,
                                   int *path_terminated);

/* Emits the LLVM statement list. */
static int __LLVM_Emit_Statement_List__(__LLVM_Emitter__ *emitter,
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

/* Emits the LLVM match. */
static int __LLVM_Emit_Match__(__LLVM_Emitter__ *emitter,
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
    /* Tracks whether the operation succeeded. */
    int ok = 0;

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
        goto done;
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
            goto done;
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
            goto done;
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
                    goto done;
            }
            else
            {
                /* Stores the expected tag. */
                LLVMValueRef expected_tag;
                if (tag_value == NULL)
                {
                    __LLVM_Fail__("Direct LLVM constructor dispatch requires tagged storage");
                    goto done;
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
                goto done;
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
            goto done;
        }
        if (!__LLVM_Emit_Statement_List__(emitter,
                                          statement->__As__.__Match__.__Cases__[index].__Body__,
                                          return_type,
                                          &terminated))
        {
            emitter->local_count = saved_local_count;
            goto done;
        }
        arm_exits[index] = LLVMGetInsertBlock(emitter->builder);
        if (arm_exits[index] == NULL ||
            ((LLVMGetBasicBlockTerminator(arm_exits[index]) != NULL) != (terminated != 0)))
        {
            emitter->local_count = saved_local_count;
            __LLVM_Fail__("L2.5 inconsistent match-arm CFG termination state");
            goto done;
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
        ok = 1;
        goto done;
    }

    merge_block = LLVMAppendBasicBlockInContext(emitter->context, function, "match.end");
    if (merge_block == NULL)
    {
        __LLVM_Fail__("L2.5 could not create match continuation");
        goto done;
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
    ok = 1;

done:
    free(case_blocks);
    free(arm_exits);
    free(tags);
    free(has_tag);
    free(needs_full_test);
    free(arm_terminated);
    return ok;
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
                field_value = __LLVM_Emit_Atom__(emitter, &input->__Value__, field_type);
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

/* Initializes the LLVM host target. */
static int __LLVM_Initialize_Host_Target__(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmPrinter();
    return 1;
#elif defined(__aarch64__) || defined(_M_ARM64)
    LLVMInitializeAArch64TargetInfo();
    LLVMInitializeAArch64Target();
    LLVMInitializeAArch64TargetMC();
    LLVMInitializeAArch64AsmPrinter();
    return 1;
#else
    return __LLVM_Fail__(
        "current Bootstrap LLVM target initialization supports x86_64/AArch64 hosts");
#endif
}

/* Returns the LLVM predeclare aggregate types. */
static int __LLVM_Predeclare_Aggregate_Types__(__LLVM_Emitter__ *emitter)
{
    /* Tracks the index. */
    size_t index;
    /* Stores the count. */
    size_t count = 0U;
    for (index = 0U; index < emitter->semantic->__Types__.__Count__; ++index)
    {
        /* References the entry. */
        __Semantic_Type_Entry__ *entry =
            (__Semantic_Type_Entry__ *)__Vector_At__(&emitter->semantic->__Types__, index);
        if (entry != NULL && entry->__Declaration__ != NULL &&
            (entry->__Declaration__->__Kind__ == __Ast_Type_Decl_Struct__ ||
             entry->__Declaration__->__Kind__ == __Ast_Type_Decl_Enum__))
        {
            ++count;
        }
    }
    emitter->aggregate_type_count = count;
    if (count == 0U)
    {
        return 1;
    }
    emitter->aggregate_types =
        (__LLVM_Aggregate_Type__ *)calloc(count, sizeof(*emitter->aggregate_types));
    if (emitter->aggregate_types == NULL)
    {
        return __LLVM_Fail__("out of memory while predeclaring L2.5 aggregate Types");
    }
    count = 0U;
    for (index = 0U; index < emitter->semantic->__Types__.__Count__; ++index)
    {
        /* References the entry. */
        __Semantic_Type_Entry__ *entry =
            (__Semantic_Type_Entry__ *)__Vector_At__(&emitter->semantic->__Types__, index);
        /* Stores the symbol. */
        char symbol[64];
        if (entry == NULL || entry->__Declaration__ == NULL ||
            (entry->__Declaration__->__Kind__ != __Ast_Type_Decl_Struct__ &&
             entry->__Declaration__->__Kind__ != __Ast_Type_Decl_Enum__))
        {
            continue;
        }
        snprintf(symbol,
                 sizeof(symbol),
                 entry->__Declaration__->__Kind__ == __Ast_Type_Decl_Enum__ ? "sultanc.enum.%zu"
                                                                            : "sultanc.record.%zu",
                 index);
        emitter->aggregate_types[count].semantic = entry;
        emitter->aggregate_types[count].type = LLVMStructCreateNamed(emitter->context, symbol);
        if (emitter->aggregate_types[count].type == NULL)
        {
            return __LLVM_Fail__("LLVM named aggregate declaration failed for L2.5");
        }
        ++count;
    }
    return 1;
}

/* Returns the LLVM define aggregate types. */
static int __LLVM_Define_Aggregate_Types__(__LLVM_Emitter__ *emitter)
{
    /* Tracks the aggregate index. */
    size_t aggregate_index;
    for (aggregate_index = 0U; aggregate_index < emitter->aggregate_type_count; ++aggregate_index)
    {
        /* References the aggregate. */
        __LLVM_Aggregate_Type__ *aggregate = &emitter->aggregate_types[aggregate_index];
        /* References the entry. */
        __Semantic_Type_Entry__ *entry = aggregate->semantic;
        /* References the declaration. */
        __Ast_Type_Declaration__ *declaration = entry != NULL ? entry->__Declaration__ : NULL;
        if (entry == NULL || declaration == NULL || entry->__Layout_State__ != 2)
        {
            return __LLVM_Fail__("L2.5 aggregate lacks canonical semantic layout");
        }
        emitter->semantic->__Active_Unit__ = entry->__Unit__;
        if (declaration->__Kind__ == __Ast_Type_Decl_Struct__)
        {
            /* References the field types. */
            LLVMTypeRef *field_types = NULL;
            /* Tracks the field index. */
            size_t field_index;
            if (declaration->__As__.__Struct__.__Count__ != 0U)
            {
                field_types = (LLVMTypeRef *)calloc(declaration->__As__.__Struct__.__Count__,
                                                    sizeof(*field_types));
                if (field_types == NULL)
                {
                    return __LLVM_Fail__("out of memory while defining L2.4 record Type");
                }
            }
            for (field_index = 0U; field_index < declaration->__As__.__Struct__.__Count__;
                 ++field_index)
            {
                /* References the field. */
                __Ast_Struct_Field__ *field =
                    &declaration->__As__.__Struct__.__Fields__[field_index];
                /* Stores the canonical offset. */
                size_t canonical_offset = 0U;
                /* References the canonical type. */
                __Ast_Type__ *canonical_type = NULL;
                if (!__Layout_Struct_Field__(emitter->semantic,
                                             entry,
                                             field->__Name__,
                                             &canonical_offset,
                                             &canonical_type) ||
                    canonical_type != field->__Slot__.__Type__)
                {
                    free(field_types);
                    return __LLVM_Fail__("L2.4 could not consume canonical struct field layout");
                }
                (void)canonical_offset;
                field_types[field_index] = __LLVM_Type__(emitter, canonical_type);
                if (field_types[field_index] == NULL)
                {
                    free(field_types);
                    return 0;
                }
            }
            LLVMStructSetBody(aggregate->type,
                              field_types,
                              (unsigned)declaration->__As__.__Struct__.__Count__,
                              0);
            free(field_types);
        }
        else if (declaration->__Kind__ == __Ast_Type_Decl_Enum__)
        {
            /* Stores the named type. */
            __Ast_Type__ named_type;
            /* Stores the elements. */
            LLVMTypeRef elements[2];
            /* Stores the payload size. */
            size_t payload_size = 0U;
            memset(&named_type, 0, sizeof(named_type));
            named_type.__Kind__ = __Ast_Type_Named__;
            named_type.__As__.__Named__.__Name__ = entry->__Name__;
            if (!__LLVM_Tagged_Layout__(emitter, &named_type, &payload_size, NULL, NULL))
            {
                return 0;
            }
            elements[0] = LLVMIntTypeInContext(emitter->context, 64U);
            elements[1] =
                LLVMArrayType(LLVMIntTypeInContext(emitter->context, 8U), (unsigned)payload_size);
            LLVMStructSetBody(aggregate->type, elements, 2U, 0);
        }
        else
        {
            return __LLVM_Fail__("L2.5 unknown canonical aggregate declaration kind");
        }
    }
    return 1;
}

/* Returns the LLVM declare functions. */
static int __LLVM_Declare_Functions__(__LLVM_Emitter__ *emitter)
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
static int __LLVM_Emit_Function_Body__(__LLVM_Emitter__ *emitter, __LLVM_Function__ *llvm_function)
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
static int __LLVM_Emit_Process_Entry__(__LLVM_Emitter__ *emitter)
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

/* Releases the LLVM program module. */
static void __LLVM_Dispose_Program_Module__(__LLVM_Emitter__ *emitter,
                                            LLVMTargetMachineRef machine,
                                            LLVMTargetDataRef data,
                                            char *triple,
                                            char *layout)
{
    if (layout != NULL)
        LLVMDisposeMessage(layout);
    if (data != NULL)
        LLVMDisposeTargetData(data);
    if (machine != NULL)
        LLVMDisposeTargetMachine(machine);
    if (triple != NULL)
        LLVMDisposeMessage(triple);
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
}

/* Returns the LLVM prepare program module. */
static int __LLVM_Prepare_Program_Module__(__Semantic_Context__ *semantic,
                                           __LLVM_Emitter__ *emitter,
                                           const __Program_Unit__ **saved_unit,
                                           LLVMTargetMachineRef *machine,
                                           LLVMTargetDataRef *data,
                                           char **triple,
                                           char **layout)
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
    /* Tracks the index. */
    size_t index;

    memset(emitter, 0, sizeof(*emitter));
    *saved_unit = NULL;
    *machine = NULL;
    *data = NULL;
    *triple = NULL;
    *layout = NULL;
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

    if (!__LLVM_Initialize_Host_Target__())
        return 0;

    emitter->semantic = semantic;
    emitter->context = LLVMContextCreate();
    emitter->module = LLVMModuleCreateWithNameInContext("sultanc_stage0_l25", emitter->context);
    emitter->builder = LLVMCreateBuilderInContext(emitter->context);
    emitter->allocation_builder = LLVMCreateBuilderInContext(emitter->context);
    if (emitter->context == NULL || emitter->module == NULL || emitter->builder == NULL ||
        emitter->allocation_builder == NULL)
        return __LLVM_Fail__("LLVM context/module/builder creation failed");

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

    *triple = LLVMGetDefaultTargetTriple();
    if (*triple == NULL)
        return __LLVM_Fail__("LLVM did not provide a host target triple");
    LLVMSetTarget(emitter->module, *triple);
    if (LLVMGetTargetFromTriple(*triple, &target, &message))
    {
        __LLVM_Fail_Message__("LLVM host target lookup failed", message);
        return 0;
    }
    *machine = LLVMCreateTargetMachine(
        target, *triple, "", "", LLVMCodeGenLevelNone, LLVMRelocDefault, LLVMCodeModelDefault);
    if (*machine == NULL)
        return __LLVM_Fail__("LLVM TargetMachine creation failed");
    *data = LLVMCreateTargetDataLayout(*machine);
    if (*data == NULL)
        return __LLVM_Fail__("LLVM target data layout creation failed");
    *layout = LLVMCopyStringRepOfTargetData(*data);
    if (*layout == NULL)
        return __LLVM_Fail__("LLVM target data layout string creation failed");
    LLVMSetDataLayout(emitter->module, *layout);
    return 1;
}

/* Emits the bootstrap LLVM object. */
int __Bootstrap_Emit_LLVM_Object__(__Semantic_Context__ *semantic, const char *path)
{
    /* Stores the emitter. */
    __LLVM_Emitter__ emitter;
    /* References the saved unit. */
    const __Program_Unit__ *saved_unit = NULL;
    /* Stores the machine. */
    LLVMTargetMachineRef machine = NULL;
    /* Stores the data. */
    LLVMTargetDataRef data = NULL;
    /* References the triple. */
    char *triple = NULL;
    /* References the layout. */
    char *layout = NULL;
    /* References the message. */
    char *message = NULL;
    /* Tracks whether the operation succeeded. */
    int ok = 0;

    memset(&emitter, 0, sizeof(emitter));
    if (path == NULL)
        return __LLVM_Fail__("missing native object output path");
    if (!__LLVM_Prepare_Program_Module__(
            semantic, &emitter, &saved_unit, &machine, &data, &triple, &layout))
        goto done;
    if (LLVMTargetMachineEmitToFile(
            machine, emitter.module, (char *)path, LLVMObjectFile, &message))
    {
        __LLVM_Fail_Message__("LLVM native object emission failed", message);
        message = NULL;
        goto done;
    }
    ok = 1;

done:
    if (message != NULL)
        LLVMDisposeMessage(message);
    __LLVM_Dispose_Program_Module__(&emitter, machine, data, triple, layout);
    if (semantic != NULL && saved_unit != NULL)
        semantic->__Active_Unit__ = saved_unit;
    return ok;
}

/* Runs the bootstrap LLVM program. */
int __Bootstrap_Run_LLVM_Program__(__Semantic_Context__ *semantic,
                                   int argc,
                                   const char *const *argv,
                                   int *status)
{
    /* Stores the emitter. */
    __LLVM_Emitter__ emitter;
    /* References the saved unit. */
    const __Program_Unit__ *saved_unit = NULL;
    /* Stores the machine. */
    LLVMTargetMachineRef machine = NULL;
    /* Stores the data. */
    LLVMTargetDataRef data = NULL;
    /* Stores the engine. */
    LLVMExecutionEngineRef engine = NULL;
    /* Stores the main value. */
    LLVMValueRef main_value = NULL;
    /* References the triple. */
    char *triple = NULL;
    /* References the layout. */
    char *layout = NULL;
    /* References the message. */
    char *message = NULL;
    /* Tracks whether the operation succeeded. */
    int ok = 0;

    memset(&emitter, 0, sizeof(emitter));
    if (status == NULL || argc < 1 || argv == NULL)
        return __LLVM_Fail__("invalid Direct-LLVM execution request");
    if (!__LLVM_Prepare_Program_Module__(
            semantic, &emitter, &saved_unit, &machine, &data, &triple, &layout))
        goto done;

    main_value = LLVMGetNamedFunction(emitter.module, "main");
    if (main_value == NULL)
    {
        __LLVM_Fail__("Direct-LLVM execution module has no process entry");
        goto done;
    }

    LLVMLinkInMCJIT();
    if (LLVMCreateExecutionEngineForModule(&engine, emitter.module, &message))
    {
        __LLVM_Fail_Message__("LLVM execution engine creation failed", message);
        message = NULL;
        goto done;
    }
    emitter.module = NULL;
    *status = LLVMRunFunctionAsMain(engine, main_value, (unsigned)argc, argv, NULL);
    ok = 1;

done:
    if (message != NULL)
        LLVMDisposeMessage(message);
    if (engine != NULL)
        LLVMDisposeExecutionEngine(engine);
    __LLVM_Dispose_Program_Module__(&emitter, machine, data, triple, layout);
    if (semantic != NULL && saved_unit != NULL)
        semantic->__Active_Unit__ = saved_unit;
    return ok;
}

/* Returns the bootstrap LLVM emitter error. */
const char *__Bootstrap_LLVM_Emitter_Error__(void)
{
    return __LLVM_Error__;
}
