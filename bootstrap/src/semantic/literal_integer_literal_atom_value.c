/* Reads integer values from literal atoms. */

#include "semantic/literal.h"

#include <stdint.h>

/* Returns the semantic integer literal atom value. */
int __Semantic_Integer_Literal_Atom_Value__(const __Ast_Atom__ *__Atom__, int64_t *__Out_Value__)
{
    if (__Atom__ == NULL || __Out_Value__ == NULL || __Atom__->__Kind__ != __Ast_Atom_Literal__ ||
        __Atom__->__As__.__Literal__ == NULL ||
        __Atom__->__As__.__Literal__->__Kind__ != __Ast_Literal_Integer__ ||
        __Atom__->__As__.__Literal__->__As__.__Integer__.__Unsigned_Wide__)
    {
        return 0;
    }
    *__Out_Value__ = __Atom__->__As__.__Literal__->__As__.__Integer__.__Value__;
    return 1;
}

/* Returns whether the atom is an unsigned-wide integer literal (> INT64_MAX). */
int __Semantic_Integer_Literal_Atom_Is_Unsigned_Wide__(const __Ast_Atom__ *__Atom__)
{
    return __Atom__ != NULL && __Atom__->__Kind__ == __Ast_Atom_Literal__ &&
           __Atom__->__As__.__Literal__ != NULL &&
           __Atom__->__As__.__Literal__->__Kind__ == __Ast_Literal_Integer__ &&
           __Atom__->__As__.__Literal__->__As__.__Integer__.__Unsigned_Wide__;
}

/* Returns whether the expression is an unsigned-wide integer literal atom. */
int __Semantic_Integer_Literal_Expression_Is_Unsigned_Wide__(const __Ast_Expression__ *__Expression__)
{
    return __Expression__ != NULL && __Expression__->__Kind__ == __Ast_Expression_Atom__ &&
           __Semantic_Integer_Literal_Atom_Is_Unsigned_Wide__(&__Expression__->__As__.__Atom__);
}
