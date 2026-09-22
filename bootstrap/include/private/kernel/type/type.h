#ifndef SULTANC__KERNEL_TYPE_TYPE_H__
#define SULTANC__KERNEL_TYPE_TYPE_H__

#include "model.h"

__Type_Builtin_Id__ __Type_Builtin_Id_From_Ast__(const __Ast_Type__ *__Type__);

__Ast_Type__ *__Type_Unwrap_Mutable__(__Ast_Type__ *__Type__);

int __Type_Resolve__(__Semantic_Context__ *__Context__,
                     __Ast_Type__ *__Type__,
                     __Resolved_Type__ *__Out_Type__);

int __Type_Compatible__(__Semantic_Context__ *__Context__,
                        __Ast_Type__ *__Left__,
                        __Ast_Type__ *__Right__);

int __Type_Is_Scalar__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__);

int __Type_Is_Signed__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__);

#endif
