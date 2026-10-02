/* Checks ordinary function calls. */

#include "semantic/body_internal.h"
#include "kernel/name/name.h"
#include "kernel/type/tagged.h"
#include "semantic/generic.h"
#include "semantic/type_named.h"
#include "frontend/identifier_identity.h"

#include <string.h>

/* Checks the body call arguments. */
static int __Body_Check_Call_Arguments__(__Semantic_Body_Context__ *__Context__,
                                         __Ast_Type__ *const *__Parameters__,
                                         size_t __Parameter_Count__,
                                         __Ast_Expression__ *const *__Arguments__,
                                         size_t __Argument_Count__,
                                         int __Apply_Safety_Effects__)
{
    /* Tracks the index. */
    size_t __Index__;

    if (__Parameter_Count__ != __Argument_Count__)
    {
        return __Body_Fail__(__Context__, __E0400_Mismatched_Types__, (__Source_Span__){0});
    }

    for (__Index__ = 0U; __Index__ < __Argument_Count__; ++__Index__)
    {
        if (!__Body_Check_Expression_Compatible__(__Context__,
                                                  __Parameters__[__Index__],
                                                  __Arguments__[__Index__],
                                                  __Arguments__[__Index__]->__Header__.__Span__))
        {
            return 0;
        }
        if (__Apply_Safety_Effects__)
        {
            if (!__Body_Safety_Check_Ephemeral_Borrow__(__Context__, __Arguments__[__Index__]))
            {
                return 0;
            }
            if (__Safety_Type_Is_Move_Only__(__Context__->__Semantic__,
                                             __Parameters__[__Index__]) &&
                !__Safety_Type_Is_Reference__(__Parameters__[__Index__], NULL) &&
                !__Body_Safety_Move_Expression__(__Context__, __Arguments__[__Index__], NULL))
            {
                return 0;
            }
        }
    }
    return 1;
}

static size_t __Body_Generic_Parameter_Index__(const __Ast_Function__ *__Function__,
                                             __Text_Slice__ __Name__)
{
    size_t __Index__;
    if (__Function__ == NULL)
        return SIZE_MAX;
    for (__Index__ = 0U; __Index__ < __Function__->__Type_Parameter_Count__; ++__Index__)
    {
        if (__Identifier_Identity_Equals__(__Function__->__Type_Parameters__[__Index__], __Name__))
            return __Index__;
    }
    return SIZE_MAX;
}

static __Ast_Type__ *__Body_Generic_Expand_Alias__(__Semantic_Body_Context__ *__Context__,
                                                   __Ast_Type__ *__Type__)
{
    __Semantic_Type_Entry__ *__Entry__ = NULL;
    if (__Type__ == NULL || __Type__->__Kind__ != __Ast_Type_Named__)
        return __Type__;
    if (!__Semantic_Resolve_Named_Entry__(__Context__->__Semantic__, __Type__, &__Entry__) ||
        __Entry__ == NULL || __Entry__->__Declaration__ == NULL)
        return __Type__;
    if (__Entry__->__Declaration__->__Kind__ == __Ast_Type_Decl_Alias__)
        return __Entry__->__Declaration__->__As__.__Alias__;
    return __Type__;
}

