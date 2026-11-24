/* Checks built-in calls and their effects. */

#include "semantic/body_internal.h"
#include "semantic/generic.h"
#include "kernel/memory/memory.h"
#include "kernel/name/name.h"
#include "frontend/identifier_identity.h"

#include <string.h>

/* Returns the body builtin identity. */
static __Name_Builtin_Function__ __Body_Builtin_Identity__(__Semantic_Body_Context__ *__Context__,
                                                           __Ast_Expression__ *__Expression__)
{
    /* References the function. */
    __Ast_Lvalue__ *__Function__;

    if (__Expression__ == NULL || __Expression__->__Kind__ != __Ast_Expression_Call__)
    {
        return __Name_Builtin_None__;
    }
    __Function__ = __Expression__->__As__.__Call__.__Function__;
    if (__Function__ == NULL || __Function__->__Kind__ != __Ast_Lvalue_Base__ ||
        __Function__->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__ ||
        __Body_Find_Local__(__Context__, __Function__) != NULL)
    {
        return __Name_Builtin_None__;
    }
    {
        /* Stores the source name. */
        __Text_Slice__ __Source_Name__ = __Function__->__As__.__Base__.__As__.__Identifier__;
        /* Stores the resolved name. */
        __Text_Slice__ __Resolved_Name__ = __Source_Name__;
        /* Stores the direct. */
        __Name_Builtin_Function__ __Direct__ = __Name_Find_Builtin_Function__(__Source_Name__);
        /* References the unit. */
        const __Program_Unit__ *__Unit__ =
            (__Context__->__Function__ == NULL) ? NULL : __Context__->__Function__->__Unit__;

        if (__Direct__ != __Name_Builtin_None__ && __Name_Builtin_Is_Direct_Source__(__Direct__))
        {
            return __Direct__;
        }
        if (__Unit__ == NULL ||
            !__Name_Resolve_Module_Alias_Name__(__Unit__, __Source_Name__, &__Resolved_Name__) ||
            __Identifier_Identity_Equals__(__Resolved_Name__, __Source_Name__))
        {
            return __Name_Builtin_None__;
        }
        return __Name_Find_Builtin_Function__(__Resolved_Name__);
    }
}

/* Returns the body builtin length. */
static int __Body_Builtin_Length__(__Semantic_Body_Context__ *__Context__,
                                   __Ast_Expression__ *__Expression__,
                                   __Ast_Type__ **__Out_Type__)
{
    /* References the argument type. */
    __Ast_Type__ *__Argument_Type__ = NULL;
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;

    if (__Expression__->__As__.__Call__.__Argument_Count__ != 1U)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    if (!__Body_Infer_Expression__(
            __Context__, __Expression__->__As__.__Call__.__Arguments__[0], &__Argument_Type__) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Argument_Type__, &__Resolved__))
    {
        return 0;
    }
    if (__Resolved__.__Kind__ != __Resolved_Type_String__ &&
        __Resolved__.__Kind__ != __Resolved_Type_Vector__)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    *__Out_Type__ = &__Body_Builtin_Int_Type__;
    return 1;
}

/* Appends the body builtin. */
static int __Body_Builtin_Append__(__Semantic_Body_Context__ *__Context__,
                                   __Ast_Expression__ *__Expression__,
                                   __Ast_Type__ **__Out_Type__)
{
    /* References the vector expression. */
    __Ast_Expression__ *__Vector_Expression__;
    /* References the vector lvalue. */
    __Ast_Lvalue__ *__Vector_Lvalue__;
    /* References the vector type. */
    __Ast_Type__ *__Vector_Type__ = NULL;
    /* References the vector local. */
    __Semantic_Local__ *__Vector_Local__ = NULL;
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;
    /* References the value expression. */
    __Ast_Expression__ *__Value_Expression__;

    if (__Expression__->__As__.__Call__.__Argument_Count__ != 2U)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }

    /* Avoid reapplying ownership effects when a call is inferred twice. */
    if (__Expression__->__Semantic_Effects_Applied__)
    {
        *__Out_Type__ = &__Body_Builtin_Void_Type__;
        return 1;
    }

    __Vector_Expression__ = __Expression__->__As__.__Call__.__Arguments__[0];
    if (__Vector_Expression__ == NULL)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    if (__Vector_Expression__->__Kind__ == __Ast_Expression_Atom__ &&
        __Vector_Expression__->__As__.__Atom__.__Kind__ == __Ast_Atom_Lvalue__)
    {
        __Vector_Lvalue__ = __Vector_Expression__->__As__.__Atom__.__As__.__Lvalue__;
        if (!__Body_Check_Assignable__(
                __Context__, __Vector_Lvalue__, &__Vector_Type__, &__Vector_Local__) ||
            !__Type_Resolve__(__Context__->__Semantic__, __Vector_Type__, &__Resolved__))
        {
            return 0;
        }
    }
    else if (__Vector_Expression__->__Kind__ == __Ast_Expression_Unary__ &&
             __Vector_Expression__->__As__.__Unary__.__Operation__ == __Unary_Dereference__)
    {
        __Ast_Type__ *__Reference_Type__ = NULL;
        __Resolved_Type__ __Reference_Resolved__;
        if (!__Body_Infer_Expression__(__Context__,
                                      __Vector_Expression__->__As__.__Unary__.__Operand__,
                                      &__Reference_Type__) ||
            !__Type_Resolve__(__Context__->__Semantic__,
                              __Reference_Type__,
                              &__Reference_Resolved__))
        {
            return 0;
        }
        if (__Reference_Resolved__.__Kind__ != __Resolved_Type_Reference__ ||
            !__Reference_Resolved__.__Reference_Mutable__ ||
            __Reference_Resolved__.__Inner__ == NULL)
        {
            return __Body_Fail__(
                __Context__, __E0400_Mismatched_Types__, __Vector_Expression__->__Header__.__Span__);
        }
        __Vector_Type__ = __Reference_Resolved__.__Inner__;
        if (!__Type_Resolve__(__Context__->__Semantic__, __Vector_Type__, &__Resolved__))
        {
            return 0;
        }
    }
    else
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    if (__Resolved__.__Kind__ != __Resolved_Type_Vector__)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }

    __Value_Expression__ = __Expression__->__As__.__Call__.__Arguments__[1];
    if (!__Body_Check_Expression_Compatible__(__Context__,
                                              __Resolved__.__Inner__,
                                              __Value_Expression__,
                                              __Value_Expression__->__Header__.__Span__))
    {
        return 0;
    }
    if (!__Expression__->__Semantic_Effects_Applied__)
    {
        if (__Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Resolved__.__Inner__) &&
            !__Safety_Type_Is_Reference__(__Resolved__.__Inner__, NULL) &&
            !__Body_Safety_Move_Expression__(__Context__, __Value_Expression__, NULL))
        {
            return 0;
        }

        if (__Vector_Local__ != NULL && __Vector_Local__->__Has_Known_Length__)
        {
            if (__Vector_Local__->__Known_Length__ == SIZE_MAX)
            {
                __Vector_Local__->__Has_Known_Length__ = 0;
            }
            else
            {
                ++__Vector_Local__->__Known_Length__;
            }
        }
    }
    *__Out_Type__ = &__Body_Builtin_Void_Type__;
    return 1;
}

