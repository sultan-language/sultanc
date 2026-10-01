/* Declares private Stage0 LLVM type lowering. */

#ifndef SULTANC_BOOTSTRAP_LLVM_TYPES_H
#define SULTANC_BOOTSTRAP_LLVM_TYPES_H

#include "llvm/llvm_internal.h"

/* Stores the LLVM boolean type. */
extern __Ast_Type__ __LLVM_Boolean_Type__;
/* Stores the LLVM integer type. */
extern __Ast_Type__ __LLVM_Integer_Type__;
/* Stores the LLVM string type. */
extern __Ast_Type__ __LLVM_String_Type__;
/* Stores the LLVM void type. */
extern __Ast_Type__ __LLVM_Void_Type__;
/* Stores the LLVM result text integer type. */
extern __Ast_Type__ __LLVM_Result_Text_Integer_Type__;
/* Stores the LLVM result integer integer type. */
extern __Ast_Type__ __LLVM_Result_Integer_Integer_Type__;
/* Stores the LLVM u 8 type. */
extern __Ast_Type__ __LLVM_U8_Type__;

int __LLVM_Resolve_Integer__(__LLVM_Emitter__ *emitter,
                             __Ast_Type__ *type,
                             unsigned *bits,
                             int *is_signed);
int __LLVM_Resolve_Struct_Field__(__LLVM_Emitter__ *emitter,
                                  __Ast_Type__ *parent_type,
                                  __Text_Slice__ field_name,
                                  size_t *out_index,
                                  size_t *out_offset,
                                  __Ast_Type__ **out_type);
int __LLVM_Enum_Is_Payload_Free__(__Semantic_Type_Entry__ *entry);
int __LLVM_Tagged_Layout__(__LLVM_Emitter__ *emitter,
                           __Ast_Type__ *type,
                           size_t *payload_size,
                           size_t *payload_offset,
                           size_t *alignment);
LLVMTypeRef __LLVM_Type__(__LLVM_Emitter__ *emitter, __Ast_Type__ *type);
__Ast_Type__ *__LLVM_Concrete_Integer_Type__(__LLVM_Emitter__ *emitter,
                                             __Ast_Type__ *candidate);
__Ast_Type__ *__LLVM_Expression_Integer_Type__(__LLVM_Emitter__ *emitter,
                                               __Ast_Expression__ *expression);
__Ast_Type__ *__LLVM_Expression_Type__(__LLVM_Emitter__ *emitter,
                                       __Ast_Expression__ *expression);

#endif
