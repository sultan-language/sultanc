/* Owns Stage0 generic substitution and monomorphized AST instances. */

#include "semantic/generic.h"
#include "frontend/identifier_identity.h"
#include "kernel/type/type.h"
#include "semantic/diagnostic.h"
#include "semantic/type_named.h"

#include <stdalign.h>
#include <stdint.h>
#include <string.h>

static void *__Generic_Allocate__(__Semantic_Context__ *__Context__,
                                  size_t __Size__,
                                  size_t __Alignment__)
{
    void *__Value__ = __Arena_Allocate__(
        &__Context__->__Generic_Arena__, __Size__, __Alignment__);
    if (__Value__ != NULL)
    {
        memset(__Value__, 0, __Size__);
    }
    return __Value__;
}

__Ast_Type__ *__Semantic_Make_Function_Type__(__Semantic_Context__ *__Context__,
                                              const __Ast_Function__ *__Function__)
{
    __Ast_Type__ *__Type__;
    __Ast_Type__ **__Parameters__ = NULL;
    size_t __Index__;
    if (__Context__ == NULL || __Function__ == NULL)
        return NULL;
    __Type__ = (__Ast_Type__ *)__Generic_Allocate__(
        __Context__, sizeof(*__Type__), alignof(__Ast_Type__));
    if (__Type__ == NULL)
        return NULL;
    if (__Function__->__Parameter_Count__ != 0U)
    {
        __Parameters__ = (__Ast_Type__ **)__Generic_Allocate__(
            __Context__, __Function__->__Parameter_Count__ * sizeof(*__Parameters__),
            alignof(__Ast_Type__ *));
        if (__Parameters__ == NULL)
            return NULL;
        for (__Index__ = 0U; __Index__ < __Function__->__Parameter_Count__; ++__Index__)
            __Parameters__[__Index__] = __Function__->__Parameters__[__Index__].__Slot__.__Type__;
    }
    __Type__->__Kind__ = __Ast_Type_Function__;
    __Type__->__As__.__Function__.__Parameters__ = __Parameters__;
    __Type__->__As__.__Function__.__Parameter_Count__ = __Function__->__Parameter_Count__;
    __Type__->__As__.__Function__.__Output__ = __Function__->__Output__.__Type__;
    return __Type__;
}

static __Ast_Type__ *__Generic_Find_Parameter__(const __Text_Slice__ *__Parameters__,
                                                __Ast_Type__ *const *__Arguments__,
                                                size_t __Argument_Count__,
                                                __Text_Slice__ __Name__)
{
    size_t __Index__;
    for (__Index__ = 0U; __Index__ < __Argument_Count__; ++__Index__)
    {
        if (__Identifier_Identity_Equals__(__Parameters__[__Index__], __Name__))
        {
            return __Arguments__[__Index__];
        }
    }
    return NULL;
}

static int __Generic_Record_Type_Owner__(__Semantic_Context__ *__Context__,
                                         __Ast_Type__ *__Type__,
                                         const __Program_Unit__ *__Unit__)
{
    if (__Type__ == NULL)
    {
        return 1;
    }
    if (__Type__->__Kind__ == __Ast_Type_Named__)
    {
        __Semantic_Type_Owner_Fact__ __Fact__;
        __Fact__.__Type__ = __Type__;
        __Fact__.__Unit__ = __Unit__;
        if (__Vector_Push__(&__Context__->__Type_Owners__, &__Fact__) == NULL)
        {
            return 0;
        }
    }
    return 1;
}