/* Checks the vector operand of a primitive that writes it (pop, truncate,
 * reserve_exact): a mutable place, or `*r` through a `&متغير`. */
static int __Body_Builtin_Vector_Place__(__Semantic_Body_Context__ *__Context__,
                                         __Ast_Expression__ *__Call__,
                                         __Resolved_Type__ *__Out_Resolved__,
                                         __Semantic_Local__ **__Out_Local__)
{
    __Ast_Expression__ *__Vector_Expression__ = __Call__->__As__.__Call__.__Arguments__[0];
    __Ast_Type__ *__Vector_Type__ = NULL;
    *__Out_Local__ = NULL;
    if (__Vector_Expression__ == NULL)
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Call__->__Header__.__Span__);
    }
    if (__Vector_Expression__->__Kind__ == __Ast_Expression_Atom__ &&
        __Vector_Expression__->__As__.__Atom__.__Kind__ == __Ast_Atom_Lvalue__)
    {
        if (!__Body_Check_Assignable__(__Context__,
                                       __Vector_Expression__->__As__.__Atom__.__As__.__Lvalue__,
                                       &__Vector_Type__, __Out_Local__))
        {
            return 0;
        }
    }
    else if (__Vector_Expression__->__Kind__ == __Ast_Expression_Unary__ &&
             __Vector_Expression__->__As__.__Unary__.__Operation__ == __Unary_Dereference__)
    {
        __Ast_Type__ *__Reference_Type__ = NULL;
        __Resolved_Type__ __Reference_Resolved__;
        if (!__Body_Infer_Expression__(__Context__, __Vector_Expression__->__As__.__Unary__.__Operand__,
                                       &__Reference_Type__) ||
            !__Type_Resolve__(__Context__->__Semantic__, __Reference_Type__, &__Reference_Resolved__))
        {
            return 0;
        }
        if (__Reference_Resolved__.__Kind__ != __Resolved_Type_Reference__ ||
            !__Reference_Resolved__.__Reference_Mutable__ || __Reference_Resolved__.__Inner__ == NULL)
        {
            return __Body_Fail__(__Context__, __E0400_Mismatched_Types__,
                                 __Vector_Expression__->__Header__.__Span__);
        }
        __Vector_Type__ = __Reference_Resolved__.__Inner__;
    }
    else
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Call__->__Header__.__Span__);
    }
    if (!__Type_Resolve__(__Context__->__Semantic__, __Vector_Type__, __Out_Resolved__))
    {
        return 0;
    }
    if (__Out_Resolved__->__Kind__ != __Resolved_Type_Vector__ || __Out_Resolved__->__Inner__ == NULL)
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Call__->__Header__.__Span__);
    }
    if (*__Out_Local__ != NULL)
    {
        /* The length is no longer known statically. */
        (*__Out_Local__)->__Has_Known_Length__ = 0;
    }
    return 1;
}

/* Checks an integer operand of a vector primitive. */
static int __Body_Builtin_Integer_Operand__(__Semantic_Body_Context__ *__Context__,
                                            __Ast_Expression__ *__Operand__)
{
    __Ast_Type__ *__Type__ = NULL;
    int64_t __Literal__ = 0;
    if (__Semantic_Integer_Literal_Expression_Value__(__Operand__, &__Literal__))
    {
        return __Body_Check_Expression_Compatible__(__Context__, &__Body_Builtin_Uint_Type__, __Operand__,
                                                    __Operand__->__Header__.__Span__);
    }
    if (!__Body_Infer_Expression__(__Context__, __Operand__, &__Type__))
    {
        return 0;
    }
    if (!__Body_Is_Integer_Like__(__Context__, __Type__, NULL))
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Operand__->__Header__.__Span__);
    }
    return 1;
}

/* Persists TYPE for the program's lifetime (the emitter reads it). */
static __Ast_Type__ *__Body_Persist_Type__(__Semantic_Body_Context__ *__Context__, __Ast_Type__ *__Type__)
{
    return __Semantic_Generic_Substitute_Type__(__Context__->__Semantic__, __Type__, NULL, NULL, 0U, NULL);
}

