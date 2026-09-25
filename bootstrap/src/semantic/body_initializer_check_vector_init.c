/* Checks vector initializers. */

#include "semantic/body_internal.h"
#include "kernel/memory/memory.h"

/* Checks the body vector initializer. */
int __Body_Check_Vector_Init__(__Semantic_Body_Context__ *__Context__,
                               __Ast_Statement__ *__Statement__)
{
    /* References the destination type. */
    __Ast_Type__ *__Destination_Type__ = NULL;
    /* References the local. */
    __Semantic_Local__ *__Local__ = NULL;
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;
    /* Tracks the index. */
    size_t __Index__ = 0U;
    if (!__Body_Check_Assignable__(__Context__,
                                   __Statement__->__As__.__Aggregate__.__Destination__,
                                   &__Destination_Type__,
                                   &__Local__) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Destination_Type__, &__Resolved__))
    {
        return 0;
    }
    if (__Resolved__.__Kind__ != __Resolved_Type_Vector__)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Statement__->__Header__.__Span__);
    }
    if (__Resolved__.__Kind__ == __Resolved_Type_Vector__ &&
        __Memory_Type_Contains_View__(__Context__->__Semantic__, __Resolved__.__Inner__))
    {
        /* Stage 0 has no multi-provenance vector lifetime representation. */
        return __Body_Fail__(
            __Context__, __E1303_Reference_Escape__, __Statement__->__Header__.__Span__);
    }
    for (__Index__ = 0U; __Index__ < __Statement__->__As__.__Aggregate__.__Value_Count__;
         ++__Index__)
    {
        if (!__Body_Check_Atom_Compatible__(
                __Context__,
                __Resolved__.__Inner__,
                &__Statement__->__As__.__Aggregate__.__Values__[__Index__],
                __Statement__->__Header__.__Span__) ||
            (__Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Resolved__.__Inner__) &&
             !__Body_Safety_Move_Atom__(__Context__,
                                        &__Statement__->__As__.__Aggregate__.__Values__[__Index__],
                                        __Statement__->__Header__.__Span__)))
        {
            return 0;
        }
    }
    if (__Local__ != NULL)
    {
        __Local__->__Has_Known_Length__ = 1;
        __Local__->__Known_Length__ = __Statement__->__As__.__Aggregate__.__Value_Count__;
        __Safety_Fact_Mark_Initialized_At__(&__Local__->__Safety__,
                                            __Statement__->__Header__.__Span__);
    }
    return 1;
}
