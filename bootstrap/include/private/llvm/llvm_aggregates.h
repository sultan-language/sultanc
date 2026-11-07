/* Declares private Stage0 LLVM aggregate lowering. */

#ifndef SULTANC_BOOTSTRAP_LLVM_AGGREGATES_H
#define SULTANC_BOOTSTRAP_LLVM_AGGREGATES_H

#include "llvm/llvm_internal.h"

__LLVM_Aggregate_Type__ *__LLVM_Find_Aggregate_Type__(__LLVM_Emitter__ *emitter,
                                                       __Semantic_Type_Entry__ *semantic);
__LLVM_Value__ __LLVM_Emit_Tagged_Construct__(__LLVM_Emitter__ *emitter,
                                              __Ast_Type__ *target_type,
                                              size_t constructor_index,
                                              __Ast_Expression__ **arguments,
                                              size_t argument_count);
int __LLVM_Write_Tagged_Arm__(__LLVM_Emitter__ *emitter,
                              __Ast_Type__ *tagged_type,
                              LLVMTypeRef tagged_llvm_type,
                              LLVMValueRef tagged_storage,
                              size_t constructor_index,
                              __LLVM_Value__ payload);
LLVMValueRef __LLVM_Copy_Tagged_Payload_To_Storage__(__LLVM_Emitter__ *emitter,
                                                     __Ast_Type__ *tagged_type,
                                                     LLVMTypeRef llvm_tagged_type,
                                                     LLVMValueRef tagged_storage,
                                                     size_t constructor_index,
                                                     size_t payload_index,
                                                     __Ast_Type__ **out_payload_type,
                                                     LLVMTypeRef *out_payload_llvm_type);
int __LLVM_Predeclare_Aggregate_Types__(__LLVM_Emitter__ *emitter);
int __LLVM_Define_Aggregate_Types__(__LLVM_Emitter__ *emitter);
/* Recursively releases the owned value of static type `type` stored at
 * `address`. Callers must only invoke this where __Memory_Type_Is_Owned__
 * already held for `type`. */
int __LLVM_Drop_Owned_At__(__LLVM_Emitter__ *emitter, LLVMValueRef address, __Ast_Type__ *type);

#endif