/* capacity / reserve_exact / pop / truncate. */
static int __Body_Builtin_Vector_Primitive__(__Semantic_Body_Context__ *__Context__,
                                             __Ast_Expression__ *__Expression__,
                                             __Name_Builtin_Function__ __Builtin__,
                                             __Ast_Type__ **__Out_Type__)
{
    __Resolved_Type__ __Resolved__;
    __Semantic_Local__ *__Local__ = NULL;
    size_t __Arguments__ = __Builtin__ == __Name_Builtin_Capacity__ || __Builtin__ == __Name_Builtin_Pop__
                               ? 1U
                               : 2U;
    if (__Expression__->__As__.__Call__.__Argument_Count__ != __Arguments__)
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    if (__Builtin__ == __Name_Builtin_Capacity__)
    {
        __Ast_Type__ *__Argument_Type__ = NULL;
        if (!__Body_Infer_Expression__(__Context__, __Expression__->__As__.__Call__.__Arguments__[0],
                                       &__Argument_Type__) ||
            !__Type_Resolve__(__Context__->__Semantic__, __Argument_Type__, &__Resolved__))
        {
            return 0;
        }
        if (__Resolved__.__Kind__ != __Resolved_Type_Vector__ &&
            __Resolved__.__Kind__ != __Resolved_Type_String__)
        {
            return __Body_Fail__(__Context__, __E0400_Mismatched_Types__,
                                 __Expression__->__Header__.__Span__);
        }
        *__Out_Type__ = &__Body_Builtin_Int_Type__;
        return 1;
    }
    if (!__Body_Builtin_Vector_Place__(__Context__, __Expression__, &__Resolved__, &__Local__))
    {
        return 0;
    }
    if (__Builtin__ != __Name_Builtin_Pop__)
    {
        if (!__Body_Builtin_Integer_Operand__(__Context__, __Expression__->__As__.__Call__.__Arguments__[1]))
        {
            return 0;
        }
        *__Out_Type__ = &__Body_Builtin_Void_Type__;
        return 1;
    }
    {
        /* pop answers an option of the element Type. */
        __Ast_Type__ __Option__;
        __Ast_Type__ *__Persisted__;
        memset(&__Option__, 0, sizeof(__Option__));
        __Option__.__Kind__ = __Ast_Type_Option__;
        __Option__.__As__.__Inner__ = __Resolved__.__Inner__;
        __Persisted__ = __Body_Persist_Type__(__Context__, &__Option__);
        if (__Persisted__ == NULL)
        {
            return __Body_Fail__(__Context__, __E1100_Internal_Context_Error__,
                                 __Expression__->__Header__.__Span__);
        }
        __Expression__->__Contextual_Type__ = __Persisted__;
        *__Out_Type__ = __Persisted__;
        return 1;
    }
}

/* extend_from (compiler/frontend/types/primitive_call.sn, إلحاق_نطاق_متجه):
 * the first operand is a mutable vector place of an integer element Type;
 * the second is a reference to a vector of that element Type, or to a text
 * when the element Type is ط8; the start and the count are integers
 * (طبيعي). The verified source reference Type is kept on its operand for
 * the emitter. */
static int __Body_Builtin_Extend_From__(__Semantic_Body_Context__ *__Context__,
                                        __Ast_Expression__ *__Expression__,
                                        __Ast_Type__ **__Out_Type__)
{
    __Ast_Expression__ **__Arguments__ = __Expression__->__As__.__Call__.__Arguments__;
    __Resolved_Type__ __Resolved__;
    __Resolved_Type__ __Element__;
    __Resolved_Type__ __Source__;
    __Resolved_Type__ __Source_Sequence__;
    __Semantic_Local__ *__Local__ = NULL;
    __Ast_Type__ *__Source_Type__ = NULL;
    __Ast_Type__ *__Persisted__;
    int __Matches__;
    if (__Expression__->__As__.__Call__.__Argument_Count__ != 4U)
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    if (!__Body_Builtin_Vector_Place__(__Context__, __Expression__, &__Resolved__, &__Local__))
    {
        return 0;
    }
    if (!__Type_Resolve__(__Context__->__Semantic__, __Resolved__.__Inner__, &__Element__) ||
        (__Element__.__Kind__ != __Resolved_Type_Signed_Integer__ &&
         __Element__.__Kind__ != __Resolved_Type_Unsigned_Integer__))
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Arguments__[0]->__Header__.__Span__);
    }
    if (!__Body_Infer_Expression__(__Context__, __Arguments__[1], &__Source_Type__) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Source_Type__, &__Source__))
    {
        return 0;
    }
    __Matches__ = __Source__.__Kind__ == __Resolved_Type_Reference__ && __Source__.__Inner__ != NULL &&
                  __Type_Resolve__(__Context__->__Semantic__, __Source__.__Inner__, &__Source_Sequence__) &&
                  ((__Source_Sequence__.__Kind__ == __Resolved_Type_Vector__ &&
                    __Source_Sequence__.__Inner__ != NULL &&
                    __Type_Compatible__(__Context__->__Semantic__, __Source_Sequence__.__Inner__,
                                        __Resolved__.__Inner__) &&
                    __Type_Compatible__(__Context__->__Semantic__, __Resolved__.__Inner__,
                                        __Source_Sequence__.__Inner__)) ||
                   (__Source_Sequence__.__Kind__ == __Resolved_Type_String__ &&
                    __Element__.__Kind__ == __Resolved_Type_Unsigned_Integer__ && __Element__.__Bits__ == 8U));
    if (!__Matches__)
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Arguments__[1]->__Header__.__Span__);
    }
    __Persisted__ = __Body_Persist_Type__(__Context__, __Source_Type__);
    if (__Persisted__ == NULL)
    {
        return __Body_Fail__(__Context__, __E1100_Internal_Context_Error__, __Expression__->__Header__.__Span__);
    }
    __Arguments__[1]->__Contextual_Type__ = __Persisted__;
    if (!__Body_Builtin_Integer_Operand__(__Context__, __Arguments__[2]) ||
        !__Body_Builtin_Integer_Operand__(__Context__, __Arguments__[3]))
    {
        return 0;
    }
    *__Out_Type__ = &__Body_Builtin_Void_Type__;
    return 1;
}

/* The wrapping arithmetic primitives: two operands of one integer Type. */
static int __Body_Builtin_Wrapping__(__Semantic_Body_Context__ *__Context__,
                                     __Ast_Expression__ *__Expression__,
                                     __Ast_Type__ **__Out_Type__)
{
    __Ast_Expression__ *__Left__;
    __Ast_Expression__ *__Right__;
    __Ast_Type__ *__Type__ = NULL;
    __Ast_Type__ *__Persisted__;
    int64_t __Literal__ = 0;
    if (__Expression__->__As__.__Call__.__Argument_Count__ != 2U)
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    __Left__ = __Expression__->__As__.__Call__.__Arguments__[0];
    __Right__ = __Expression__->__As__.__Call__.__Arguments__[1];
    if (__Semantic_Integer_Literal_Expression_Value__(__Left__, &__Literal__) &&
        !__Semantic_Integer_Literal_Expression_Value__(__Right__, &__Literal__))
    {
        __Ast_Expression__ *__Swap__ = __Left__;
        __Left__ = __Right__;
        __Right__ = __Swap__;
    }
    if (!__Body_Infer_Expression__(__Context__, __Left__, &__Type__) ||
        !__Body_Is_Integer_Like__(__Context__, __Type__, NULL) ||
        !__Body_Check_Expression_Compatible__(__Context__, __Type__, __Right__,
                                              __Right__->__Header__.__Span__))
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    __Persisted__ = __Body_Persist_Type__(__Context__, __Type__);
    if (__Persisted__ == NULL)
    {
        return __Body_Fail__(__Context__, __E1100_Internal_Context_Error__, __Expression__->__Header__.__Span__);
    }
    __Expression__->__Contextual_Type__ = __Persisted__;
    *__Out_Type__ = __Persisted__;
    return 1;
}

