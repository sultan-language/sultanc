/* Validates module-level value declarations.
 *
 * Canonical owner: compiler/frontend/types/semantic_pipeline.sn resolves the
 * declared Type of each قيمة_عليا in its declaring scope and binds it to the
 * symbol; an external initializer contributes no expression. Stage0 applies the
 * same Type resolution. External data is read through ordinary place loads, and
 * Stage0 has no ownership facts for foreign storage, so owned (move/drop) value
 * Types are rejected explicitly instead of being copied silently. */

#include "semantic/check_global_declarations.h"
#include "semantic/check_type_reference.h"
#include "semantic/diagnostic.h"
#include "kernel/memory/memory.h"
#include "kernel/type/type.h"

/* Checks the semantic global declarations. */
int __Semantic_Check_Global_Declarations__(__Semantic_Context__ *__Context__)
{
    /* Tracks the index. */
    size_t __Index__;
    /* Stores the previously active unit. */
    const __Program_Unit__ *__Saved_Unit__ = __Context__->__Active_Unit__;

    for (__Index__ = 0U; __Index__ < __Context__->__Globals__.__Count__; ++__Index__)
    {
        /* References the entry. */
        __Semantic_Global_Entry__ *__Entry__ =
            (__Semantic_Global_Entry__ *)__Vector_At__(&__Context__->__Globals__, __Index__);
        /* Stores the resolved declared Type. */
        __Resolved_Type__ __Resolved__;

        if (__Entry__ == NULL || __Entry__->__Declaration__ == NULL ||
            __Entry__->__Declaration__->__Type__ == NULL)
        {
            __Context__->__Active_Unit__ = __Saved_Unit__;
            return __Semantic_Fail__(
                __Context__, __E1100_Internal_Context_Error__, (__Source_Span__){0});
        }
        __Context__->__Active_Unit__ = __Entry__->__Unit__;
        if (!__Semantic_Check_Type_Reference__(__Context__, __Entry__->__Declaration__->__Type__))
        {
            __Context__->__Active_Unit__ = __Saved_Unit__;
            return 0;
        }
        if (!__Type_Resolve__(__Context__, __Entry__->__Declaration__->__Type__, &__Resolved__))
        {
            __Context__->__Active_Unit__ = __Saved_Unit__;
            return __Semantic_Fail__(__Context__,
                                     __E0409_Unresolved_Type__,
                                     __Entry__->__Declaration__->__Header__.__Span__);
        }
        if (__Resolved__.__Kind__ == __Resolved_Type_Void__ ||
            __Resolved__.__Kind__ == __Resolved_Type_Unknown__)
        {
            __Context__->__Active_Unit__ = __Saved_Unit__;
            return __Semantic_Fail__(__Context__,
                                     __E0400_Mismatched_Types__,
                                     __Entry__->__Declaration__->__Header__.__Span__);
        }
        if (!__Entry__->__Declaration__->__External__ ||
            __Memory_Type_Is_Owned__(__Context__, __Entry__->__Declaration__->__Type__))
        {
            __Context__->__Active_Unit__ = __Saved_Unit__;
            return __Semantic_Fail__(__Context__,
                                     __E0404_Statement_Typecheck_Not_Implemented__,
                                     __Entry__->__Declaration__->__Header__.__Span__);
        }
    }
    __Context__->__Active_Unit__ = __Saved_Unit__;
    return 1;
}