__Ast_Type__ *__Semantic_Generic_Substitute_Type__(__Semantic_Context__ *__Context__,
                                                    __Ast_Type__ *__Type__,
                                                    const __Text_Slice__ *__Parameters__,
                                                    __Ast_Type__ *const *__Arguments__,
                                                    size_t __Argument_Count__,
                                                    const __Program_Unit__ *__Owner_Unit__)
{
    __Ast_Type__ *__Result__;
    const __Program_Unit__ *__Effective_Owner_Unit__ = __Owner_Unit__;
    size_t __Index__;
    if (__Context__ == NULL || __Type__ == NULL)
    {
        return NULL;
    }

    /*
     * A concrete type substituted from the caller keeps the lexical unit where
     * that type was written.  The template unit only owns the template nodes.
     * Without this distinction a type argument imported by the caller is
     * incorrectly resolved as though it were declared inside the generic
     * template module.
     */
    if (__Argument_Count__ == 0U)
    {
        const __Program_Unit__ *__Recorded_Owner_Unit__ =
            __Semantic_Type_Owner_Unit__(__Context__, __Type__);
        if (__Recorded_Owner_Unit__ != NULL)
        {
            __Effective_Owner_Unit__ = __Recorded_Owner_Unit__;
        }
    }
    if (__Effective_Owner_Unit__ == NULL)
    {
        __Effective_Owner_Unit__ = __Context__->__Active_Unit__;
    }

    if (__Type__->__Kind__ == __Ast_Type_Named__ &&
        __Type__->__As__.__Named__.__Argument_Count__ == 0U && __Argument_Count__ != 0U)
    {
        __Ast_Type__ *__Bound__ = __Generic_Find_Parameter__(
            __Parameters__, __Arguments__, __Argument_Count__, __Type__->__As__.__Named__.__Name__);
        if (__Bound__ != NULL)
        {
            return __Semantic_Generic_Substitute_Type__(
                __Context__, __Bound__, NULL, NULL, 0U, __Effective_Owner_Unit__);
        }
    }

    __Result__ = (__Ast_Type__ *)__Generic_Allocate__(
        __Context__, sizeof(*__Result__), alignof(__Ast_Type__));
    if (__Result__ == NULL)
    {
        return NULL;
    }
    *__Result__ = *__Type__;

    switch (__Type__->__Kind__)
    {
        case __Ast_Type_Reference__:
        case __Ast_Type_Vector__:
        case __Ast_Type_Box__:
        case __Ast_Type_Option__:
        case __Ast_Type_Mutable__:
            __Result__->__As__.__Inner__ = __Semantic_Generic_Substitute_Type__(
                __Context__,
                __Type__->__As__.__Inner__,
                __Parameters__,
                __Arguments__,
                __Argument_Count__,
                __Effective_Owner_Unit__);
            if (__Result__->__As__.__Inner__ == NULL)
                return NULL;
            break;
        case __Ast_Type_Result__:
            __Result__->__As__.__Result__.__Ok__ = __Semantic_Generic_Substitute_Type__(
                __Context__, __Type__->__As__.__Result__.__Ok__, __Parameters__, __Arguments__,
                __Argument_Count__, __Effective_Owner_Unit__);
            __Result__->__As__.__Result__.__Error__ = __Semantic_Generic_Substitute_Type__(
                __Context__, __Type__->__As__.__Result__.__Error__, __Parameters__, __Arguments__,
                __Argument_Count__, __Effective_Owner_Unit__);
            if (__Result__->__As__.__Result__.__Ok__ == NULL ||
                __Result__->__As__.__Result__.__Error__ == NULL)
                return NULL;
            break;
        case __Ast_Type_Function__:
            __Result__->__As__.__Function__.__Parameters__ = NULL;
            if (__Type__->__As__.__Function__.__Parameter_Count__ != 0U)
            {
                __Result__->__As__.__Function__.__Parameters__ = (__Ast_Type__ **)__Generic_Allocate__(
                    __Context__,
                    __Type__->__As__.__Function__.__Parameter_Count__ * sizeof(__Ast_Type__ *),
                    alignof(__Ast_Type__ *));
                if (__Result__->__As__.__Function__.__Parameters__ == NULL)
                    return NULL;
            }
            for (__Index__ = 0U; __Index__ < __Type__->__As__.__Function__.__Parameter_Count__;
                 ++__Index__)
            {
                __Result__->__As__.__Function__.__Parameters__[__Index__] =
                    __Semantic_Generic_Substitute_Type__(
                        __Context__,
                        __Type__->__As__.__Function__.__Parameters__[__Index__],
                        __Parameters__, __Arguments__, __Argument_Count__, __Effective_Owner_Unit__);
                if (__Result__->__As__.__Function__.__Parameters__[__Index__] == NULL)
                    return NULL;
            }
            __Result__->__As__.__Function__.__Output__ = __Semantic_Generic_Substitute_Type__(
                __Context__, __Type__->__As__.__Function__.__Output__, __Parameters__, __Arguments__,
                __Argument_Count__, __Effective_Owner_Unit__);
            if (__Result__->__As__.__Function__.__Output__ == NULL)
                return NULL;
            break;
        case __Ast_Type_Named__:
            __Result__->__As__.__Named__.__Arguments__ = NULL;
            if (__Type__->__As__.__Named__.__Argument_Count__ != 0U)
            {
                __Result__->__As__.__Named__.__Arguments__ = (__Ast_Type__ **)__Generic_Allocate__(
                    __Context__,
                    __Type__->__As__.__Named__.__Argument_Count__ * sizeof(__Ast_Type__ *),
                    alignof(__Ast_Type__ *));
                if (__Result__->__As__.__Named__.__Arguments__ == NULL)
                    return NULL;
            }
            for (__Index__ = 0U; __Index__ < __Type__->__As__.__Named__.__Argument_Count__;
                 ++__Index__)
            {
                __Result__->__As__.__Named__.__Arguments__[__Index__] =
                    __Semantic_Generic_Substitute_Type__(
                        __Context__,
                        __Type__->__As__.__Named__.__Arguments__[__Index__],
                        __Parameters__, __Arguments__, __Argument_Count__, __Effective_Owner_Unit__);
                if (__Result__->__As__.__Named__.__Arguments__[__Index__] == NULL)
                    return NULL;
            }
            break;
        default:
            break;
    }
    if (!__Generic_Record_Type_Owner__(__Context__, __Result__, __Effective_Owner_Unit__))
    {
        return NULL;
    }
    return __Result__;
}