/* Swaps two mutable places without creating an intermediate moved-out state. */
static int __Body_Builtin_Swap__(__Semantic_Body_Context__ *__Context__,
                                 __Ast_Expression__ *__Expression__,
                                 __Ast_Type__ **__Out_Type__)
{
    __Ast_Expression__ *__Left_Expression__;
    __Ast_Expression__ *__Right_Expression__;
    __Ast_Lvalue__ *__Left_Lvalue__;
    __Ast_Lvalue__ *__Right_Lvalue__;
    __Ast_Type__ *__Left_Type__ = NULL;
    __Ast_Type__ *__Right_Type__ = NULL;
    __Semantic_Local__ *__Left_Local__ = NULL;
    __Semantic_Local__ *__Right_Local__ = NULL;

    if (__Expression__->__As__.__Call__.__Argument_Count__ != 2U)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    __Left_Expression__ = __Expression__->__As__.__Call__.__Arguments__[0];
    __Right_Expression__ = __Expression__->__As__.__Call__.__Arguments__[1];
    if (__Left_Expression__ == NULL || __Right_Expression__ == NULL ||
        __Left_Expression__->__Kind__ != __Ast_Expression_Atom__ ||
        __Right_Expression__->__Kind__ != __Ast_Expression_Atom__ ||
        __Left_Expression__->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
        __Right_Expression__->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
    }
    __Left_Lvalue__ = __Left_Expression__->__As__.__Atom__.__As__.__Lvalue__;
    __Right_Lvalue__ = __Right_Expression__->__As__.__Atom__.__As__.__Lvalue__;
    if (__Left_Lvalue__ == NULL || __Right_Lvalue__ == NULL ||
        !__Body_Infer_Expression__(__Context__, __Left_Expression__, &__Left_Type__) ||
        !__Body_Infer_Expression__(__Context__, __Right_Expression__, &__Right_Type__) ||
        !__Body_Check_Assignable__(__Context__, __Left_Lvalue__, &__Left_Type__, &__Left_Local__) ||
        !__Body_Check_Assignable__(__Context__, __Right_Lvalue__, &__Right_Type__, &__Right_Local__) ||
        !__Type_Compatible__(__Context__->__Semantic__, __Left_Type__, __Right_Type__) ||
        !__Type_Compatible__(__Context__->__Semantic__, __Right_Type__, __Left_Type__))
    {
        return 0;
    }
    (void)__Left_Local__;
    (void)__Right_Local__;
    *__Out_Type__ = &__Body_Builtin_Void_Type__;
    __Expression__->__Semantic_Effects_Applied__ = 1;
    return 1;
}

/* Consumes an owned local once, using Stage0's existing safety facts.
 * Runtime disposal is intentionally not claimed without the canonical provider. */
static int __Body_Builtin_Release__(__Semantic_Body_Context__ *context,
                                    __Ast_Expression__ *expression,
                                    __Ast_Type__ **out_type)
{
    __Ast_Expression__ *argument;
    __Ast_Lvalue__ *lvalue;
    __Ast_Type__ *type = NULL;
    __Semantic_Local__ *local;
    if (expression->__Semantic_Effects_Applied__)
    {
        *out_type = &__Body_Builtin_Void_Type__;
        return 1;
    }
    if (expression->__As__.__Call__.__Argument_Count__ != 1U ||
        (argument = expression->__As__.__Call__.__Arguments__[0]) == NULL ||
        argument->__Kind__ != __Ast_Expression_Atom__ ||
        argument->__As__.__Atom__.__Kind__ != __Ast_Atom_Lvalue__ ||
        (lvalue = argument->__As__.__Atom__.__As__.__Lvalue__) == NULL ||
        lvalue->__Kind__ != __Ast_Lvalue_Base__ ||
        (local = __Body_Find_Local__(context, lvalue)) == NULL ||
        !__Body_Infer_Expression__(context, argument, &type) ||
        !__Memory_Type_Is_Owned__(context->__Semantic__, type))
        return __Body_Fail__(context, __E0400_Mismatched_Types__,
                             expression->__Header__.__Span__);
    if (!expression->__Semantic_Effects_Applied__)
    {
        if (!__Body_Safety_Move_Expression__(context, argument, NULL))
            return 0;
        expression->__Semantic_Effects_Applied__ = 1;
    }
    (void)local;
    *out_type = &__Body_Builtin_Void_Type__;
    return 1;
}

