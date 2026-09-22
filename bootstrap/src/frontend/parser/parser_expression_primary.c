#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/expression_internal.h"

#include <stdalign.h>

__Ast_Expression__ *__Parser_Parse_Primary__(__Parser__ *__Parser_State__)
{
    __Source_Position__ __Start__;
    if (__Parser_Accept__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
    {
        __Ast_Expression__ *__Expression__ = __Parser_Parse_Expression_Min__(__Parser_State__, 1);
        if (__Expression__ == NULL ||
            !__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
        {
            return NULL;
        }
        return __Expression__;
    }
    if (__Parser_State__->__Current__.__Kind__ == __Token_IDENTIFIER__)
    {
        __Ast_Lvalue__ *__Function_Or_Value__ = NULL;
        __Ast_Expression__ *__Expression__ = NULL;
        __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
        __Function_Or_Value__ = __Parser_Parse_Lvalue__(__Parser_State__);
        if (__Function_Or_Value__ == NULL)
        {
            return NULL;
        }
        if (__Parser_Accept__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
        {
            __Vector__ __Arguments__;
            __Vector_Init__(&__Arguments__, sizeof(__Ast_Expression__ *));
            if (__Parser_State__->__Current__.__Kind__ != __Token_RIGHT_PARENTHESIS_OPERATOR__)
            {
                for (;;)
                {
                    __Ast_Expression__ *__Argument__ =
                        __Parser_Parse_Expression_Min__(__Parser_State__, 1);
                    if (__Argument__ == NULL ||
                        __Vector_Push__(&__Arguments__, &__Argument__) == NULL)
                    {
                        __Vector_Destroy__(&__Arguments__);
                        if (!__Parser_State__->__Failed__)
                        {
                            (void)__Parser_Fail_Internal__(__Parser_State__,
                                                           __Diag_Word_Syntax_Internal_Oom__);
                        }
                        return NULL;
                    }
                    if (!__Parser_Accept__(__Parser_State__, __Token_COMMA_OPERATOR__))
                    {
                        break;
                    }
                }
            }
            if (!__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
            {
                __Vector_Destroy__(&__Arguments__);
                return NULL;
            }
            __Expression__ = __Parser_New_Expression__(
                __Parser_State__,
                __Ast_Expression_Call__,
                __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
            if (__Expression__ == NULL)
            {
                __Vector_Destroy__(&__Arguments__);
                return NULL;
            }
            __Expression__->__As__.__Call__.__Function__ = __Function_Or_Value__;
            __Expression__->__As__.__Call__.__Argument_Count__ = __Arguments__.__Count__;
            __Expression__->__As__.__Call__.__Arguments__ =
                (__Ast_Expression__ **)__Parser_Freeze_Vector__(
                    __Parser_State__, &__Arguments__, alignof(__Ast_Expression__ *));
            __Vector_Destroy__(&__Arguments__);
            return __Expression__;
        }
        __Expression__ = __Parser_New_Expression__(
            __Parser_State__, __Ast_Expression_Atom__, __Function_Or_Value__->__Header__.__Span__);
        if (__Expression__ == NULL)
        {
            return NULL;
        }
        __Expression__->__As__.__Atom__.__Kind__ = __Ast_Atom_Lvalue__;
        __Expression__->__As__.__Atom__.__As__.__Lvalue__ = __Function_Or_Value__;
        return __Expression__;
    }
    {
        int __Ok__ = 0;
        __Ast_Atom__ __Atom__ = __Parser_Parse_Atom__(__Parser_State__, &__Ok__);
        __Ast_Expression__ *__Expression__ = NULL;
        if (!__Ok__)
        {
            return NULL;
        }
        if (__Atom__.__Kind__ == __Ast_Atom_Literal__)
        {
            __Expression__ =
                __Parser_New_Expression__(__Parser_State__,
                                          __Ast_Expression_Atom__,
                                          __Atom__.__As__.__Literal__->__Header__.__Span__);
        }
        else
        {
            __Expression__ =
                __Parser_New_Expression__(__Parser_State__,
                                          __Ast_Expression_Atom__,
                                          __Atom__.__As__.__Lvalue__->__Header__.__Span__);
        }
        if (__Expression__ == NULL)
        {
            return NULL;
        }
        __Expression__->__As__.__Atom__ = __Atom__;
        return __Expression__;
    }
}