int __Semantic_Generic_Type_Equal__(__Semantic_Context__ *__Context__,
                                    __Ast_Type__ *__Left__,
                                    __Ast_Type__ *__Right__)
{
    return __Type_Compatible__(__Context__, __Left__, __Right__) &&
           __Type_Compatible__(__Context__, __Right__, __Left__);
}

static __Ast_Type_Declaration__ *__Generic_Clone_Type_Declaration__(
    __Semantic_Context__ *__Context__,
    __Ast_Type_Declaration__ *__Template__,
    __Ast_Type__ *const *__Arguments__,
    const __Program_Unit__ *__Unit__)
{
    __Ast_Type_Declaration__ *__Declaration__;
    size_t __Index__;
    __Declaration__ = (__Ast_Type_Declaration__ *)__Generic_Allocate__(
        __Context__, sizeof(*__Declaration__), alignof(__Ast_Type_Declaration__));
    if (__Declaration__ == NULL)
        return NULL;
    *__Declaration__ = *__Template__;
    __Declaration__->__Type_Parameters__ = NULL;
    __Declaration__->__Type_Parameter_Count__ = 0U;
    if (__Template__->__Kind__ == __Ast_Type_Decl_Alias__)
    {
        __Declaration__->__As__.__Alias__ = __Semantic_Generic_Substitute_Type__(
            __Context__, __Template__->__As__.__Alias__, __Template__->__Type_Parameters__,
            __Arguments__, __Template__->__Type_Parameter_Count__, __Unit__);
        if (__Declaration__->__As__.__Alias__ == NULL)
            return NULL;
    }
    else if (__Template__->__Kind__ == __Ast_Type_Decl_Struct__)
    {
        __Declaration__->__As__.__Struct__.__Fields__ = NULL;
        if (__Template__->__As__.__Struct__.__Count__ != 0U)
        {
            __Declaration__->__As__.__Struct__.__Fields__ =
                (__Ast_Struct_Field__ *)__Generic_Allocate__(
                    __Context__,
                    __Template__->__As__.__Struct__.__Count__ * sizeof(__Ast_Struct_Field__),
                    alignof(__Ast_Struct_Field__));
            if (__Declaration__->__As__.__Struct__.__Fields__ == NULL)
                return NULL;
        }
        for (__Index__ = 0U; __Index__ < __Template__->__As__.__Struct__.__Count__; ++__Index__)
        {
            __Declaration__->__As__.__Struct__.__Fields__[__Index__] =
                __Template__->__As__.__Struct__.__Fields__[__Index__];
            __Declaration__->__As__.__Struct__.__Fields__[__Index__].__Slot__.__Type__ =
                __Semantic_Generic_Substitute_Type__(
                    __Context__,
                    __Template__->__As__.__Struct__.__Fields__[__Index__].__Slot__.__Type__,
                    __Template__->__Type_Parameters__, __Arguments__,
                    __Template__->__Type_Parameter_Count__, __Unit__);
            if (__Declaration__->__As__.__Struct__.__Fields__[__Index__].__Slot__.__Type__ == NULL)
                return NULL;
        }
    }
    else
    {
        __Declaration__->__As__.__Enum__.__Constructors__ = NULL;
        if (__Template__->__As__.__Enum__.__Count__ != 0U)
        {
            __Declaration__->__As__.__Enum__.__Constructors__ =
                (__Ast_Enum_Constructor__ *)__Generic_Allocate__(
                    __Context__,
                    __Template__->__As__.__Enum__.__Count__ * sizeof(__Ast_Enum_Constructor__),
                    alignof(__Ast_Enum_Constructor__));
            if (__Declaration__->__As__.__Enum__.__Constructors__ == NULL)
                return NULL;
        }
        for (__Index__ = 0U; __Index__ < __Template__->__As__.__Enum__.__Count__; ++__Index__)
        {
            __Ast_Enum_Constructor__ *__Target__ =
                &__Declaration__->__As__.__Enum__.__Constructors__[__Index__];
            __Ast_Enum_Constructor__ *__Source__ =
                &__Template__->__As__.__Enum__.__Constructors__[__Index__];
            size_t __Payload_Index__;
            *__Target__ = *__Source__;
            __Target__->__Payload_Slots__ = NULL;
            if (__Source__->__Payload_Count__ != 0U)
            {
                __Target__->__Payload_Slots__ = (__Ast_Slot__ *)__Generic_Allocate__(
                    __Context__, __Source__->__Payload_Count__ * sizeof(__Ast_Slot__),
                    alignof(__Ast_Slot__));
                if (__Target__->__Payload_Slots__ == NULL)
                    return NULL;
            }
            for (__Payload_Index__ = 0U; __Payload_Index__ < __Source__->__Payload_Count__;
                 ++__Payload_Index__)
            {
                __Target__->__Payload_Slots__[__Payload_Index__] =
                    __Source__->__Payload_Slots__[__Payload_Index__];
                __Target__->__Payload_Slots__[__Payload_Index__].__Type__ =
                    __Semantic_Generic_Substitute_Type__(
                        __Context__, __Source__->__Payload_Slots__[__Payload_Index__].__Type__,
                        __Template__->__Type_Parameters__, __Arguments__,
                        __Template__->__Type_Parameter_Count__, __Unit__);
                if (__Target__->__Payload_Slots__[__Payload_Index__].__Type__ == NULL)
                    return NULL;
            }
        }
    }
    return __Declaration__;
}

