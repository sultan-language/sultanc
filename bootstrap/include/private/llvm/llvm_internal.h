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
    /* Stores the drop-elaboration runtime flag (i1*), non-NULL exactly when
     * __Memory_Type_Is_Owned__ holds for this local's type. True means this
     * local currently holds a live value this function must still release;
     * cleared on move, set on (re)initialization, checked before overwrite
     * and at every function exit. NULL for non-owning locals. */
    LLVMValueRef drop_flag;
    /* If this is a match payload bound from an owning scrutinee, transfer
     * the original owner's drop responsibility when the payload is moved.
     * Borrowed provenance is tracked separately, even for match aliases. */
    LLVMValueRef match_owner_flag;
    /* Independent runtime provenance of this local's current value. Unlike
     * drop_flag, this remains meaningful for non-owning pattern bindings and
     * changes when an owning-shaped local is reassigned from a reference. */
    LLVMValueRef borrowed_flag;
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
    /* Tracks whether a live function references this declaration. */
    int queued;
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

/* Defines one memoized drop-glue function: a real generated LLVM function
 * that releases one owned value of a given named (Struct/Enum) shape,
 * taking its storage address and returning void. Named types are the only
 * shape that can introduce a type-level cycle (e.g. a struct containing
 * `box` of itself), since Vector/Box/Option/Result are structural
 * containers whose element/inner type is stated directly; recursion
 * through them only cycles by eventually reaching a Named type again. Key
 * is the canonical __Semantic_Type_Entry__* for that declaration (stable
 * across every occurrence of the same declared type, per
 * __Semantic_Resolve_Named_Entry__), so this is one function per distinct
 * declared type, never per occurrence. Drop glue for a Named type calls
 * through this function instead of inlining its body at every recursion
 * site, which is what turns an unbounded (for a self-referential type,
 * infinite) C-level inlining recursion into a single, finite, ordinary
 * recursive LLVM function call. */
typedef struct
{
    /* References the canonical named-type entry this function drops. */
    __Semantic_Type_Entry__ *key;
    /* Stores the function's own LLVM aggregate type (its parameter's
     * pointee type), needed to rebuild the pointer type at call sites. */
    LLVMTypeRef llvm_type;
    /* Stores the generated function value, declared (and registered here)
     * before its body is built so a self- or mutually-recursive reference
     * encountered while building the body resolves to this same forward
     * declaration instead of recursing in C. */
    LLVMValueRef function;
    /* Tracks whether the body has been built yet (false immediately after
     * registration, true once __LLVM_Build_Drop_Function_Body__ returns),
     * purely for an internal consistency check -- callers never need to
     * wait on this, since the function is a valid call target the moment
     * it is registered. */
    int defined;
} __LLVM_Drop_Function__;

/* Defines the LLVM emitter structure. */
typedef struct
{
    /* References the semantic. */
    __Semantic_Context__ *semantic;
    /* Borrows the canonical bootstrap compilation target owned by the driver. */
    const __Bootstrap_Target__ *target;
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
    /* Worklist of reachable functions, populated during LLVM lowering. */
    __LLVM_Function__ **function_worklist;
    size_t function_worklist_count;
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
    /* Stores the free function (drop elaboration's release boundary). */
    LLVMValueRef free_function;
    /* Stores the free type. */
    LLVMTypeRef free_type;
    /* Stores the memcmp function. */
    LLVMValueRef memcmp_function;
    /* Stores the memcmp type. */
    LLVMTypeRef memcmp_type;
    /* Stores the string literal count. */
    size_t string_literal_count;
    /* References the memoized drop-glue functions for Named (Struct/Enum,
     * including Option/Result instances backed by a declaration) owned
     * types; see __LLVM_Drop_Function__ for why this exists. */
    __LLVM_Drop_Function__ *drop_functions;
    /* Stores the drop-function count. */
    size_t drop_function_count;
    /* Stores the drop-function capacity. */
    size_t drop_function_capacity;
    /* The enclosing loops of the function being lowered: the target of
     * `continue` (the condition, or a `لكل` loop's step) and of `break`. */
    LLVMBasicBlockRef *loop_continue_blocks;
    LLVMBasicBlockRef *loop_break_blocks;
    size_t loop_depth;
    size_t loop_capacity;
    /* The storage and Type of the call result most recently materialized as
     * the base of a place (`make().f`), for the read that destroys it. */
    LLVMValueRef call_temporary_address;
    __Ast_Type__ *call_temporary_type;
    /* Tracks the failed state. */
    int failed;
} __LLVM_Emitter__;

#endif
