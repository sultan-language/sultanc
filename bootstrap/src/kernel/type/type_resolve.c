#include "kernel/type/type.h"
#include "kernel/type/builtin.h"
#include "semantic/type_named.h"
#include "semantic/diagnostic.h"

#include <string.h>

static __Resolved_Type_Kind__
__Type_Resolved_Kind_From_Builtin_Class__(__Type_Builtin_Class__ __Class__)
{
    switch (__Class__)
    {
        case __Type_Builtin_Class_Unknown__:
            return __Resolved_Type_Unknown__;
        case __Type_Builtin_Class_Boolean__:
            return __Resolved_Type_Boolean__;
        case __Type_Builtin_Class_Signed_Integer__:
            return __Resolved_Type_Signed_Integer__;
        case __Type_Builtin_Class_Unsigned_Integer__:
            return __Resolved_Type_Unsigned_Integer__;
        case __Type_Builtin_Class_Float__:
            return __Resolved_Type_Float__;
        case __Type_Builtin_Class_Character__:
            return __Resolved_Type_Character__;
        case __Type_Builtin_Class_String__:
            return __Resolved_Type_String__;
        case __Type_Builtin_Class_Void__:
            return __Resolved_Type_Void__;
    }
    return __Resolved_Type_Unknown__;
}

__Type_Builtin_Id__ __Type_Builtin_Id_From_Ast__(const __Ast_Type__ *__Type__)
{
    if (__Type__ == NULL)
    {
        return __Type_Builtin_Invalid__;
    }

    switch (__Type__->__Kind__)
    {
#define SULTANC__TYPE_BUILTIN__(                                                                   \
    Id, Token, AstKind, ResolvedKind, Bits, Scalar, Signed, Stage1, Name)                          \
    case __Ast_Type_##AstKind##__:                                                                 \
        return __Type_Builtin_##Id##__;
#define SULTANC__TYPE_MACHINE__(                                                                   \
    Id, Token, Machine, ResolvedKind, Bits, Scalar, Signed, Stage1, Name)
#include "kernel/type/type_registry.def"
#undef SULTANC__TYPE_MACHINE__
#undef SULTANC__TYPE_BUILTIN__
        default:
            break;
    }

    if (__Type__->__Kind__ != __Ast_Type_Machine__)
    {
        return __Type_Builtin_Invalid__;
    }

    switch (__Type__->__As__.__Machine__)
    {
#define SULTANC__TYPE_BUILTIN__(                                                                   \
    Id, Token, AstKind, ResolvedKind, Bits, Scalar, Signed, Stage1, Name)
#define SULTANC__TYPE_MACHINE__(                                                                   \
    Id, Token, Machine, ResolvedKind, Bits, Scalar, Signed, Stage1, Name)                          \
    case __Machine_##Machine##__:                                                                  \
        return __Type_Builtin_##Id##__;
#include "kernel/type/type_registry.def"
#undef SULTANC__TYPE_MACHINE__
#undef SULTANC__TYPE_BUILTIN__
    }
    return __Type_Builtin_Invalid__;
}

int __Type_Resolve__(__Semantic_Context__ *__Context__,
                     __Ast_Type__ *__Type__,
                     __Resolved_Type__ *__Out_Type__)
{
    __Ast_Type__ *__Original__ = __Type__;
    __Ast_Type__ *__Base__ = __Type_Unwrap_Mutable__(__Type__);
    __Type_Builtin_Id__ __Builtin_Id__;
    const __Type_Builtin_Descriptor__ *__Builtin__;

    if (__Out_Type__ == NULL)
    {
        return 0;
    }
    memset(__Out_Type__, 0, sizeof(*__Out_Type__));
    __Out_Type__->__Builtin__ = __Type_Builtin_Invalid__;
    if (__Base__ == NULL)
    {
        return 0;
    }
    __Out_Type__->__Ast__ = __Base__;
    __Out_Type__->__Mutable__ = (__Original__ != __Base__);

    __Builtin_Id__ = __Type_Builtin_Id_From_Ast__(__Base__);
    __Builtin__ = __Type_Builtin_Get_Descriptor__(__Builtin_Id__);
    if (__Builtin__ != NULL)
    {
        __Out_Type__->__Builtin__ = __Builtin__->__Id__;
        __Out_Type__->__Kind__ = __Type_Resolved_Kind_From_Builtin_Class__(__Builtin__->__Class__);
        __Out_Type__->__Bits__ = __Builtin__->__Bits__;
        return 1;
    }

    switch (__Base__->__Kind__)
    {
        case __Ast_Type_Reference__:
            __Out_Type__->__Kind__ = __Resolved_Type_Reference__;
            __Out_Type__->__Reference_Mutable__ =
                __Base__->__As__.__Inner__ != NULL &&
                __Base__->__As__.__Inner__->__Kind__ == __Ast_Type_Mutable__;
            __Out_Type__->__Inner__ = __Type_Unwrap_Mutable__(__Base__->__As__.__Inner__);
            break;
        case __Ast_Type_Vector__:
            __Out_Type__->__Kind__ = __Resolved_Type_Vector__;
            __Out_Type__->__Inner__ = __Base__->__As__.__Inner__;
            break;
        case __Ast_Type_Box__:
            __Out_Type__->__Kind__ = __Resolved_Type_Box__;
            __Out_Type__->__Inner__ = __Base__->__As__.__Inner__;
            break;
        case __Ast_Type_Option__:
            __Out_Type__->__Kind__ = __Resolved_Type_Option__;
            __Out_Type__->__Inner__ = __Base__->__As__.__Inner__;
            break;
        case __Ast_Type_Result__:
            __Out_Type__->__Kind__ = __Resolved_Type_Result__;
            break;
        case __Ast_Type_Named__:
        {
            __Semantic_Type_Entry__ *__Entry__ = NULL;
            if (!__Semantic_Resolve_Named_Entry__(__Context__, __Base__, &__Entry__) ||
                __Entry__ == NULL || __Entry__->__Declaration__ == NULL)
            {
                return 0;
            }
            __Out_Type__->__Named__ = __Entry__;
            if (__Entry__->__Declaration__->__Kind__ == __Ast_Type_Decl_Struct__)
            {
                __Out_Type__->__Kind__ = __Resolved_Type_Struct__;
            }
            else
            {
                __Out_Type__->__Kind__ = __Resolved_Type_Enum__;
            }
            break;
        }
        case __Ast_Type_Mutable__:
        case __Ast_Type_Any__:
        case __Ast_Type_Boolean__:
        case __Ast_Type_Machine__:
        case __Ast_Type_Integer__:
        case __Ast_Type_Unsigned_Integer__:
        case __Ast_Type_Character__:
        case __Ast_Type_String__:
        case __Ast_Type_Void__:
            return 0;
    }
    return 1;
}
