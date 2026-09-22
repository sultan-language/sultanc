#include "frontend/ast/storage.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/statement_internal.h"

#include <stdalign.h>

__Ast_Block__ *__Parser_Block_From_Statement__(__Parser__ *__Parser_State__,
                                               __Ast_Statement__ *__Statement__)
{
    __Ast_Block__ *__Block__ = (__Ast_Block__ *)__Ast_Allocate__(
        __Parser_State__->__Ast__, sizeof(*__Block__), alignof(__Ast_Block__));
    if (__Block__ == NULL)
    {
        (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
        return NULL;
    }
    __Block__->__Header__ =
        __Ast_New_Header__(__Parser_State__->__Ast__, __Statement__->__Header__.__Span__);
    __Block__->__Statement_Count__ = 1U;
    __Block__->__Statements__ =
        (__Ast_Statement__ **)__Ast_Copy_Array__(__Parser_State__->__Ast__,
                                                 &__Statement__,
                                                 1U,
                                                 sizeof(__Statement__),
                                                 alignof(__Ast_Statement__ *));
    if (__Block__->__Statements__ == NULL)
    {
        (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
        return NULL;
    }
    return __Block__;
}