int __Semantic_Generic_Resolve_Type_Instance__(__Semantic_Context__ *__Context__,
                                               __Semantic_Type_Entry__ *__Template__,
                                               __Ast_Type__ *__Use_Type__,
                                               __Semantic_Type_Entry__ **__Out_Entry__)
{
    __Ast_Type__ **__Arguments__ = NULL;
    const __Program_Unit__ *__Owner_Unit__;
    size_t __Index__;
    if (__Context__ == NULL || __Template__ == NULL || __Use_Type__ == NULL ||
        __Use_Type__->__Kind__ != __Ast_Type_Named__ || __Out_Entry__ == NULL)
        return 0;
    if (__Template__->__Declaration__ == NULL ||
        __Template__->__Declaration__->__Type_Parameter_Count__ == 0U)
    {
        if (__Use_Type__->__As__.__Named__.__Argument_Count__ != 0U)
            return __Semantic_Fail__(
                __Context__, __E0409_Unresolved_Type__, (__Source_Span__){0});
        *__Out_Entry__ = __Template__;
        return 1;
    }
    if (__Use_Type__->__As__.__Named__.__Argument_Count__ !=
        __Template__->__Declaration__->__Type_Parameter_Count__)
    {
        return __Semantic_Fail__(
            __Context__, __E0409_Unresolved_Type__, (__Source_Span__){0});
    }
    __Owner_Unit__ = __Semantic_Type_Owner_Unit__(__Context__, __Use_Type__);
    if (__Owner_Unit__ == NULL)
        __Owner_Unit__ = __Context__->__Active_Unit__;

    /*
     * Lookup must stay allocation-free on a cache hit.  The use-site arguments
     * already describe the concrete semantic identity.  Deep-copying them before
     * the lookup used to append fresh type-owner facts on every repeated generic
     * resolution, causing the Stage0 generic arena and owner table to grow with
     * queries rather than with distinct instances.
     */
    for (__Index__ = 0U; __Index__ < __Context__->__Generic_Types__.__Count__; ++__Index__)
    {
        __Semantic_Type_Entry__ **__Slot__ = (__Semantic_Type_Entry__ **)__Vector_At__(
            &__Context__->__Generic_Types__, __Index__);
        __Semantic_Type_Entry__ *__Entry__ = __Slot__ == NULL ? NULL : *__Slot__;
        size_t __Argument_Index__;
        int __Equal__ = 1;
        if (__Entry__ == NULL || __Entry__->__Template_Declaration__ != __Template__->__Declaration__ ||
            __Entry__->__Type_Argument_Count__ != __Use_Type__->__As__.__Named__.__Argument_Count__)
            continue;
        for (__Argument_Index__ = 0U; __Argument_Index__ < __Entry__->__Type_Argument_Count__;
             ++__Argument_Index__)
        {
            if (!__Semantic_Generic_Type_Equal__(
                    __Context__, __Entry__->__Type_Arguments__[__Argument_Index__],
                    __Use_Type__->__As__.__Named__.__Arguments__[__Argument_Index__]))
            {
                __Equal__ = 0;
                break;
            }
        }
        if (__Equal__)
        {
            *__Out_Entry__ = __Entry__;
            return 1;
        }
    }

    /* Only distinct instances need persistent, arena-owned argument copies. */
    __Arguments__ = (__Ast_Type__ **)__Generic_Allocate__(
        __Context__,
        __Use_Type__->__As__.__Named__.__Argument_Count__ * sizeof(__Ast_Type__ *),
        alignof(__Ast_Type__ *));
    if (__Arguments__ == NULL)
        return __Semantic_Fail__(
            __Context__, __E1100_Internal_Context_Error__, (__Source_Span__){0});
    for (__Index__ = 0U; __Index__ < __Use_Type__->__As__.__Named__.__Argument_Count__; ++__Index__)
    {
        __Arguments__[__Index__] = __Semantic_Generic_Substitute_Type__(
            __Context__, __Use_Type__->__As__.__Named__.__Arguments__[__Index__], NULL, NULL, 0U,
            __Owner_Unit__);
        if (__Arguments__[__Index__] == NULL)
            return __Semantic_Fail__(
                __Context__, __E1100_Internal_Context_Error__, (__Source_Span__){0});
    }
    {
        __Semantic_Type_Entry__ *__Entry__ = (__Semantic_Type_Entry__ *)__Generic_Allocate__(
            __Context__, sizeof(__Semantic_Type_Entry__), alignof(__Semantic_Type_Entry__));
        if (__Entry__ == NULL)
            return __Semantic_Fail__(
                __Context__, __E1100_Internal_Context_Error__, (__Source_Span__){0});
        __Entry__->__Name__ = __Template__->__Name__;
        __Entry__->__Template_Declaration__ = __Template__->__Declaration__;
        __Entry__->__Type_Arguments__ = __Arguments__;
        __Entry__->__Type_Argument_Count__ = __Use_Type__->__As__.__Named__.__Argument_Count__;
        __Entry__->__Unit__ = __Template__->__Unit__;
        __Entry__->__Public__ = __Template__->__Public__;
        __Entry__->__Declaration__ = __Generic_Clone_Type_Declaration__(
            __Context__, __Template__->__Declaration__, __Arguments__, __Template__->__Unit__);
        if (__Entry__->__Declaration__ == NULL ||
            __Vector_Push__(&__Context__->__Generic_Types__, &__Entry__) == NULL)
        {
            return __Semantic_Fail__(
                __Context__, __E1100_Internal_Context_Error__, (__Source_Span__){0});
        }
        *__Out_Entry__ = __Entry__;
    }
    return 1;
}