static int __Body_Generic_Type_Contains_Parameter__(
    const __Ast_Function__ *__Template_Function__,
    __Ast_Type__ *__Type__)
{
    size_t __Index__;
    if (__Template_Function__ == NULL || __Type__ == NULL)
        return 0;
    if (__Type__->__Kind__ == __Ast_Type_Named__ &&
        __Type__->__As__.__Named__.__Argument_Count__ == 0U &&
        __Body_Generic_Parameter_Index__(
            __Template_Function__, __Type__->__As__.__Named__.__Name__) != SIZE_MAX)
        return 1;
    switch (__Type__->__Kind__)
    {
        case __Ast_Type_Mutable__:
        case __Ast_Type_Reference__:
        case __Ast_Type_Vector__:
        case __Ast_Type_Box__:
        case __Ast_Type_Option__:
            return __Body_Generic_Type_Contains_Parameter__(
                __Template_Function__, __Type__->__As__.__Inner__);
        case __Ast_Type_Result__:
            return __Body_Generic_Type_Contains_Parameter__(
                       __Template_Function__, __Type__->__As__.__Result__.__Ok__) ||
                   __Body_Generic_Type_Contains_Parameter__(
                       __Template_Function__, __Type__->__As__.__Result__.__Error__);
        case __Ast_Type_Function__:
            for (__Index__ = 0U; __Index__ < __Type__->__As__.__Function__.__Parameter_Count__;
                 ++__Index__)
            {
                if (__Body_Generic_Type_Contains_Parameter__(
                        __Template_Function__,
                        __Type__->__As__.__Function__.__Parameters__[__Index__]))
                    return 1;
            }
            return __Body_Generic_Type_Contains_Parameter__(
                __Template_Function__, __Type__->__As__.__Function__.__Output__);
        case __Ast_Type_Named__:
            for (__Index__ = 0U; __Index__ < __Type__->__As__.__Named__.__Argument_Count__; ++__Index__)
            {
                if (__Body_Generic_Type_Contains_Parameter__(
                        __Template_Function__, __Type__->__As__.__Named__.__Arguments__[__Index__]))
                    return 1;
            }
            return 0;
        default:
            return 0;
    }
}

