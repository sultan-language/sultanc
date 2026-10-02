/* Declares shared private Stage0 LLVM emitter state and types. */

#ifndef SULTANC_BOOTSTRAP_LLVM_INTERNAL_H
#define SULTANC_BOOTSTRAP_LLVM_INTERNAL_H

#include "llvm/llvm_emitter.h"
#include "llvm/llvm_c_api.h"
#include "target/bootstrap_target.h"

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
    /* Stores the canonical bootstrap compilation target. */
    __Bootstrap_Target__ target;
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

#endif
