/* Dispatches expression type inference. */

#include "semantic/body_internal.h"
#include "frontend/identifier_identity.h"

/* Matches an unbound الفارغ name occurrence (canonical هل_مؤشر_فارغ_سياقي):
 * no local, function or module value of that name is visible. */
int __Body_Is_Unbound_Null_Name__(__Semantic_Body_Context__ *context,
                                  __Ast_Expression__ *expression)
{
    __Ast_Lvalue__ *lvalue;
    __Semantic_Function_Entry__ *function = NULL;
    __Semantic_Global_Entry__ *global = NULL;
    __Text_Slice__ name;
    const __Program_Unit__ *unit;

    if (context == NULL || context->__Function__ == NULL || expression == NULL ||
        expression->__Kind__ != __Ast_Expression_Atom__ ||
        expression->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
        (lvalue = expression->__As__.__Atom__.__As__.__Lvalue__) == NULL ||
        lvalue->__Kind__ != __Ast_Lvalue_Base__ ||
        lvalue->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__)
    {
        return 0;
    }
    name = lvalue->__As__.__Base__.__As__.__Identifier__;
    if (!__Identifier_Identity_Equals_Cstr__(name, "الفارغ") ||
        __Body_Find_Local__(context, lvalue) != NULL)
    {
        return 0;
    }
    unit = context->__Function__->__Unit__;
    return __Name_Resolve_Function__(context->__Semantic__, unit, name, &function) ==
               __Name_Lookup_Missing__ &&
           __Name_Resolve_Global__(context->__Semantic__, unit, name, &global) ==
               __Name_Lookup_Missing__;
}

/* Matches only an unbound null spelling in a raw pointer context. */
static int __Body_Is_Contextual_Null__(__Semantic_Body_Context__ *context,
                                      __Ast_Expression__ *expression)
{
    __Ast_Lvalue__ *lvalue;
    __Resolved_Type__ resolved;
    __Semantic_Function_Entry__ *function = NULL;
    __Semantic_Global_Entry__ *global = NULL;
    __Text_Slice__ name;
    const __Program_Unit__ *unit;

    if (context == NULL || context->__Function__ == NULL ||
        expression->__Kind__ != __Ast_Expression_Atom__ ||
        expression->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
        (lvalue = expression->__As__.__Atom__.__As__.__Lvalue__) == NULL ||
        lvalue->__Kind__ != __Ast_Lvalue_Base__ ||
        lvalue->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__ ||
        expression->__Contextual_Type__ == NULL ||
        !__Type_Resolve__(context->__Semantic__, expression->__Contextual_Type__, &resolved) ||
        resolved.__Kind__ != __Resolved_Type_Pointer__)
    {
        return 0;
    }
    name = lvalue->__As__.__Base__.__As__.__Identifier__;
    if (!__Identifier_Identity_Equals_Cstr__(name, "الفارغ") ||
        __Body_Find_Local__(context, lvalue) != NULL)
    {
        return 0;
    }
    unit = context->__Function__->__Unit__;
    return __Name_Resolve_Function__(context->__Semantic__, unit, name, &function) ==
               __Name_Lookup_Missing__ &&
           __Name_Resolve_Global__(context->__Semantic__, unit, name, &global) ==
               __Name_Lookup_Missing__;
}

/* Infers the body expression. */
int __Body_Infer_Expression__(__Semantic_Body_Context__ *__Context__,
                              __Ast_Expression__ *__Expression__,
                              __Ast_Type__ **__Out_Type__)
{
    if (__Expression__ == NULL)
    {
        return 0;
    }
    if (__Expression__->__Kind__ == __Ast_Expression_Atom__)
    {
        if (__Expression__->__Semantic_Null_Pointer__ ||
            __Body_Is_Contextual_Null__(__Context__, __Expression__))
        {
            __Resolved_Type__ resolved;
            if (__Expression__->__Contextual_Type__ == NULL ||
                !__Type_Resolve__(__Context__->__Semantic__,
                                  __Expression__->__Contextual_Type__, &resolved) ||
                resolved.__Kind__ != __Resolved_Type_Pointer__)
                return __Body_Fail__(__Context__, __E0400_Mismatched_Types__,
                                     __Expression__->__Header__.__Span__);
            __Expression__->__Semantic_Null_Pointer__ = 1;
            *__Out_Type__ = __Expression__->__Contextual_Type__;
            return 1;
        }
        return __Body_Infer_Atom__(__Context__, &__Expression__->__As__.__Atom__, __Out_Type__);
    }
    if (__Expression__->__Kind__ == __Ast_Expression_Call__)
    {
        return __Body_Infer_Call__(__Context__, __Expression__, __Out_Type__);
    }
    if (__Expression__->__Kind__ == __Ast_Expression_Unary__)
    {
        return __Body_Infer_Unary__(__Context__, __Expression__, __Out_Type__);
    }
    if (__Expression__->__Kind__ == __Ast_Expression_Binary__)
    {
        return __Body_Infer_Binary__(__Context__, __Expression__, __Out_Type__);
    }
    if (__Expression__->__Kind__ == __Ast_Expression_Conversion__)
    {
        return __Body_Infer_Conversion__(__Context__, __Expression__, __Out_Type__);
    }
    return __Body_Fail__(__Context__,
                         __E0404_Statement_Typecheck_Not_Implemented__,
                         __Expression__->__Header__.__Span__);
}