static int __Body_Generic_Infer_Type__(__Semantic_Body_Context__ *__Context__,
                                      const __Ast_Function__ *__Template_Function__,
                                      __Ast_Type__ *__Template_Type__,
                                      __Ast_Type__ *__Actual_Type__,
                                      __Ast_Type__ **__Bindings__)
{
    size_t __Parameter_Index__;
    size_t __Index__;
    if (__Template_Type__ == NULL || __Actual_Type__ == NULL)
        return 0;

    if (!__Body_Generic_Type_Contains_Parameter__(__Template_Function__, __Template_Type__))
        return 1;

    if (__Template_Type__->__Kind__ == __Ast_Type_Named__ &&
        __Template_Type__->__As__.__Named__.__Argument_Count__ == 0U)
    {
        __Parameter_Index__ = __Body_Generic_Parameter_Index__(
            __Template_Function__, __Template_Type__->__As__.__Named__.__Name__);
        if (__Parameter_Index__ != SIZE_MAX)
        {
            if (__Bindings__[__Parameter_Index__] == NULL)
            {
                __Bindings__[__Parameter_Index__] = __Actual_Type__;
            }
            /* Once a canonical binding exists, concrete argument checking below
             * owns compatibility. This preserves contextual literal conversion
             * instead of requiring the literal's provisional default type to
             * equal the already-inferred generic argument. */
            return 1;
        }
    }

    __Actual_Type__ = __Body_Generic_Expand_Alias__(__Context__, __Actual_Type__);
    if (__Template_Type__->__Kind__ != __Actual_Type__->__Kind__)
    {
        return __Type_Compatible__(__Context__->__Semantic__, __Template_Type__, __Actual_Type__);
    }

    switch (__Template_Type__->__Kind__)
    {
        case __Ast_Type_Reference__:
        {
            __Ast_Type__ *__Template_Inner__ = __Template_Type__->__As__.__Inner__;
            __Ast_Type__ *__Actual_Inner__ = __Actual_Type__->__As__.__Inner__;

            /* Generic binding follows the referenced value type, while ordinary
             * call compatibility remains responsible for reference mutability. */
            if (__Template_Inner__ != NULL && __Actual_Inner__ != NULL)
            {
                if (__Template_Inner__->__Kind__ == __Ast_Type_Mutable__ &&
                    __Actual_Inner__->__Kind__ != __Ast_Type_Mutable__)
                {
                    __Template_Inner__ = __Template_Inner__->__As__.__Inner__;
                }
                else if (__Actual_Inner__->__Kind__ == __Ast_Type_Mutable__ &&
                         __Template_Inner__->__Kind__ != __Ast_Type_Mutable__)
                {
                    __Actual_Inner__ = __Actual_Inner__->__As__.__Inner__;
                }
            }
            return __Body_Generic_Infer_Type__(
                __Context__, __Template_Function__, __Template_Inner__,
                __Actual_Inner__, __Bindings__);
        }
        case __Ast_Type_Mutable__:
        case __Ast_Type_Vector__:
        case __Ast_Type_Box__:
        case __Ast_Type_Option__:
            return __Body_Generic_Infer_Type__(
                __Context__, __Template_Function__, __Template_Type__->__As__.__Inner__,
                __Actual_Type__->__As__.__Inner__, __Bindings__);
        case __Ast_Type_Result__:
            return __Body_Generic_Infer_Type__(
                       __Context__, __Template_Function__,
                       __Template_Type__->__As__.__Result__.__Ok__,
                       __Actual_Type__->__As__.__Result__.__Ok__, __Bindings__) &&
                   __Body_Generic_Infer_Type__(
                       __Context__, __Template_Function__,
                       __Template_Type__->__As__.__Result__.__Error__,
                       __Actual_Type__->__As__.__Result__.__Error__, __Bindings__);
        case __Ast_Type_Function__:
            if (__Template_Type__->__As__.__Function__.__Parameter_Count__ !=
                __Actual_Type__->__As__.__Function__.__Parameter_Count__)
                return 0;
            for (__Index__ = 0U;
                 __Index__ < __Template_Type__->__As__.__Function__.__Parameter_Count__;
                 ++__Index__)
            {
                if (!__Body_Generic_Infer_Type__(
                        __Context__, __Template_Function__,
                        __Template_Type__->__As__.__Function__.__Parameters__[__Index__],
                        __Actual_Type__->__As__.__Function__.__Parameters__[__Index__], __Bindings__))
                    return 0;
            }
            return __Body_Generic_Infer_Type__(
                __Context__, __Template_Function__, __Template_Type__->__As__.__Function__.__Output__,
                __Actual_Type__->__As__.__Function__.__Output__, __Bindings__);
        case __Ast_Type_Named__:
        {
            __Ast_Type__ *__Expanded_Actual__ = __Body_Generic_Expand_Alias__(__Context__, __Actual_Type__);
            if (__Expanded_Actual__ != __Actual_Type__)
                return __Body_Generic_Infer_Type__(__Context__, __Template_Function__, __Template_Type__,
                                                   __Expanded_Actual__, __Bindings__);
            if (!__Identifier_Identity_Equals__(__Template_Type__->__As__.__Named__.__Name__,
                                                __Actual_Type__->__As__.__Named__.__Name__) ||
                __Template_Type__->__As__.__Named__.__Argument_Count__ !=
                    __Actual_Type__->__As__.__Named__.__Argument_Count__)
                return 0;
            for (__Index__ = 0U; __Index__ < __Template_Type__->__As__.__Named__.__Argument_Count__;
                 ++__Index__)
            {
                if (!__Body_Generic_Infer_Type__(
                        __Context__, __Template_Function__,
                        __Template_Type__->__As__.__Named__.__Arguments__[__Index__],
                        __Actual_Type__->__As__.__Named__.__Arguments__[__Index__], __Bindings__))
                    return 0;
            }
            return 1;
        }
        default:
            return __Type_Compatible__(__Context__->__Semantic__, __Template_Type__, __Actual_Type__);
    }
}