/* Requires the body builtin argument count. */
static int __Body_Builtin_Require_Argument_Count__(__Semantic_Body_Context__ *__Context__,
                                                   __Ast_Expression__ *__Expression__,
                                                   size_t __Expected__)
{
    if (__Expression__->__As__.__Call__.__Argument_Count__ == __Expected__)
    {
        return 1;
    }
    return __Body_Fail__(
        __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
}

/* Checks a memory-ordering operand of an atomic primitive or the fence: a u32
 * code (0 Relaxed, 1 Release, 2 Acquire, 3 AcqRel, 4 SeqCst). */
static int __Body_Builtin_Ordering_Operand__(__Semantic_Body_Context__ *__Context__,
                                             __Ast_Expression__ *__Operand__)
{
    return __Body_Check_Expression_Compatible__(__Context__, &__Body_Builtin_U32_Type__, __Operand__,
                                                __Operand__->__Header__.__Span__);
}

/* The atomic primitives (compiler/frontend/types/primitive_call.sn,
 * هل_مرجع_ذري_لبدائي): the first operand is a reference (mutable or not) to a
 * 32- or 64-bit integer, عدد or طبيعي; the value operands have that integer's
 * Type and the trailing operands are u32 ordering codes. Load, swap,
 * compare-and-swap and the fetch operations answer the referenced Type (the
 * old value); store answers nothing. The fence takes one ordering. The
 * verified reference Type is kept on the first operand and the result Type on
 * the call, both persisted for the emitter. */
static int __Body_Builtin_Atomic__(__Semantic_Body_Context__ *__Context__,
                                   __Ast_Expression__ *__Expression__,
                                   __Name_Builtin_Function__ __Builtin__,
                                   __Ast_Type__ **__Out_Type__)
{
    __Ast_Expression__ **__Arguments__ = __Expression__->__As__.__Call__.__Arguments__;
    __Ast_Type__ *__Reference_Type__ = NULL;
    __Ast_Type__ *__Persisted_Reference__;
    __Ast_Type__ *__Cell_Type__;
    __Resolved_Type__ __Reference__;
    __Resolved_Type__ __Cell__;
    size_t __Count__;
    size_t __Values__;
    size_t __Index__;
    if (__Builtin__ == __Name_Builtin_Memory_Fence__)
    {
        if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 1U) ||
            !__Body_Builtin_Ordering_Operand__(__Context__, __Arguments__[0]))
            return 0;
        __Expression__->__Contextual_Type__ = &__Body_Builtin_Void_Type__;
        *__Out_Type__ = &__Body_Builtin_Void_Type__;
        return 1;
    }
    switch (__Builtin__)
    {
        case __Name_Builtin_Atomic_Load__:
            __Count__ = 2U;
            __Values__ = 0U;
            break;
        case __Name_Builtin_Atomic_Compare_Swap__:
            __Count__ = 5U;
            __Values__ = 2U;
            break;
        default:
            __Count__ = 3U;
            __Values__ = 1U;
            break;
    }
    if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, __Count__))
        return 0;
    if (!__Body_Infer_Expression__(__Context__, __Arguments__[0], &__Reference_Type__) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Reference_Type__, &__Reference__))
        return 0;
    if (__Reference__.__Kind__ != __Resolved_Type_Reference__ || __Reference__.__Inner__ == NULL ||
        !__Type_Resolve__(__Context__->__Semantic__, __Reference__.__Inner__, &__Cell__) ||
        (__Cell__.__Kind__ != __Resolved_Type_Signed_Integer__ &&
         __Cell__.__Kind__ != __Resolved_Type_Unsigned_Integer__) ||
        (__Cell__.__Bits__ != 32U && __Cell__.__Bits__ != 64U))
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__,
                             __Arguments__[0]->__Header__.__Span__);
    __Persisted_Reference__ = __Body_Persist_Type__(__Context__, __Reference_Type__);
    if (__Persisted_Reference__ == NULL)
        return __Body_Fail__(__Context__, __E1100_Internal_Context_Error__,
                             __Expression__->__Header__.__Span__);
    __Arguments__[0]->__Contextual_Type__ = __Persisted_Reference__;
    if (!__Type_Resolve__(__Context__->__Semantic__, __Persisted_Reference__, &__Reference__) ||
        __Reference__.__Inner__ == NULL)
        return __Body_Fail__(__Context__, __E1100_Internal_Context_Error__,
                             __Expression__->__Header__.__Span__);
    __Cell_Type__ = __Reference__.__Inner__;
    for (__Index__ = 1U; __Index__ < __Count__; ++__Index__)
    {
        if (__Index__ <= __Values__)
        {
            if (!__Body_Check_Expression_Compatible__(__Context__, __Cell_Type__, __Arguments__[__Index__],
                                                      __Arguments__[__Index__]->__Header__.__Span__))
                return 0;
        }
        else if (!__Body_Builtin_Ordering_Operand__(__Context__, __Arguments__[__Index__]))
            return 0;
    }
    if (__Builtin__ == __Name_Builtin_Atomic_Store__)
    {
        __Expression__->__Contextual_Type__ = &__Body_Builtin_Void_Type__;
        *__Out_Type__ = &__Body_Builtin_Void_Type__;
        return 1;
    }
    __Expression__->__Contextual_Type__ = __Cell_Type__;
    *__Out_Type__ = __Cell_Type__;
    return 1;
}

/* Flat machine memory (compiler/frontend/types/primitive_call.sn): every
 * operand is a u64 (the address, and the stored word). Read answers the u64
 * word; write answers nothing. */
static int __Body_Builtin_Machine_Memory__(__Semantic_Body_Context__ *__Context__,
                                           __Ast_Expression__ *__Expression__,
                                           __Name_Builtin_Function__ __Builtin__,
                                           __Ast_Type__ **__Out_Type__)
{
    size_t __Count__ = __Builtin__ == __Name_Builtin_Machine_Memory_Read__ ? 1U : 2U;
    size_t __Index__;
    if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, __Count__))
        return 0;
    for (__Index__ = 0U; __Index__ < __Count__; ++__Index__)
    {
        __Ast_Expression__ *__Argument__ = __Expression__->__As__.__Call__.__Arguments__[__Index__];
        if (!__Body_Check_Expression_Compatible__(__Context__, &__Body_Builtin_U64_Type__, __Argument__,
                                                  __Argument__->__Header__.__Span__))
            return 0;
    }
    *__Out_Type__ = __Builtin__ == __Name_Builtin_Machine_Memory_Read__ ? &__Body_Builtin_U64_Type__
                                                                       : &__Body_Builtin_Void_Type__;
    __Expression__->__Contextual_Type__ = *__Out_Type__;
    return 1;
}

/* WebAssembly linear memory (compiler/frontend/types/primitive_call.sn): every
 * operand is a u32. Grow, read and heap base answer a u32; write and copy
 * answer nothing. Only wasm32 lowers them (checked by the emitter). */
