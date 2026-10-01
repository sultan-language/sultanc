/* Declares keyword lookup operations. */

#ifndef SULTANC__FRONTEND_LEXER_KEYWORD_H__
#define SULTANC__FRONTEND_LEXER_KEYWORD_H__

#include "core/source.h"
#include "token.h"

/* Defines the keyword result structure. */
typedef struct
{
    /* Tracks the found state. */
    int __Found__;
    /* Stores the kind. */
    __Token_Kind__ __Kind__;
    /* Stores the boolean value. */
    int __Boolean_Value__;
} __Keyword_Result__;

/* Finds the keyword. */
__Keyword_Result__ __Keyword_Lookup__(__Text_Slice__ __Text__);

#endif
