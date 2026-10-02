/* Declares Stage0 generic instantiation support. */

#ifndef SULTANC__SEMANTIC_GENERIC_H__
#define SULTANC__SEMANTIC_GENERIC_H__

#include "semantic/context.h"

/* Builds an arena-owned function type from a concrete function signature. */
__Ast_Type__ *__Semantic_Make_Function_Type__(__Semantic_Context__ *__Context__,
                                              const __Ast_Function__ *__Function__);

/* Substitutes generic type parameters into an arena-owned concrete type. */
__Ast_Type__ *__Semantic_Generic_Substitute_Type__(__Semantic_Context__ *__Context__,
                                                    __Ast_Type__ *__Type__,
                                                    const __Text_Slice__ *__Parameters__,
                                                    __Ast_Type__ *const *__Arguments__,
                                                    size_t __Argument_Count__,
                                                    const __Program_Unit__ *__Owner_Unit__);

/* Resolves or creates a concrete generic named-type entry. */
int __Semantic_Generic_Resolve_Type_Instance__(__Semantic_Context__ *__Context__,
                                               __Semantic_Type_Entry__ *__Template__,
                                               __Ast_Type__ *__Use_Type__,
                                               __Semantic_Type_Entry__ **__Out_Entry__);

/* Returns or creates a concrete generic function instance. */
__Semantic_Function_Entry__ *__Semantic_Generic_Function_Instance__(
    __Semantic_Context__ *__Context__,
    __Semantic_Function_Entry__ *__Template__,
    __Ast_Type__ *const *__Arguments__,
    size_t __Argument_Count__);

/* Returns the generic function instance at the given instance index. */
__Semantic_Function_Entry__ *__Semantic_Generic_Function_At__(__Semantic_Context__ *__Context__,
                                                             size_t __Index__);

/* Checks exact semantic equality for concrete generic arguments. */
int __Semantic_Generic_Type_Equal__(__Semantic_Context__ *__Context__,
                                    __Ast_Type__ *__Left__,
                                    __Ast_Type__ *__Right__);

#endif