static int __Body_Builtin_Linear_Memory__(__Semantic_Body_Context__ *__Context__,
                                          __Ast_Expression__ *__Expression__,
                                          __Name_Builtin_Function__ __Builtin__,
                                          __Ast_Type__ **__Out_Type__)
{
    size_t __Count__;
    size_t __Index__;
    switch (__Builtin__)
    {
        case __Name_Builtin_Linear_Heap_Base__:
            __Count__ = 0U;
            break;
        case __Name_Builtin_Linear_Memory_Write__:
            __Count__ = 2U;
            break;
        case __Name_Builtin_Linear_Memory_Copy__:
            __Count__ = 3U;
            break;
        default:
            __Count__ = 1U;
            break;
    }
    if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, __Count__))
        return 0;
    for (__Index__ = 0U; __Index__ < __Count__; ++__Index__)
    {
        __Ast_Expression__ *__Argument__ = __Expression__->__As__.__Call__.__Arguments__[__Index__];
        if (!__Body_Check_Expression_Compatible__(__Context__, &__Body_Builtin_U32_Type__, __Argument__,
                                                  __Argument__->__Header__.__Span__))
            return 0;
    }
    *__Out_Type__ = (__Builtin__ == __Name_Builtin_Linear_Memory_Write__ ||
                     __Builtin__ == __Name_Builtin_Linear_Memory_Copy__)
                        ? &__Body_Builtin_Void_Type__
                        : &__Body_Builtin_U32_Type__;
    __Expression__->__Contextual_Type__ = *__Out_Type__;
    return 1;
}

/* The number of operands of a float operation. */
static size_t __Body_Builtin_Float_Operand_Count__(__Name_Builtin_Function__ __Builtin__)
{
    switch (__Builtin__)
    {
        case __Name_Builtin_Float_Minimum__:
        case __Name_Builtin_Float_Maximum__:
        case __Name_Builtin_Float_Copy_Sign__:
            return 2U;
        case __Name_Builtin_Float_Fused_Multiply_Add__:
            return 3U;
        default:
            return 1U;
    }
}

/* The float operations (compiler/frontend/types/primitive_call.sn): the first
 * operand has a float Type (f32 or f64) and every other operand has that
 * Type, which is also the result Type, persisted on the call. */
static int __Body_Builtin_Float_Operation__(__Semantic_Body_Context__ *__Context__,
                                            __Ast_Expression__ *__Expression__,
                                            __Name_Builtin_Function__ __Builtin__,
                                            __Ast_Type__ **__Out_Type__)
{
    __Ast_Expression__ **__Arguments__ = __Expression__->__As__.__Call__.__Arguments__;
    size_t __Count__ = __Body_Builtin_Float_Operand_Count__(__Builtin__);
    __Ast_Type__ *__Type__ = NULL;
    __Ast_Type__ *__Persisted__;
    __Resolved_Type__ __Resolved__;
    size_t __Index__;
    if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, __Count__))
        return 0;
    if (!__Body_Infer_Expression__(__Context__, __Arguments__[0], &__Type__) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Type__, &__Resolved__))
        return 0;
    if (__Resolved__.__Kind__ != __Resolved_Type_Float__ ||
        (__Resolved__.__Bits__ != 32U && __Resolved__.__Bits__ != 64U))
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, __Arguments__[0]->__Header__.__Span__);
    __Persisted__ = __Body_Persist_Type__(__Context__, __Type__);
    if (__Persisted__ == NULL)
        return __Body_Fail__(__Context__, __E1100_Internal_Context_Error__,
                             __Expression__->__Header__.__Span__);
    for (__Index__ = 1U; __Index__ < __Count__; ++__Index__)
    {
        if (!__Body_Check_Expression_Compatible__(__Context__, __Persisted__, __Arguments__[__Index__],
                                                  __Arguments__[__Index__]->__Header__.__Span__))
            return 0;
    }
    __Expression__->__Contextual_Type__ = __Persisted__;
    *__Out_Type__ = __Persisted__;
    return 1;
}

