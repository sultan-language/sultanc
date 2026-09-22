#include "frontend/lexer/escape.h"
#include "frontend/lexer/character.h"
#include "support/text/utf8.h"

int __Lexer_Parse_Hex_Escape__(__Lexer__ *__Lexer_State__,
                               size_t __Digits__,
                               uint32_t *__Out_Codepoint__)
{
    size_t __Index__ = 0U;
    uint32_t __Value__ = 0U;
    if (!__Lexer_Has__(__Lexer_State__, __Digits__))
    {
        return 0;
    }
    for (__Index__ = 0U; __Index__ < __Digits__; ++__Index__)
    {
        int __Digit__ = __Lexer_Hex_Value__(__Lexer_Peek__(__Lexer_State__, __Index__));
        if (__Digit__ < 0)
        {
            return 0;
        }
        __Value__ = (__Value__ << 4U) | (uint32_t)__Digit__;
    }
    __Lexer_Advance_Bytes__(__Lexer_State__, __Digits__);
    *__Out_Codepoint__ = __Value__;
    return 1;
}

static int __Lexer_Append_Codepoint__(__Text_Builder__ *__Builder__, uint32_t __Codepoint__)
{
    unsigned char __Encoded__[4];
    size_t __Width__ = __Utf8_Encode__(__Codepoint__, __Encoded__);
    return __Width__ != 0U && __Text_Builder_Append__(__Builder__, __Encoded__, __Width__);
}

int __Lexer_Scan_Escape__(__Lexer__ *__Lexer_State__,
                          __Text_Builder__ *__Builder__,
                          __Source_Position__ __Start__)
{
    unsigned char __Escape__ = 0U;
    uint32_t __Codepoint__ = 0U;
    if (!__Lexer_Has__(__Lexer_State__, 1U))
    {
        return __Lexer_Fail__(__Lexer_State__,
                              __E0001_Syntax_Error__,
                              __Start__,
                              __Diag_Word_Lex_Unterminated_Escape__);
    }
    __Escape__ = __Lexer_Peek__(__Lexer_State__, 0U);
    __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
    switch (__Escape__)
    {
        case 'n':
            return __Text_Builder_Append_Byte__(__Builder__, (unsigned char)'\n');
        case 'r':
            return __Text_Builder_Append_Byte__(__Builder__, (unsigned char)'\r');
        case 't':
            return __Text_Builder_Append_Byte__(__Builder__, (unsigned char)'\t');
        case '\\':
            return __Text_Builder_Append_Byte__(__Builder__, (unsigned char)'\\');
        case '"':
            return __Text_Builder_Append_Byte__(__Builder__, (unsigned char)'"');
        case 'x':
            if (!__Lexer_Parse_Hex_Escape__(__Lexer_State__, 2U, &__Codepoint__))
            {
                return __Lexer_Fail__(__Lexer_State__,
                                      __E0001_Syntax_Error__,
                                      __Start__,
                                      __Diag_Word_Lex_Bad_Hex_Escape__);
            }
            return __Lexer_Append_Codepoint__(__Builder__, __Codepoint__);
        case 'u':
            if (!__Lexer_Parse_Hex_Escape__(__Lexer_State__, 4U, &__Codepoint__) ||
                !__Utf8_Is_Scalar__(__Codepoint__))
            {
                return __Lexer_Fail__(__Lexer_State__,
                                      __E0001_Syntax_Error__,
                                      __Start__,
                                      __Diag_Word_Lex_Bad_Unicode_Escape__);
            }
            return __Lexer_Append_Codepoint__(__Builder__, __Codepoint__);
        case 'U':
            if (!__Lexer_Parse_Hex_Escape__(__Lexer_State__, 8U, &__Codepoint__) ||
                !__Utf8_Is_Scalar__(__Codepoint__))
            {
                return __Lexer_Fail__(__Lexer_State__,
                                      __E0001_Syntax_Error__,
                                      __Start__,
                                      __Diag_Word_Lex_Bad_Unicode_Escape__);
            }
            return __Lexer_Append_Codepoint__(__Builder__, __Codepoint__);
        default:
            return __Lexer_Fail__(
                __Lexer_State__, __E0001_Syntax_Error__, __Start__, __Diag_Word_Lex_Bad_Escape__);
    }
}
