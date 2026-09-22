#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/pattern.h"
#include "frontend/parser/span.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/statement_internal.h"

#include <stdalign.h>

int __Parser_Parse_Match__(__Parser__ *__Parser_State__, __Vector__ *__Statements__)
{
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    __Ast_Expression__ *__Value__ = NULL;
    __Vector__ __Cases__;
    __Ast_Statement__ *__Statement__ = NULL;

    __Vector_Init__(&__Cases__, sizeof(__Ast_Match_Case__));
    if (!__Parser_Advance__(__Parser_State__))
    {
        __Vector_Destroy__(&__Cases__);
        return 0;
    }

    __Value__ = __Parser_Parse_Expression__(__Parser_State__);
    if (__Value__ == NULL || !__Parser_Expect__(__Parser_State__, __Token_WITH__))
    {
        __Vector_Destroy__(&__Cases__);
        return 0;
    }

    while (__Parser_Accept__(__Parser_State__, __Token_OR_OPERATOR__))
    {
        __Ast_Match_Case__ __Case__ = {0};

        __Case__.__Pattern__ = __Parser_Parse_Pattern__(__Parser_State__);
        if (__Case__.__Pattern__ == NULL ||
            !__Parser_Expect__(__Parser_State__, __Token_RIGHT_ARROW_OPERATOR__))
        {
            __Vector_Destroy__(&__Cases__);
            return 0;
        }

        __Case__.__Body__ = __Parser_Parse_Block__(__Parser_State__);
        if (__Case__.__Body__ == NULL || __Vector_Push__(&__Cases__, &__Case__) == NULL)
        {
            __Vector_Destroy__(&__Cases__);
            return 0;
        }
    }

    if (__Cases__.__Count__ == 0U)
    {
        __Vector_Destroy__(&__Cases__);
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Match_Requires_Case__);
    }
    (void)__Parser_Accept__(__Parser_State__, __Token_SEMICOLON_OPERATOR__);

    __Statement__ = __Parser_New_Statement__(
        __Parser_State__,
        __Ast_Statement_Match__,
        __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
    if (__Statement__ == NULL)
    {
        __Vector_Destroy__(&__Cases__);
        return 0;
    }

    __Statement__->__As__.__Match__.__Value__ = __Value__;
    __Statement__->__As__.__Match__.__Case_Count__ = __Cases__.__Count__;
    __Statement__->__As__.__Match__.__Cases__ = (__Ast_Match_Case__ *)__Parser_Freeze_Vector__(
        __Parser_State__, &__Cases__, alignof(__Ast_Match_Case__));
    __Vector_Destroy__(&__Cases__);
    return __Parser_Push_Statement__(__Parser_State__, __Statements__, __Statement__);
}