/* Returns the retained compiler builtin type. */
static int __Body_Builtin_Runtime__(__Semantic_Body_Context__ *__Context__,
                                    __Ast_Expression__ *__Expression__,
                                    __Name_Builtin_Function__ __Builtin__,
                                    __Ast_Type__ **__Out_Type__)
{
    /* References the result type. */
    __Ast_Type__ *__Result_Type__ = NULL;

    switch (__Builtin__)
    {
        case __Name_Builtin_F64_Bits__:
        case __Name_Builtin_F64_From_Bits__:
        case __Name_Builtin_F32_Bits__:
        case __Name_Builtin_F32_From_Bits__:
        {
            __Ast_Type__ *argument_type = NULL;
            __Ast_Type__ *required_type;
            __Resolved_Type__ actual;
            __Resolved_Type__ required;
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 1U))
                return 0;
            required_type = (__Builtin__ == __Name_Builtin_F64_Bits__)
                                ? &__Body_Builtin_F64_Type__
                                : (__Builtin__ == __Name_Builtin_F64_From_Bits__)
                                      ? &__Body_Builtin_U64_Type__
                                      : (__Builtin__ == __Name_Builtin_F32_Bits__)
                                            ? &__Body_Builtin_F32_Type__
                                            : &__Body_Builtin_U32_Type__;
            if (!__Body_Infer_Expression__(__Context__,
                                         __Expression__->__As__.__Call__.__Arguments__[0],
                                         &argument_type) ||
                !__Type_Resolve__(__Context__->__Semantic__, argument_type, &actual) ||
                !__Type_Resolve__(__Context__->__Semantic__, required_type, &required) ||
                actual.__Kind__ != required.__Kind__ ||
                actual.__Bits__ != required.__Bits__)
                return __Body_Fail__(__Context__, __E0400_Mismatched_Types__,
                                     __Expression__->__Header__.__Span__);
            __Result_Type__ = (__Builtin__ == __Name_Builtin_F64_Bits__)
                                  ? &__Body_Builtin_U64_Type__
                                  : (__Builtin__ == __Name_Builtin_F64_From_Bits__)
                                        ? &__Body_Builtin_F64_Type__
                                        : (__Builtin__ == __Name_Builtin_F32_Bits__)
                                              ? &__Body_Builtin_U32_Type__
                                              : &__Body_Builtin_F32_Type__;
            break;
        }
        case __Name_Builtin_Host_Architecture__:
        case __Name_Builtin_Host_Platform__:
        case __Name_Builtin_Host_Environment__:
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 0U))
            {
                return 0;
            }
            __Result_Type__ = &__Body_Builtin_Int_Type__;
            break;
        case __Name_Builtin_Raw_Kernel_Call__:
        {
            /* Canonical slot typing (compiler/frontend/types/primitive_call.sn):
             * slot 0 is ط64; slots 1..6 are ط64, a raw pointer, or a reference
             * to a fixed-width scalar. Result is the raw ط64 kernel word. */
            size_t index;
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 7U))
                return 0;
            if (!__Body_Check_Expression_Compatible__(
                    __Context__, &__Body_Builtin_U64_Type__,
                    __Expression__->__As__.__Call__.__Arguments__[0],
                    __Expression__->__Header__.__Span__))
                return 0;
            for (index = 1U; index < 7U; ++index)
            {
                __Ast_Expression__ *argument = __Expression__->__As__.__Call__.__Arguments__[index];
                __Ast_Type__ *argument_type = NULL;
                __Resolved_Type__ resolved;
                __Resolved_Type__ inner;
                int64_t literal = 0;
                if (__Semantic_Integer_Literal_Expression_Value__(argument, &literal) ||
                    __Semantic_Integer_Literal_Expression_Is_Unsigned_Wide__(argument))
                {
                    if (!__Body_Check_Expression_Compatible__(
                            __Context__, &__Body_Builtin_U64_Type__, argument,
                            __Expression__->__Header__.__Span__))
                        return 0;
                    continue;
                }
                if (!__Body_Infer_Expression__(__Context__, argument, &argument_type) ||
                    !__Type_Resolve__(__Context__->__Semantic__, argument_type, &resolved))
                    return 0;
                /* The emitter lowers each word from its verified semantic
                 * type; record it so address-of operands keep their type.
                 * Inferred reference types live in the per-body synthetic
                 * arena, so persist a program-lifetime copy. */
                argument_type = __Semantic_Generic_Substitute_Type__(
                    __Context__->__Semantic__, argument_type, NULL, NULL, 0U, NULL);
                if (argument_type == NULL)
                    return __Body_Fail__(__Context__, __E1100_Internal_Context_Error__,
                                         __Expression__->__Header__.__Span__);
                if ((resolved.__Kind__ == __Resolved_Type_Unsigned_Integer__ &&
                     resolved.__Bits__ == 64U) ||
                    resolved.__Kind__ == __Resolved_Type_Pointer__)
                {
                    argument->__Contextual_Type__ = argument_type;
                    continue;
                }
                if (resolved.__Kind__ == __Resolved_Type_Reference__ && resolved.__Inner__ != NULL &&
                    __Type_Resolve__(__Context__->__Semantic__, resolved.__Inner__, &inner) &&
                    (inner.__Kind__ == __Resolved_Type_Unsigned_Integer__ ||
                     inner.__Kind__ == __Resolved_Type_Signed_Integer__ ||
                     inner.__Kind__ == __Resolved_Type_Float__))
                {
                    argument->__Contextual_Type__ = argument_type;
                    continue;
                }
                return __Body_Fail__(__Context__, __E0400_Mismatched_Types__,
                                     __Expression__->__Header__.__Span__);
            }
            __Result_Type__ = &__Body_Builtin_U64_Type__;
            break;
        }
        case __Name_Builtin_C_Address_Call__:
        {
            /* Canonical slot typing (compiler/frontend/types/primitive_call.sn):
             * slot 0 is the result witness (u64 or f64, the result Type);
             * slots 1..9 are u64 (the address, eight integer words); slots
             * 10..17 are f64. */
            size_t index;
            __Ast_Type__ *witness_type = NULL;
            __Resolved_Type__ witness;
            if (!__Body_Builtin_Require_Argument_Count__(__Context__, __Expression__, 18U))
                return 0;
            if (!__Body_Infer_Expression__(__Context__, __Expression__->__As__.__Call__.__Arguments__[0],
                                           &witness_type) ||
                !__Type_Resolve__(__Context__->__Semantic__, witness_type, &witness))
                return 0;
            if (witness.__Kind__ == __Resolved_Type_Float__ && witness.__Bits__ == 64U)
                __Result_Type__ = &__Body_Builtin_F64_Type__;
            else if (witness.__Kind__ == __Resolved_Type_Unsigned_Integer__ && witness.__Bits__ == 64U)
                __Result_Type__ = &__Body_Builtin_U64_Type__;
            else
                return __Body_Fail__(__Context__, __E0400_Mismatched_Types__,
                                     __Expression__->__Header__.__Span__);
            for (index = 1U; index < 18U; ++index)
            {
                if (!__Body_Check_Expression_Compatible__(
                        __Context__,
                        index < 10U ? &__Body_Builtin_U64_Type__ : &__Body_Builtin_F64_Type__,
                        __Expression__->__As__.__Call__.__Arguments__[index],
                        __Expression__->__Header__.__Span__))
                    return 0;
            }
            __Expression__->__Contextual_Type__ = __Result_Type__;
            break;
        }
        case __Name_Builtin_None__:
        case __Name_Builtin_Length__:
        case __Name_Builtin_Append__:
        case __Name_Builtin_Swap__:
        case __Name_Builtin_Release__:
        case __Name_Builtin_Capacity__:
        case __Name_Builtin_Reserve_Exact__:
        case __Name_Builtin_Pop__:
        case __Name_Builtin_Truncate__:
        case __Name_Builtin_Extend_From__:
        case __Name_Builtin_Wrapping_Add__:
        case __Name_Builtin_Wrapping_Subtract__:
        case __Name_Builtin_Wrapping_Multiply__:
        case __Name_Builtin_Wrapping_Shift_Left__:
        case __Name_Builtin_Wrapping_Shift_Right__:
        case __Name_Builtin_Atomic_Load__:
        case __Name_Builtin_Atomic_Store__:
        case __Name_Builtin_Atomic_Swap__:
        case __Name_Builtin_Atomic_Compare_Swap__:
        case __Name_Builtin_Atomic_Fetch_Add__:
        case __Name_Builtin_Atomic_Fetch_And__:
        case __Name_Builtin_Atomic_Fetch_Or__:
        case __Name_Builtin_Atomic_Fetch_Xor__:
        case __Name_Builtin_Memory_Fence__:
        case __Name_Builtin_Machine_Memory_Read__:
        case __Name_Builtin_Machine_Memory_Write__:
        case __Name_Builtin_Linear_Memory_Grow__:
        case __Name_Builtin_Linear_Memory_Read__:
        case __Name_Builtin_Linear_Memory_Write__:
        case __Name_Builtin_Linear_Memory_Copy__:
        case __Name_Builtin_Linear_Heap_Base__:
        case __Name_Builtin_Float_Sqrt__:
        case __Name_Builtin_Float_Abs__:
        case __Name_Builtin_Float_Floor__:
        case __Name_Builtin_Float_Ceil__:
        case __Name_Builtin_Float_Trunc__:
        case __Name_Builtin_Float_Round_Even__:
        case __Name_Builtin_Float_Minimum__:
        case __Name_Builtin_Float_Maximum__:
        case __Name_Builtin_Float_Copy_Sign__:
        case __Name_Builtin_Float_Fused_Multiply_Add__:
            return 0;
    }

    if (__Result_Type__ == NULL)
    {
        return __Body_Fail__(
            __Context__, __E1100_Internal_Context_Error__, __Expression__->__Header__.__Span__);
    }
    *__Out_Type__ = __Result_Type__;
    __Expression__->__Semantic_Effects_Applied__ = 1;
    return 1;
}

