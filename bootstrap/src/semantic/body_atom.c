#include "semantic/body_internal.h"

int __Body_Infer_Atom__(__Semantic_Body_Context__ *__Context__,
                        const __Ast_Atom__ *__Atom__,
                        __Ast_Type__ **__Out_Type__)
{
    if (__Atom__ == NULL)
    {
        return 0;
    }
    if (__Atom__->__Kind__ == __Ast_Atom_Lvalue__)
    {
        return __Body_Infer_Lvalue__(__Context__, __Atom__->__As__.__Lvalue__, __Out_Type__, NULL);
    }
    if (__Atom__->__As__.__Literal__ == NULL)
    {
        return 0;
    }
    switch (__Atom__->__As__.__Literal__->__Kind__)
    {
        case __Ast_Literal_Boolean__:
            *__Out_Type__ = &__Body_Builtin_Bool_Type__;
            return 1;
        case __Ast_Literal_Integer__:
            *__Out_Type__ = &__Body_Builtin_Int_Type__;
            return 1;
        case __Ast_Literal_Character__:
            *__Out_Type__ = &__Body_Builtin_Char_Type__;
            return 1;
        case __Ast_Literal_String__:
            *__Out_Type__ = &__Body_Builtin_String_Type__;
            return 1;
        case __Ast_Literal_Undefined__:
            return __Body_Fail__(__Context__,
                                 __E0600_Undefined_Literal_In_Lowering__,
                                 __Atom__->__As__.__Literal__->__Header__.__Span__);
    }
    return 0;
}
