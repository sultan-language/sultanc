#include "frontend/ast/storage.h"
#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/statement_internal.h"

#include <stdalign.h>

int __Parser_Parse_Lvalue_Statement__(__Parser__ *__Parser_State__, __Vector__ *__Statements__)
{
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    int __Leading_Dereference__ = __Parser_Accept__(__Parser_State__, __Token_STAR_OPERATOR__);
    __Ast_Lvalue__ *__Lvalue__ = __Parser_Parse_Lvalue__(__Parser_State__);
    if (__Lvalue__ == NULL)
    {
        return 0;
    }
    if (__Leading_Dereference__)
    {
        __Ast_Lvalue__ *__Dereference__ = (__Ast_Lvalue__ *)__Ast_Allocate__(
            __Parser_State__->__Ast__, sizeof(*__Dereference__), alignof(__Ast_Lvalue__));
        if (__Dereference__ == NULL)
        {
            return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
        }
        __Dereference__->__Header__ =
            __Ast_New_Header__(__Parser_State__->__Ast__,
                               __Parser_Span__(__Start__, __Lvalue__->__Header__.__Span__.__End__));
        __Dereference__->__Kind__ = __Ast_Lvalue_Dereference__;
        __Dereference__->__As__.__Dereference_Parent__ = __Lvalue__;
        __Lvalue__ = __Dereference__;
    }
    if (!__Leading_Dereference__ &&
        __Parser_State__->__Current__.__Kind__ == __Token_LEFT_PARENTHESIS_OPERATOR__)
    {
        __Ast_Expression__ *__Call__ =
            __Parser_Call_After_Lvalue__(__Parser_State__, __Lvalue__, __Start__);
        __Temporary_Id__ __Temporary__ = __Parser_State__->__Next_Temporary__++;
        __Source_Span__ __Span__;
        __Ast_Statement__ *__Declaration__ = NULL;
        __Ast_Statement__ *__Copy__ = NULL;
        __Ast_Lvalue__ *__Temporary_Lvalue__ = NULL;
        __Text_Slice__ __Empty__ = {NULL, 0U};
        if (__Call__ == NULL || !__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
        {
            return 0;
        }
        __Span__ = __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__);
        __Declaration__ = __Parser_New_Statement__(
            __Parser_State__, __Ast_Statement_Variable_Declaration__, __Span__);
        if (__Declaration__ == NULL)
        {
            return 0;
        }
        __Declaration__->__As__.__Variable__.__Name_Kind__ = __Ast_Lvalue_Base_Temporary__;
        __Declaration__->__As__.__Variable__.__Name__.__Temporary__ = __Temporary__;
        __Declaration__->__As__.__Variable__.__Slot__.__Header__ =
            __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
        __Declaration__->__As__.__Variable__.__Slot__.__Type__ = NULL;
        if (!__Parser_Push_Statement__(__Parser_State__, __Statements__, __Declaration__))
        {
            return 0;
        }
        __Temporary_Lvalue__ = __Parser_New_Base_Lvalue__(
            __Parser_State__, __Ast_Lvalue_Base_Temporary__, __Empty__, __Temporary__, __Span__);
        if (__Temporary_Lvalue__ == NULL)
        {
            return 0;
        }
        __Copy__ = __Parser_New_Statement__(__Parser_State__, __Ast_Statement_Copy__, __Span__);
        if (__Copy__ == NULL)
        {
            return 0;
        }
        __Copy__->__As__.__Copy__.__Destination__ = __Temporary_Lvalue__;
        __Copy__->__As__.__Copy__.__Expression__ = __Call__;
        return __Parser_Push_Statement__(__Parser_State__, __Statements__, __Copy__);
    }

    if (!__Parser_Expect__(__Parser_State__, __Token_OPERATOR_ASSIGNMENT_COLON__))
    {
        return 0;
    }
    {
        __Ast_Expression__ *__Expression__ = __Parser_Parse_Expression__(__Parser_State__);
        __Ast_Statement__ *__Statement__ = NULL;
        if (__Expression__ == NULL ||
            !__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
        {
            return 0;
        }
        __Statement__ = __Parser_New_Statement__(
            __Parser_State__,
            __Ast_Statement_Copy__,
            __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
        if (__Statement__ == NULL)
        {
            return 0;
        }
        __Statement__->__As__.__Copy__.__Destination__ = __Lvalue__;
        __Statement__->__As__.__Copy__.__Expression__ = __Expression__;
        return __Parser_Push_Statement__(__Parser_State__, __Statements__, __Statement__);
    }
}