/* Infers the body try builtin call. */
int __Body_Try_Infer_Builtin_Call__(__Semantic_Body_Context__ *__Context__,
                                    __Ast_Expression__ *__Expression__,
                                    __Ast_Type__ **__Out_Type__,
                                    int *__Matched__)
{
    /* Stores the builtin. */
    __Name_Builtin_Function__ __Builtin__ = __Body_Builtin_Identity__(__Context__, __Expression__);

    *__Matched__ = __Builtin__ != __Name_Builtin_None__;
    switch (__Builtin__)
    {
        case __Name_Builtin_None__:
            return 1;
        case __Name_Builtin_Length__:
            if (!__Body_Builtin_Length__(__Context__, __Expression__, __Out_Type__))
            {
                return 0;
            }
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
        case __Name_Builtin_Append__:
            if (!__Body_Builtin_Append__(__Context__, __Expression__, __Out_Type__))
            {
                return 0;
            }
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
        case __Name_Builtin_Swap__:
            return __Body_Builtin_Swap__(__Context__, __Expression__, __Out_Type__);
        case __Name_Builtin_Release__:
            return __Body_Builtin_Release__(__Context__, __Expression__, __Out_Type__);
        case __Name_Builtin_F64_Bits__:
        case __Name_Builtin_F64_From_Bits__:
        case __Name_Builtin_F32_Bits__:
        case __Name_Builtin_F32_From_Bits__:
        case __Name_Builtin_Host_Architecture__:
        case __Name_Builtin_Host_Platform__:
        case __Name_Builtin_Host_Environment__:
        case __Name_Builtin_Raw_Kernel_Call__:
        case __Name_Builtin_C_Address_Call__:
            return __Body_Builtin_Runtime__(__Context__, __Expression__, __Builtin__, __Out_Type__);
        case __Name_Builtin_Capacity__:
        case __Name_Builtin_Reserve_Exact__:
        case __Name_Builtin_Pop__:
        case __Name_Builtin_Truncate__:
            if (!__Body_Builtin_Vector_Primitive__(__Context__, __Expression__, __Builtin__, __Out_Type__))
                return 0;
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
        case __Name_Builtin_Extend_From__:
            if (!__Body_Builtin_Extend_From__(__Context__, __Expression__, __Out_Type__))
                return 0;
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
        case __Name_Builtin_Wrapping_Add__:
        case __Name_Builtin_Wrapping_Subtract__:
        case __Name_Builtin_Wrapping_Multiply__:
        case __Name_Builtin_Wrapping_Shift_Left__:
        case __Name_Builtin_Wrapping_Shift_Right__:
            if (!__Body_Builtin_Wrapping__(__Context__, __Expression__, __Out_Type__))
                return 0;
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
        case __Name_Builtin_Atomic_Load__:
        case __Name_Builtin_Atomic_Store__:
        case __Name_Builtin_Atomic_Swap__:
        case __Name_Builtin_Atomic_Compare_Swap__:
        case __Name_Builtin_Atomic_Fetch_Add__:
        case __Name_Builtin_Atomic_Fetch_And__:
        case __Name_Builtin_Atomic_Fetch_Or__:
        case __Name_Builtin_Atomic_Fetch_Xor__:
        case __Name_Builtin_Memory_Fence__:
            if (!__Body_Builtin_Atomic__(__Context__, __Expression__, __Builtin__, __Out_Type__))
                return 0;
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
        case __Name_Builtin_Machine_Memory_Read__:
        case __Name_Builtin_Machine_Memory_Write__:
            if (!__Body_Builtin_Machine_Memory__(__Context__, __Expression__, __Builtin__, __Out_Type__))
                return 0;
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
        case __Name_Builtin_Linear_Memory_Grow__:
        case __Name_Builtin_Linear_Memory_Read__:
        case __Name_Builtin_Linear_Memory_Write__:
        case __Name_Builtin_Linear_Memory_Copy__:
        case __Name_Builtin_Linear_Heap_Base__:
            if (!__Body_Builtin_Linear_Memory__(__Context__, __Expression__, __Builtin__, __Out_Type__))
                return 0;
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
        case __Name_Builtin_Float_Sqrt__:
        case __Name_Builtin_Float_Abs__:
        case __Name_Builtin_Float_Floor__:
        case __Name_Builtin_Float_Ceil__:
        case __Name_Builtin_Float_Trunc__:
        case __Name_Builtin_Float_Round_Even__:
        case __Name_Builtin_Float_Minimum__:
        case __Name_Builtin_Float_Maximum__:
        case __Name_Builtin_Float_Copy_Sign__:
        case __Name_Builtin_Float_Fused_Multiply_Add__:
            if (!__Body_Builtin_Float_Operation__(__Context__, __Expression__, __Builtin__, __Out_Type__))
                return 0;
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
    }
    return 0;
}
