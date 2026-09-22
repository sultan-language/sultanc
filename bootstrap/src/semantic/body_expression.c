#include "semantic/body_internal.h"

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
