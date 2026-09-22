#include "frontend/parser/cursor.h"

int __Parser_Token_Is_Contextual_Name__(const __Token__ *__Token_Value__)
{
    if (__Token_Value__ == NULL)
    {
        return 0;
    }
    return __Token_Value__->__Kind__ == __Token_IDENTIFIER__ ||
           __Token_Is_Keyword_Kind__(__Token_Value__->__Kind__);
}

int __Parser_Take_Contextual_Name__(__Parser__ *__Parser_State__, __Text_Slice__ *__Out_Name__)
{
    if (__Parser_State__ == NULL || __Out_Name__ == NULL ||
        !__Parser_Token_Is_Contextual_Name__(&__Parser_State__->__Current__))
    {
        return 0;
    }

    *__Out_Name__ = __Parser_State__->__Current__.__Kind__ == __Token_IDENTIFIER__
                        ? __Parser_State__->__Current__.__Lexeme__
                        : __Parser_State__->__Current__.__Lexeme__;
    return __Parser_Advance__(__Parser_State__);
}