static __Ast_Expression__ *__Generic_Clone_Expression__(__Semantic_Context__ *,
                                                      __Ast_Expression__ *,
                                                      const __Text_Slice__ *,
                                                      __Ast_Type__ *const *,
                                                      size_t,
                                                      const __Program_Unit__ *);
static __Ast_Block__ *__Generic_Clone_Block__(__Semantic_Context__ *,
                                            __Ast_Block__ *,
                                            const __Text_Slice__ *,
                                            __Ast_Type__ *const *,
                                            size_t,
                                            const __Program_Unit__ *);

static __Ast_Expression__ *__Generic_Clone_Expression__(__Semantic_Context__ *__Context__,
                                                      __Ast_Expression__ *__Source__,
                                                      const __Text_Slice__ *__Parameters__,
                                                      __Ast_Type__ *const *__Arguments__,
                                                      size_t __Argument_Count__,
                                                      const __Program_Unit__ *__Unit__)
{
    __Ast_Expression__ *__Result__;
    size_t __Index__;
    if (__Source__ == NULL)
        return NULL;
    __Result__ = (__Ast_Expression__ *)__Generic_Allocate__(
        __Context__, sizeof(*__Result__), alignof(__Ast_Expression__));
    if (__Result__ == NULL)
        return NULL;
    *__Result__ = *__Source__;
    __Result__->__Contextual_Type__ = NULL;
    __Result__->__Semantic_Effects_Applied__ = 0;
    switch (__Source__->__Kind__)
    {
        case __Ast_Expression_Binary__:
            __Result__->__As__.__Binary__.__Left__ = __Generic_Clone_Expression__(
                __Context__, __Source__->__As__.__Binary__.__Left__, __Parameters__, __Arguments__,
                __Argument_Count__, __Unit__);
            __Result__->__As__.__Binary__.__Right__ = __Generic_Clone_Expression__(
                __Context__, __Source__->__As__.__Binary__.__Right__, __Parameters__, __Arguments__,
                __Argument_Count__, __Unit__);
            break;
        case __Ast_Expression_Unary__:
            __Result__->__As__.__Unary__.__Operand__ = __Generic_Clone_Expression__(
                __Context__, __Source__->__As__.__Unary__.__Operand__, __Parameters__, __Arguments__,
                __Argument_Count__, __Unit__);
            break;
        case __Ast_Expression_Call__:
            __Result__->__As__.__Call__.__Resolved_Generic_Function_Index__ = SIZE_MAX;
            __Result__->__As__.__Call__.__Arguments__ = NULL;
            if (__Source__->__As__.__Call__.__Argument_Count__ != 0U)
            {
                __Result__->__As__.__Call__.__Arguments__ = (__Ast_Expression__ **)__Generic_Allocate__(
                    __Context__,
                    __Source__->__As__.__Call__.__Argument_Count__ * sizeof(__Ast_Expression__ *),
                    alignof(__Ast_Expression__ *));
                if (__Result__->__As__.__Call__.__Arguments__ == NULL)
                    return NULL;
            }
            for (__Index__ = 0U; __Index__ < __Source__->__As__.__Call__.__Argument_Count__;
                 ++__Index__)
            {
                __Result__->__As__.__Call__.__Arguments__[__Index__] = __Generic_Clone_Expression__(
                    __Context__, __Source__->__As__.__Call__.__Arguments__[__Index__], __Parameters__,
                    __Arguments__, __Argument_Count__, __Unit__);
                if (__Result__->__As__.__Call__.__Arguments__[__Index__] == NULL)
                    return NULL;
            }
            break;
        case __Ast_Expression_Conversion__:
            __Result__->__As__.__Conversion__.__Operand__ = __Generic_Clone_Expression__(
                __Context__, __Source__->__As__.__Conversion__.__Operand__, __Parameters__,
                __Arguments__, __Argument_Count__, __Unit__);
            __Result__->__As__.__Conversion__.__Target_Type__ =
                __Semantic_Generic_Substitute_Type__(
                    __Context__, __Source__->__As__.__Conversion__.__Target_Type__, __Parameters__,
                    __Arguments__, __Argument_Count__, __Unit__);
            break;
        case __Ast_Expression_Atom__:
            break;
    }
    return __Result__;
}