static __Semantic_Function_Entry__ *__Body_Instantiate_Generic_Callee__(
    __Semantic_Body_Context__ *__Context__,
    __Semantic_Function_Entry__ *__Template__,
    __Ast_Expression__ *__Call__)
{
    __Ast_Type__ **__Bindings__;
    size_t __Index__;
    if (__Template__ == NULL || __Template__->__Function__ == NULL ||
        !__Template__->__Is_Generic_Template__)
        return __Template__;
    __Bindings__ = (__Ast_Type__ **)__Arena_Allocate__(
        &__Context__->__Synthetic_Types__,
        __Template__->__Function__->__Type_Parameter_Count__ * sizeof(*__Bindings__),
        _Alignof(__Ast_Type__ *));
    if (__Bindings__ == NULL)
        return NULL;
    memset(__Bindings__, 0,
           __Template__->__Function__->__Type_Parameter_Count__ * sizeof(*__Bindings__));
    if (__Template__->__Function__->__Parameter_Count__ !=
        __Call__->__As__.__Call__.__Argument_Count__)
        return NULL;

    /* Contextual output types constrain generic parameters before argument
     * inference. Integer literals initially infer a provisional default type;
     * binding from that provisional type first would incorrectly prevent a
     * surrounding expected type such as u8 from selecting the concrete generic
     * instance. Ordinary argument compatibility below still decides whether
     * each actual argument fits the selected instance. */
    if (__Call__->__Contextual_Type__ != NULL)
    {
        __Ast_Type__ *__Expected_Output__ = __Type_Unwrap_Mutable__(__Call__->__Contextual_Type__);
        if (__Expected_Output__ == NULL ||
            !__Body_Generic_Infer_Type__(
                __Context__, __Template__->__Function__,
                __Template__->__Function__->__Output__.__Type__,
                __Expected_Output__, __Bindings__))
        {
            return NULL;
        }
    }

    for (__Index__ = 0U; __Index__ < __Template__->__Function__->__Parameter_Count__; ++__Index__)
    {
        __Ast_Type__ *__Actual__ = NULL;
        if (!__Body_Infer_Expression__(
                __Context__, __Call__->__As__.__Call__.__Arguments__[__Index__], &__Actual__) ||
            !__Body_Generic_Infer_Type__(
                __Context__, __Template__->__Function__,
                __Template__->__Function__->__Parameters__[__Index__].__Slot__.__Type__,
                __Actual__, __Bindings__))
            return NULL;
    }
    for (__Index__ = 0U; __Index__ < __Template__->__Function__->__Type_Parameter_Count__; ++__Index__)
    {
        if (__Bindings__[__Index__] == NULL)
            return NULL;
    }
    return __Semantic_Generic_Function_Instance__(
        __Context__->__Semantic__, __Template__, __Bindings__,
        __Template__->__Function__->__Type_Parameter_Count__);
}

