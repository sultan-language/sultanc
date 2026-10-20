/* Parses module-level external data declarations.
 *
 * Canonical owner: compiler/frontend/parser/declarations.sn
 * (حلل_عنصر_قيمة_عليا with خارجية). The grammar is
 *
 *   ["عام"] "خارجي" "دع" name ":" type "=" text ";"
 *
 * External data is an imported linker symbol, never an initialized value. The
 * text literal is the exact native symbol name, so punctuation in linker names
 * needs no identifier escape. Native linker names are nonempty byte strings
 * without embedded NUL. */

#include "frontend/ast/storage.h"
#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/module_internal.h"
#include "frontend/parser/span.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/token_payload.h"

#include <stdalign.h>
#include <string.h>

/* Parses one external data item. The current token must be "دع"; the caller
 * has already consumed "خارجي" (and "عام" when public). */
int __Parser_Parse_Global_Item__(__Parser__ *__Parser_State__,
                                 __Vector__ *__Items__,
                                 int __Public__,
                                 __Source_Position__ __Start__)
{
    /* Stores the declared name. */
    __Text_Slice__ __Name__;
    /* References the declared type. */
    __Ast_Type__ *__Type__ = NULL;
    /* Stores the decoded linker symbol bytes. */
    __Text_Slice__ __Link_Text__;
    /* References the declaration. */
    __Ast_Global__ *__Global__ = NULL;
    /* References the item. */
    __Ast_Module_Item__ *__Item__ = NULL;
    /* Stores the declaration span. */
    __Source_Span__ __Span__;

    __Link_Text__.__Data__ = NULL;
    __Link_Text__.__Length__ = 0U;
    if (!__Parser_Expect__(__Parser_State__, __Token_LET__))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
    {
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Variable_Name__);
    }
    __Name__ = __Parser_State__->__Current__.__Lexeme__;
    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_COLON_OPERATOR__))
    {
        return 0;
    }
    __Type__ = __Parser_Parse_Type__(__Parser_State__);
    if (__Type__ == NULL)
    {
        return 0;
    }
    if (!__Parser_Expect__(__Parser_State__, __Token_DEFINITION_SEPARATOR__))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_LIT_STR__)
    {
        return __Parser_Fail__(__Parser_State__,
                               __Diag_Word_Syntax_Expected_External_Symbol_Name__);
    }
    if (!__Parser_Current_Text__(__Parser_State__, &__Link_Text__))
    {
        return 0;
    }
    if (__Link_Text__.__Length__ == 0U ||
        memchr(__Link_Text__.__Data__, '\0', __Link_Text__.__Length__) != NULL)
    {
        return __Parser_Fail__(__Parser_State__,
                               __Diag_Word_Syntax_Expected_External_Symbol_Name__);
    }
    /* The decoded literal lives in lexer storage; the AST owns its own copy. */
    __Link_Text__ = __Parser_Copy_Literal_Text__(__Parser_State__, __Link_Text__);
    if (__Link_Text__.__Data__ == NULL)
    {
        return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
    }
    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
    {
        return 0;
    }

    __Span__ = __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__);
    __Global__ = (__Ast_Global__ *)__Ast_Allocate__(
        __Parser_State__->__Ast__, sizeof(*__Global__), alignof(__Ast_Global__));
    if (__Global__ == NULL)
    {
        return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
    }
    memset(__Global__, 0, sizeof(*__Global__));
    __Global__->__Header__ = __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
    __Global__->__Type__ = __Type__;
    __Global__->__Link_Name__ = __Link_Text__;
    __Global__->__External__ = 1;

    __Item__ = __Parser_New_Module_Item__(
        __Parser_State__, __Ast_Module_Item_Global__, __Name__, __Span__);
    if (__Item__ == NULL)
    {
        return 0;
    }
    __Item__->__Public__ = __Public__;
    __Item__->__As__.__Global__ = __Global__;
    return __Parser_Push_Module_Item__(__Parser_State__, __Items__, __Item__);
}
