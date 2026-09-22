/* GENERATED FILE — DO NOT EDIT.
 * Owner: tools/generate/registry/type_registry.def
 * Frontend-owned Stage-0 token-to-AST primitive-type syntax adapter.
 * Regenerate with: python3 tools/generate/generate_type_builtins.py
 */

#include "frontend/parser/type_internal.h"
#include "core/source.h"

int __Parser_Type_Primitive__(__Token_Kind__ __Token_Value__,
                              __Ast_Type_Kind__ *__Out_Kind__,
                              __Machine_Type__ *__Out_Machine__)
{
    if (__Out_Kind__ == NULL || __Out_Machine__ == NULL)
    {
        return 0;
    }

    switch (__Token_Value__)
    {
        case __Token_ANY__:
            *__Out_Kind__ = __Ast_Type_Any__;
            return 1;
        case __Token_BOOL__:
            *__Out_Kind__ = __Ast_Type_Boolean__;
            return 1;
        case __Token_STRING__:
            *__Out_Kind__ = __Ast_Type_String__;
            return 1;
        case __Token_CHAR__:
            *__Out_Kind__ = __Ast_Type_Character__;
            return 1;
        case __Token_INT__:
            *__Out_Kind__ = __Ast_Type_Integer__;
            return 1;
        case __Token_UINT__:
            *__Out_Kind__ = __Ast_Type_Unsigned_Integer__;
            return 1;
        case __Token_VOID__:
            *__Out_Kind__ = __Ast_Type_Void__;
            return 1;
        case __Token_I8__:
            *__Out_Kind__ = __Ast_Type_Machine__;
            *__Out_Machine__ = __Machine_I8__;
            return 1;
        case __Token_I16__:
            *__Out_Kind__ = __Ast_Type_Machine__;
            *__Out_Machine__ = __Machine_I16__;
            return 1;
        case __Token_I32__:
            *__Out_Kind__ = __Ast_Type_Machine__;
            *__Out_Machine__ = __Machine_I32__;
            return 1;
        case __Token_I64__:
            *__Out_Kind__ = __Ast_Type_Machine__;
            *__Out_Machine__ = __Machine_I64__;
            return 1;
        case __Token_U8__:
            *__Out_Kind__ = __Ast_Type_Machine__;
            *__Out_Machine__ = __Machine_U8__;
            return 1;
        case __Token_U16__:
            *__Out_Kind__ = __Ast_Type_Machine__;
            *__Out_Machine__ = __Machine_U16__;
            return 1;
        case __Token_U32__:
            *__Out_Kind__ = __Ast_Type_Machine__;
            *__Out_Machine__ = __Machine_U32__;
            return 1;
        case __Token_U64__:
            *__Out_Kind__ = __Ast_Type_Machine__;
            *__Out_Machine__ = __Machine_U64__;
            return 1;
        case __Token_F32__:
            *__Out_Kind__ = __Ast_Type_Machine__;
            *__Out_Machine__ = __Machine_F32__;
            return 1;
        case __Token_F64__:
            *__Out_Kind__ = __Ast_Type_Machine__;
            *__Out_Machine__ = __Machine_F64__;
            return 1;
        default:
            return 0;
    }
}

int __Parser_Type_Contextual__(__Text_Slice__ __Name__, __Ast_Type_Kind__ *__Out_Kind__)
{
    if (__Out_Kind__ == NULL)
    {
        return 0;
    }
    if (__Text_Equals_Cstr__(__Name__, "option") ||
        __Text_Equals_Cstr__(__Name__, "اختيار"))
    {
        *__Out_Kind__ = __Ast_Type_Option__;
        return 1;
    }
    if (__Text_Equals_Cstr__(__Name__, "result") ||
        __Text_Equals_Cstr__(__Name__, "نتيجة"))
    {
        *__Out_Kind__ = __Ast_Type_Result__;
        return 1;
    }
    if (__Text_Equals_Cstr__(__Name__, "vector") ||
        __Text_Equals_Cstr__(__Name__, "متجه"))
    {
        *__Out_Kind__ = __Ast_Type_Vector__;
        return 1;
    }
    return 0;
}