static __Ast_Statement__ *__Generic_Clone_Statement__(__Semantic_Context__ *__Context__,
                                                    __Ast_Statement__ *__Source__,
                                                    const __Text_Slice__ *__Parameters__,
                                                    __Ast_Type__ *const *__Arguments__,
                                                    size_t __Argument_Count__,
                                                    const __Program_Unit__ *__Unit__)
{
    __Ast_Statement__ *__Result__;
    size_t __Index__;
    if (__Source__ == NULL)
        return NULL;
    __Result__ = (__Ast_Statement__ *)__Generic_Allocate__(
        __Context__, sizeof(*__Result__), alignof(__Ast_Statement__));
    if (__Result__ == NULL)
        return NULL;
    *__Result__ = *__Source__;
    switch (__Source__->__Kind__)
    {
        case __Ast_Statement_Variable_Declaration__:
            __Result__->__As__.__Variable__.__Slot__.__Type__ = __Semantic_Generic_Substitute_Type__(
                __Context__, __Source__->__As__.__Variable__.__Slot__.__Type__, __Parameters__,
                __Arguments__, __Argument_Count__, __Unit__);
            break;
        case __Ast_Statement_Copy__:
            __Result__->__As__.__Copy__.__Expression__ = __Generic_Clone_Expression__(
                __Context__, __Source__->__As__.__Copy__.__Expression__, __Parameters__, __Arguments__,
                __Argument_Count__, __Unit__);
            break;
        case __Ast_Statement_While__:
            __Result__->__As__.__While__.__Condition__ = __Generic_Clone_Expression__(
                __Context__, __Source__->__As__.__While__.__Condition__, __Parameters__, __Arguments__,
                __Argument_Count__, __Unit__);
            __Result__->__As__.__While__.__Body__ = __Generic_Clone_Block__(
                __Context__, __Source__->__As__.__While__.__Body__, __Parameters__, __Arguments__,
                __Argument_Count__, __Unit__);
            break;
        case __Ast_Statement_If__:
            __Result__->__As__.__If__.__Condition__ = __Generic_Clone_Expression__(
                __Context__, __Source__->__As__.__If__.__Condition__, __Parameters__, __Arguments__,
                __Argument_Count__, __Unit__);
            __Result__->__As__.__If__.__Then__ = __Generic_Clone_Block__(
                __Context__, __Source__->__As__.__If__.__Then__, __Parameters__, __Arguments__,
                __Argument_Count__, __Unit__);
            __Result__->__As__.__If__.__Else__ = __Generic_Clone_Block__(
                __Context__, __Source__->__As__.__If__.__Else__, __Parameters__, __Arguments__,
                __Argument_Count__, __Unit__);
            break;
        case __Ast_Statement_Match__:
            __Result__->__As__.__Match__.__Value__ = __Generic_Clone_Expression__(
                __Context__, __Source__->__As__.__Match__.__Value__, __Parameters__, __Arguments__,
                __Argument_Count__, __Unit__);
            __Result__->__As__.__Match__.__Cases__ = NULL;
            if (__Source__->__As__.__Match__.__Case_Count__ != 0U)
            {
                __Result__->__As__.__Match__.__Cases__ = (__Ast_Match_Case__ *)__Generic_Allocate__(
                    __Context__, __Source__->__As__.__Match__.__Case_Count__ * sizeof(__Ast_Match_Case__),
                    alignof(__Ast_Match_Case__));
                if (__Result__->__As__.__Match__.__Cases__ == NULL)
                    return NULL;
            }
            for (__Index__ = 0U; __Index__ < __Source__->__As__.__Match__.__Case_Count__; ++__Index__)
            {
                __Result__->__As__.__Match__.__Cases__[__Index__] =
                    __Source__->__As__.__Match__.__Cases__[__Index__];
                __Result__->__As__.__Match__.__Cases__[__Index__].__Body__ = __Generic_Clone_Block__(
                    __Context__, __Source__->__As__.__Match__.__Cases__[__Index__].__Body__,
                    __Parameters__, __Arguments__, __Argument_Count__, __Unit__);
            }
            break;
        case __Ast_Statement_Return__:
            __Result__->__As__.__Return__ = __Generic_Clone_Expression__(
                __Context__, __Source__->__As__.__Return__, __Parameters__, __Arguments__,
                __Argument_Count__, __Unit__);
            break;
        case __Ast_Statement_Initialize_Record__:
            __Result__->__As__.__Record__.__Fields__ = NULL;
            if (__Source__->__As__.__Record__.__Field_Count__ != 0U)
            {
                __Result__->__As__.__Record__.__Fields__ = (__Ast_Record_Input__ *)__Generic_Allocate__(
                    __Context__,
                    __Source__->__As__.__Record__.__Field_Count__ * sizeof(__Ast_Record_Input__),
                    alignof(__Ast_Record_Input__));
                if (__Result__->__As__.__Record__.__Fields__ == NULL)
                    return NULL;
            }
            for (__Index__ = 0U; __Index__ < __Source__->__As__.__Record__.__Field_Count__; ++__Index__)
            {
                __Result__->__As__.__Record__.__Fields__[__Index__] =
                    __Source__->__As__.__Record__.__Fields__[__Index__];
                __Result__->__As__.__Record__.__Fields__[__Index__].__Value__ =
                    __Generic_Clone_Expression__(
                        __Context__,
                        __Source__->__As__.__Record__.__Fields__[__Index__].__Value__,
                        __Parameters__, __Arguments__, __Argument_Count__, __Unit__);
                if (__Result__->__As__.__Record__.__Fields__[__Index__].__Value__ == NULL)
                    return NULL;
            }
            break;
        case __Ast_Statement_Initialize_Vector__:
        case __Ast_Statement_Initialize_Box__:
            break;
    }
    return __Result__;
}

