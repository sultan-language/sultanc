/* Parses while statements. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/statement_internal.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/ast_factory.h"

#include <stdalign.h>

/* Parses the parser while. */
int __Parser_Parse_While__(__Parser__ *__Parser_State__, __Vector__ *__Statements__)
{
    /* Stores the start. */
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    /* References the condition. */
    __Ast_Expression__ *__Condition__ = NULL;
    /* References the body. */
    __Ast_Block__ *__Body__ = NULL;
    /* References the statement. */
    __Ast_Statement__ *__Statement__ = NULL;
    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
    {
        return 0;
    }
    __Condition__ = __Parser_Parse_Expression__(__Parser_State__);
    if (__Condition__ == NULL ||
        !__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
    {
        return 0;
    }
    __Body__ = __Parser_Parse_Block__(__Parser_State__);
    if (__Body__ == NULL)
    {
        return 0;
    }
    __Statement__ =
        __Parser_New_Statement__(__Parser_State__,
                                 __Ast_Statement_While__,
                                 __Parser_Span__(__Start__, __Body__->__Header__.__Span__.__End__));
    if (__Statement__ == NULL)
    {
        return 0;
    }
    __Statement__->__As__.__While__.__Condition__ = __Condition__;
    __Statement__->__As__.__While__.__Body__ = __Body__;
    (void)__Parser_Accept__(__Parser_State__, __Token_SEMICOLON_OPERATOR__);
    return __Parser_Push_Statement__(__Parser_State__, __Statements__, __Statement__);
}

/* Parses `لكل (دع name: type = value; condition; place := value) body`
 * (canonical حلل_لكل): a nested scope holding the initializer and a while
 * loop whose step runs after the body and on continue. */
int __Parser_Parse_For__(__Parser__ *__Parser_State__, __Vector__ *__Statements__)
{
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    __Vector__ __Scope__;
    __Ast_Expression__ *__Condition__ = NULL;
    __Ast_Lvalue__ *__Step_Place__ = NULL;
    __Ast_Expression__ *__Step_Value__ = NULL;
    __Ast_Statement__ *__Step__ = NULL;
    __Ast_Block__ *__Body__ = NULL;
    __Ast_Statement__ *__Loop__ = NULL;
    __Ast_Block__ *__Scope_Block__ = NULL;
    __Ast_Statement__ *__Statement__ = NULL;
    __Source_Position__ __Step_Start__;
    int __Leading_Dereference__;

    __Vector_Init__(&__Scope__, sizeof(__Ast_Statement__ *));
    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
    {
        __Vector_Destroy__(&__Scope__);
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_LET__)
    {
        __Vector_Destroy__(&__Scope__);
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Statement__);
    }
    if (!__Parser_Parse_Variable__(__Parser_State__, &__Scope__))
    {
        __Vector_Destroy__(&__Scope__);
        return 0;
    }
    __Condition__ = __Parser_Parse_Expression__(__Parser_State__);
    if (__Condition__ == NULL ||
        !__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
    {
        __Vector_Destroy__(&__Scope__);
        return 0;
    }
    __Step_Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    __Leading_Dereference__ = __Parser_Accept__(__Parser_State__, __Token_STAR_OPERATOR__);
    __Step_Place__ = __Parser_Parse_Lvalue__(__Parser_State__);
    if (__Step_Place__ == NULL)
    {
        __Vector_Destroy__(&__Scope__);
        return 0;
    }
    if (__Leading_Dereference__)
    {
        __Ast_Lvalue__ *__Dereference__ = __Parser_New_Lvalue__(
            __Parser_State__, __Ast_Lvalue_Dereference__,
            __Parser_Span__(__Step_Start__, __Step_Place__->__Header__.__Span__.__End__));
        if (__Dereference__ == NULL)
        {
            __Vector_Destroy__(&__Scope__);
            return 0;
        }
        __Dereference__->__As__.__Dereference_Parent__ = __Step_Place__;
        __Step_Place__ = __Dereference__;
    }
    if (!__Parser_Expect__(__Parser_State__, __Token_OPERATOR_ASSIGNMENT_COLON__) ||
        (__Step_Value__ = __Parser_Parse_Expression__(__Parser_State__)) == NULL)
    {
        __Vector_Destroy__(&__Scope__);
        return 0;
    }
    __Step__ = __Parser_New_Statement__(
        __Parser_State__, __Ast_Statement_Copy__,
        __Parser_Span__(__Step_Start__, __Parser_State__->__Previous__.__Span__.__End__));
    if (__Step__ == NULL ||
        !__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
    {
        __Vector_Destroy__(&__Scope__);
        return 0;
    }
    __Step__->__As__.__Copy__.__Destination__ = __Step_Place__;
    __Step__->__As__.__Copy__.__Expression__ = __Step_Value__;
    __Body__ = __Parser_Parse_Block__(__Parser_State__);
    if (__Body__ == NULL)
    {
        __Vector_Destroy__(&__Scope__);
        return 0;
    }
    __Loop__ = __Parser_New_Statement__(
        __Parser_State__, __Ast_Statement_While__,
        __Parser_Span__(__Start__, __Body__->__Header__.__Span__.__End__));
    if (__Loop__ == NULL)
    {
        __Vector_Destroy__(&__Scope__);
        return 0;
    }
    __Loop__->__As__.__While__.__Condition__ = __Condition__;
    __Loop__->__As__.__While__.__Body__ = __Body__;
    __Loop__->__As__.__While__.__Step__ = __Parser_Block_From_Statement__(__Parser_State__, __Step__);
    if (__Loop__->__As__.__While__.__Step__ == NULL ||
        !__Parser_Push_Statement__(__Parser_State__, &__Scope__, __Loop__))
    {
        __Vector_Destroy__(&__Scope__);
        return 0;
    }
    __Scope_Block__ = __Parser_New_Block__(__Parser_State__, __Loop__->__Header__.__Span__);
    if (__Scope_Block__ == NULL)
    {
        __Vector_Destroy__(&__Scope__);
        return 0;
    }
    __Scope_Block__->__Statement_Count__ = __Scope__.__Count__;
    __Scope_Block__->__Statements__ = (__Ast_Statement__ **)__Parser_Freeze_Vector__(
        __Parser_State__, &__Scope__, alignof(__Ast_Statement__ *));
    __Vector_Destroy__(&__Scope__);
    __Statement__ = __Parser_New_Statement__(__Parser_State__, __Ast_Statement_Block__,
                                             __Loop__->__Header__.__Span__);
    if (__Statement__ == NULL)
    {
        return 0;
    }
    __Statement__->__As__.__Block__ = __Scope_Block__;
    (void)__Parser_Accept__(__Parser_State__, __Token_SEMICOLON_OPERATOR__);
    return __Parser_Push_Statement__(__Parser_State__, __Statements__, __Statement__);
}
