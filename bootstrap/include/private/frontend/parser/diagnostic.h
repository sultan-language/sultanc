/* Declares parser diagnostic helpers. */

#ifndef SULTANC__FRONTEND_PARSER_DIAGNOSTIC_H__
#define SULTANC__FRONTEND_PARSER_DIAGNOSTIC_H__

#include "frontend/lexer/token.h"
#include "internal.h"

/* Expects the parser. */
int __Parser_Expect__(__Parser__ *__Parser_State__, __Token_Kind__ __Kind__);

/* Records a failure for the parser. */
int __Parser_Fail__(__Parser__ *__Parser_State__, __Diagnostic_Wording_Key__ __Message_Key__);

/* Records a failure for the parser internal. */
int __Parser_Fail_Internal__(__Parser__ *__Parser_State__,
                             __Diagnostic_Wording_Key__ __Message_Key__);

#endif
