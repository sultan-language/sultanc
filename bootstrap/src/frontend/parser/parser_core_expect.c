/* Validates required parser tokens. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/internal.h"

#include <stddef.h>

/* Returns the parser expected token spelling. */
static const char *__Parser_Expected_Token_Spelling__(__Token_Kind__ __Kind__)
{
    switch (__Kind__)
    {
        case __Token_LEFT_PARENTHESIS_OPERATOR__:
            return "(";
        case __Token_RIGHT_PARENTHESIS_OPERATOR__:
            return ")";
        case __Token_LEFT_BRACE_OPERATOR__:
            return "{";
        case __Token_RIGHT_BRACE_OPERATOR__:
            return "}";
        case __Token_LEFT_BRACKET_OPERATOR__:
            return "[";
        case __Token_RIGHT_BRACKET_OPERATOR__:
            return "]";
        case __Token_COMMA_OPERATOR__:
            return ",";
        case __Token_SEMICOLON_OPERATOR__:
            return ";";
        case __Token_COLON_OPERATOR__:
            return ":";
        case __Token_OPERATOR_ASSIGNMENT_COLON__:
            return ":=";
        case __Token_DEFINITION_SEPARATOR__:
            return "=";
        case __Token_EQUALS_OPERATOR__:
            return "==";
        case __Token_DOT_OPERATOR__:
            return ".";
        case __Token_GREATER_THAN_OPERATOR__:
            return ">";
        case __Token_RIGHT_ARROW_OPERATOR__:
            return "->";
        case __Token_WITH__:
        case __Token_FUNCTION_DEF__:
            return "keyword";
        default:
            return "token";
    }
}

/* Sets the parser found argument. */
static int __Parser_Set_Found_Argument__(__Parser__ *__Parser_State__)
{
    if (__Parser_State__->__Current__.__Lexeme__.__Length__ != 0U)
    {
        return __Diagnostic_Set_Argument_Text__(__Parser_State__->__Diagnostic__,
                                                __Diagnostic_Argument_Found__,
                                                __Parser_State__->__Current__.__Lexeme__);
    }

    return __Diagnostic_Set_Argument_Cstr__(
        __Parser_State__->__Diagnostic__, __Diagnostic_Argument_Found__, "<EOF>");
}

/* Records a failure for the parser missing semicolon. */
static int __Parser_Fail_Missing_Semicolon__(__Parser__ *__Parser_State__)
{
    /* Stores the insertion. */
    __Source_Span__ __Insertion__;

    __Insertion__.__Start__ = __Parser_State__->__Previous__.__Span__.__End__;
    __Insertion__.__End__ = __Insertion__.__Start__;

    __Diagnostic_Begin__(__Parser_State__->__Diagnostic__, __E0001_Syntax_Error__, __Insertion__);
    __Diagnostic_Set_Message_Key__(__Parser_State__->__Diagnostic__,
                                   __Diag_Word_Syntax_Missing_Semicolon__);
    __Diagnostic_Set_Primary_Role__(__Parser_State__->__Diagnostic__,
                                    __Diagnostic_Span_Role_Expected_Here__);
    (void)__Diagnostic_Set_Argument_Cstr__(
        __Parser_State__->__Diagnostic__, __Diagnostic_Argument_Expected__, ";");
    (void)__Diagnostic_Add_Related_Span__(__Parser_State__->__Diagnostic__,
                                          __Parser_State__->__Current__.__Span__,
                                          __Diagnostic_Span_Role_Parser_Discovery__);
    (void)__Diagnostic_Add_Help__(__Parser_State__->__Diagnostic__,
                                  __Diag_Word_Help_Insert_Semicolon__);
    (void)__Diagnostic_Add_Fix__(__Parser_State__->__Diagnostic__,
                                 __Insertion__,
                                 ";",
                                 __Diagnostic_Fix_Machine_Applicable__,
                                 __Diag_Word_Fix_Insert_Semicolon__);
    __Parser_State__->__Failed__ = 1;
    return 0;
}

/* Expects the parser. */
int __Parser_Expect__(__Parser__ *__Parser_State__, __Token_Kind__ __Kind__)
{
    /* References the expected. */
    const char *__Expected__;

    if (__Parser_State__ == NULL)
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ == __Kind__)
    {
        return __Parser_Advance__(__Parser_State__);
    }

    if (__Kind__ == __Token_SEMICOLON_OPERATOR__)
    {
        return __Parser_Fail_Missing_Semicolon__(__Parser_State__);
    }

    if (__Kind__ == __Token_FUNCTION_DEF__)
    {
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Function_Definition__);
    }
    if (__Kind__ == __Token_WITH__)
    {
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_With__);
    }

    __Expected__ = __Parser_Expected_Token_Spelling__(__Kind__);
    __Diagnostic_Begin__(__Parser_State__->__Diagnostic__,
                         __E0001_Syntax_Error__,
                         __Parser_State__->__Current__.__Span__);
    __Diagnostic_Set_Message_Key__(__Parser_State__->__Diagnostic__,
                                   __Diag_Word_Syntax_Expected_Found__);
    __Diagnostic_Set_Primary_Role__(__Parser_State__->__Diagnostic__,
                                    __Diagnostic_Span_Role_Expected_Here__);
    (void)__Diagnostic_Set_Argument_Cstr__(
        __Parser_State__->__Diagnostic__, __Diagnostic_Argument_Expected__, __Expected__);
    (void)__Parser_Set_Found_Argument__(__Parser_State__);
    __Parser_State__->__Failed__ = 1;
    return 0;
}