static __Ast_Block__ *__Generic_Clone_Block__(__Semantic_Context__ *__Context__,
                                            __Ast_Block__ *__Source__,
                                            const __Text_Slice__ *__Parameters__,
                                            __Ast_Type__ *const *__Arguments__,
                                            size_t __Argument_Count__,
                                            const __Program_Unit__ *__Unit__)
{
    __Ast_Block__ *__Result__;
    size_t __Index__;
    if (__Source__ == NULL)
        return NULL;
    __Result__ = (__Ast_Block__ *)__Generic_Allocate__(
        __Context__, sizeof(*__Result__), alignof(__Ast_Block__));
    if (__Result__ == NULL)
        return NULL;
    *__Result__ = *__Source__;
    __Result__->__Statements__ = NULL;
    if (__Source__->__Statement_Count__ != 0U)
    {
        __Result__->__Statements__ = (__Ast_Statement__ **)__Generic_Allocate__(
            __Context__, __Source__->__Statement_Count__ * sizeof(__Ast_Statement__ *),
            alignof(__Ast_Statement__ *));
        if (__Result__->__Statements__ == NULL)
            return NULL;
    }
    for (__Index__ = 0U; __Index__ < __Source__->__Statement_Count__; ++__Index__)
    {
        __Result__->__Statements__[__Index__] = __Generic_Clone_Statement__(
            __Context__, __Source__->__Statements__[__Index__], __Parameters__, __Arguments__,
            __Argument_Count__, __Unit__);
        if (__Result__->__Statements__[__Index__] == NULL)
            return NULL;
    }
    return __Result__;
}

static __Ast_Function__ *__Generic_Clone_Function__(__Semantic_Context__ *__Context__,
                                                  __Ast_Function__ *__Template__,
                                                  __Ast_Type__ *const *__Arguments__,
                                                  const __Program_Unit__ *__Unit__)
{
    __Ast_Function__ *__Function__;
    size_t __Index__;
    __Function__ = (__Ast_Function__ *)__Generic_Allocate__(
        __Context__, sizeof(*__Function__), alignof(__Ast_Function__));
    if (__Function__ == NULL)
        return NULL;
    *__Function__ = *__Template__;
    __Function__->__Type_Parameters__ = NULL;
    __Function__->__Type_Parameter_Count__ = 0U;
    __Function__->__Parameters__ = NULL;
    if (__Template__->__Parameter_Count__ != 0U)
    {
        __Function__->__Parameters__ = (__Ast_Function_Parameter__ *)__Generic_Allocate__(
            __Context__, __Template__->__Parameter_Count__ * sizeof(__Ast_Function_Parameter__),
            alignof(__Ast_Function_Parameter__));
        if (__Function__->__Parameters__ == NULL)
            return NULL;
    }
    for (__Index__ = 0U; __Index__ < __Template__->__Parameter_Count__; ++__Index__)
    {
        __Function__->__Parameters__[__Index__] = __Template__->__Parameters__[__Index__];
        __Function__->__Parameters__[__Index__].__Slot__.__Type__ =
            __Semantic_Generic_Substitute_Type__(
                __Context__, __Template__->__Parameters__[__Index__].__Slot__.__Type__,
                __Template__->__Type_Parameters__, __Arguments__, __Template__->__Type_Parameter_Count__,
                __Unit__);
        if (__Function__->__Parameters__[__Index__].__Slot__.__Type__ == NULL)
            return NULL;
    }
    __Function__->__Output__.__Type__ = __Semantic_Generic_Substitute_Type__(
        __Context__, __Template__->__Output__.__Type__, __Template__->__Type_Parameters__, __Arguments__,
        __Template__->__Type_Parameter_Count__, __Unit__);
    if (__Function__->__Output__.__Type__ == NULL)
        return NULL;
    __Function__->__Body__ = __Generic_Clone_Block__(
        __Context__, __Template__->__Body__, __Template__->__Type_Parameters__, __Arguments__,
        __Template__->__Type_Parameter_Count__, __Unit__);
    if (__Template__->__Body__ != NULL && __Function__->__Body__ == NULL)
        return NULL;
    return __Function__;
}

