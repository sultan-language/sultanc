#include "frontend/lexer/cursor.h"

int __Lexer_Fail__(__Lexer__ *__Lexer_State__,
                   __Error_Id__ __Id__,
                   __Source_Position__ __Start__,
                   __Diagnostic_Wording_Key__ __Message_Key__)
{
    __Source_Span__ __Span__;

    __Span__.__Start__ = __Start__;
    __Span__.__End__ = __Lexer_Position__(__Lexer_State__);
    __Diagnostic_Begin__(&__Lexer_State__->__Diagnostic__, __Id__, __Span__);
    __Diagnostic_Set_Message_Key__(&__Lexer_State__->__Diagnostic__, __Message_Key__);
    __Lexer_State__->__Failed__ = 1;
    return 0;
}
