/* Declares canonical type resolution and compatibility. */

#ifndef SULTANC__KERNEL_TYPE_TYPE_H__
#define SULTANC__KERNEL_TYPE_TYPE_H__

#include "model.h"

/* Maps the AST to the type builtin ID. */
__Type_Builtin_Id__ __Type_Builtin_Id_From_Ast__(const __Ast_Type__ *__Type__);

/* Unwraps the type mutable. */
__Ast_Type__ *__Type_Unwrap_Mutable__(__Ast_Type__ *__Type__);

/* Resolves the type. */
int __Type_Resolve__(__Semantic_Context__ *__Context__,
                     __Ast_Type__ *__Type__,
                     __Resolved_Type__ *__Out_Type__);

/* Returns the type compatible. */
int __Type_Compatible__(__Semantic_Context__ *__Context__,
                        __Ast_Type__ *__Left__,
                        __Ast_Type__ *__Right__);

/* Checks whether the type is scalar. */
int __Type_Is_Scalar__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__);

/* Checks whether the type is signed. */
int __Type_Is_Signed__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__);

#endif