__Semantic_Function_Entry__ *__Semantic_Generic_Function_Instance__(
    __Semantic_Context__ *__Context__,
    __Semantic_Function_Entry__ *__Template__,
    __Ast_Type__ *const *__Arguments__,
    size_t __Argument_Count__)
{
    size_t __Index__;
    __Ast_Type__ **__Persistent_Arguments__;
    if (__Context__ == NULL || __Template__ == NULL || __Template__->__Function__ == NULL ||
        !__Template__->__Is_Generic_Template__ ||
        __Argument_Count__ != __Template__->__Function__->__Type_Parameter_Count__)
    {
        return NULL;
    }
    for (__Index__ = 0U; __Index__ < __Context__->__Generic_Functions__.__Count__; ++__Index__)
    {
        __Semantic_Function_Entry__ **__Slot__ = (__Semantic_Function_Entry__ **)__Vector_At__(
            &__Context__->__Generic_Functions__, __Index__);
        __Semantic_Function_Entry__ *__Entry__ = __Slot__ == NULL ? NULL : *__Slot__;
        size_t __Argument_Index__;
        int __Equal__ = 1;
        if (__Entry__ == NULL || __Entry__->__Template_Function__ != __Template__->__Function__ ||
            __Entry__->__Type_Argument_Count__ != __Argument_Count__)
            continue;
        for (__Argument_Index__ = 0U; __Argument_Index__ < __Argument_Count__; ++__Argument_Index__)
        {
            if (!__Semantic_Generic_Type_Equal__(
                    __Context__, __Entry__->__Type_Arguments__[__Argument_Index__],
                    __Arguments__[__Argument_Index__]))
            {
                __Equal__ = 0;
                break;
            }
        }
        if (__Equal__)
            return __Entry__;
    }
    __Persistent_Arguments__ = (__Ast_Type__ **)__Generic_Allocate__(
        __Context__, __Argument_Count__ * sizeof(__Ast_Type__ *), alignof(__Ast_Type__ *));
    if (__Persistent_Arguments__ == NULL)
        return NULL;
    for (__Index__ = 0U; __Index__ < __Argument_Count__; ++__Index__)
    {
        __Persistent_Arguments__[__Index__] = __Semantic_Generic_Substitute_Type__(
            __Context__, __Arguments__[__Index__], NULL, NULL, 0U, __Template__->__Unit__);
        if (__Persistent_Arguments__[__Index__] == NULL)
            return NULL;
    }
    {
        __Semantic_Function_Entry__ *__Entry__ =
            (__Semantic_Function_Entry__ *)__Generic_Allocate__(
                __Context__, sizeof(__Semantic_Function_Entry__), alignof(__Semantic_Function_Entry__));
        if (__Entry__ == NULL)
            return NULL;
        __Entry__->__Name__ = __Template__->__Name__;
        __Entry__->__Template_Function__ = __Template__->__Function__;
        __Entry__->__Type_Arguments__ = __Persistent_Arguments__;
        __Entry__->__Type_Argument_Count__ = __Argument_Count__;
        __Entry__->__Unit__ = __Template__->__Unit__;
        __Entry__->__Public__ = __Template__->__Public__;
        __Entry__->__Index__ = __Context__->__Functions__.__Count__ +
                              __Context__->__Generic_Functions__.__Count__;
        __Entry__->__Reference_Return_Parameter__ = SIZE_MAX;
        __Entry__->__Composite_View_Return_Parameter__ = SIZE_MAX;
        __Entry__->__Function__ = __Generic_Clone_Function__(
            __Context__, __Template__->__Function__, __Persistent_Arguments__, __Template__->__Unit__);
        if (__Entry__->__Function__ == NULL)
            return NULL;
        __Entry__->__Callable_Type__ =
            __Semantic_Make_Function_Type__(__Context__, __Entry__->__Function__);
        if (__Entry__->__Callable_Type__ == NULL ||
            __Vector_Push__(&__Context__->__Generic_Functions__, &__Entry__) == NULL)
            return NULL;
        return __Entry__;
    }
}

__Semantic_Function_Entry__ *__Semantic_Generic_Function_At__(__Semantic_Context__ *__Context__,
                                                               size_t __Index__)
{
    __Semantic_Function_Entry__ **__Slot__;
    if (__Context__ == NULL || __Index__ >= __Context__->__Generic_Functions__.__Count__)
        return NULL;
    __Slot__ = (__Semantic_Function_Entry__ **)__Vector_At__(
        &__Context__->__Generic_Functions__, __Index__);
    return __Slot__ == NULL ? NULL : *__Slot__;
}

