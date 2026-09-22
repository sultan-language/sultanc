#ifndef SULTANC__FRONTEND_LEXER_KEYWORD_H__
#define SULTANC__FRONTEND_LEXER_KEYWORD_H__

#include "core/source.h"
#include "token.h"

typedef struct
{
    int __Found__;
    __Token_Kind__ __Kind__;
    int __Boolean_Value__;
} __Keyword_Result__;

__Keyword_Result__ __Keyword_Lookup__(__Text_Slice__ __Text__);

#endif