/* Infers the body call. */
int __Body_Infer_Call__(__Semantic_Body_Context__ *__Context__,
                        __Ast_Expression__ *__Expression__,
                        __Ast_Type__ **__Out_Type__)
{
    /* References the function lvalue. */
    __Ast_Lvalue__ *__Function_Lvalue__ = __Expression__->__As__.__Call__.__Function__;
    /* References the callee. */
    __Semantic_Function_Entry__ *__Callee__ = NULL;
    /* References the local. */
    __Semantic_Local__ *__Local__ = NULL;

    {
        /* Tracks the builtin matched state. */
        int __Builtin_Matched__ = 0;
        if (!__Body_Try_Infer_Builtin_Call__(
                __Context__, __Expression__, __Out_Type__, &__Builtin_Matched__))
        {
            return 0;
        }
        if (__Builtin_Matched__)
        {
            return 1;
        }
    }

    if (__Function_Lvalue__ == NULL)
    {
        return __Body_Fail__(
            __Context__, __E0300_Unknown_Name__, __Expression__->__Header__.__Span__);
    }

    {
        /* Tracks the matched state. */
        int __Matched__ = 0;
        if (!__Body_Try_Infer_Enum_Construct__(
                __Context__, __Expression__, __Out_Type__, &__Matched__))
        {
            return 0;
        }
        if (__Matched__)
        {
            return 1;
        }
    }

    if (__Function_Lvalue__->__Kind__ == __Ast_Lvalue_Base__ &&
        __Function_Lvalue__->__As__.__Base__.__Kind__ == __Ast_Lvalue_Base_Identifier__)
    {
        __Local__ = __Body_Find_Local__(__Context__, __Function_Lvalue__);
        if (__Local__ == NULL)
        {
            /* Stores the lookup. */
            __Name_Lookup_Status__ __Lookup__ = __Name_Resolve_Function__(
                __Context__->__Semantic__,
                __Context__->__Function__->__Unit__,
                __Function_Lvalue__->__As__.__Base__.__As__.__Identifier__,
                &__Callee__);
            if (__Lookup__ == __Name_Lookup_Private__ || __Lookup__ == __Name_Lookup_Ambiguous__)
            {
                return __Body_Fail__(__Context__,
                                     __Name_Lookup_Error_Id__(__Lookup__),
                                     __Expression__->__Header__.__Span__);
            }
        }
    }

    if (__Callee__ != NULL && __Callee__->__Is_Generic_Template__)
    {
        __Semantic_Function_Entry__ *__Instance__ =
            __Body_Instantiate_Generic_Callee__(__Context__, __Callee__, __Expression__);
        if (__Instance__ == NULL)
        {
            return __Body_Fail__(__Context__, __E0400_Mismatched_Types__,
                                 __Expression__->__Header__.__Span__);
        }
        __Callee__ = __Instance__;
        __Expression__->__As__.__Call__.__Resolved_Generic_Function_Index__ =
            __Callee__->__Index__ - __Context__->__Semantic__->__Functions__.__Count__;
    }

    if (__Callee__ != NULL)
    {
        /* References the parameters. */
        __Ast_Type__ **__Parameters__ = NULL;

        /* Tracks the index. */
        size_t __Index__;
        /* Tracks whether the operation succeeded. */
        int __Ok__;

        if (__Callee__->__Function__->__Parameter_Count__ != 0U)
        {
            __Parameters__ = (__Ast_Type__ **)__Arena_Allocate__(
                &__Context__->__Synthetic_Types__,
                __Callee__->__Function__->__Parameter_Count__ * sizeof(*__Parameters__),
                _Alignof(__Ast_Type__ *));
            if (__Parameters__ == NULL)
            {
                return __Body_Fail__(__Context__,
                                     __E1100_Internal_Context_Error__,
                                     __Expression__->__Header__.__Span__);
            }
            for (__Index__ = 0U; __Index__ < __Callee__->__Function__->__Parameter_Count__;
                 ++__Index__)
            {
                __Parameters__[__Index__] =
                    __Callee__->__Function__->__Parameters__[__Index__].__Slot__.__Type__;
            }
        }
        __Ok__ = __Body_Check_Call_Arguments__(__Context__,
                                               __Parameters__,
                                               __Callee__->__Function__->__Parameter_Count__,
                                               __Expression__->__As__.__Call__.__Arguments__,
                                               __Expression__->__As__.__Call__.__Argument_Count__,
                                               !__Expression__->__Semantic_Effects_Applied__);
        if (!__Ok__)
        {
            return 0;
        }
        *__Out_Type__ = __Callee__->__Function__->__Output__.__Type__;
        __Expression__->__Semantic_Effects_Applied__ = 1;
        return 1;
    }

    if (__Local__ != NULL)
    {
        __Resolved_Type__ __Resolved__;
        if (__Type_Resolve__(__Context__->__Semantic__, __Local__->__Type__, &__Resolved__) &&
            __Resolved__.__Kind__ == __Resolved_Type_Function__)
        {
            if (!__Body_Check_Call_Arguments__(
                    __Context__, __Resolved__.__Parameters__, __Resolved__.__Parameter_Count__,
                    __Expression__->__As__.__Call__.__Arguments__,
                    __Expression__->__As__.__Call__.__Argument_Count__,
                    !__Expression__->__Semantic_Effects_Applied__))
                return 0;
            *__Out_Type__ = __Resolved__.__Output__;
            __Expression__->__Semantic_Effects_Applied__ = 1;
            return 1;
        }
    }
    return __Body_Fail__(
        __Context__, __E0400_Mismatched_Types__, __Expression__->__Header__.__Span__);
}
