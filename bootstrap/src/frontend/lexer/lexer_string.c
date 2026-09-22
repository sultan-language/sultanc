#include "frontend/lexer/literal.h"
#include "frontend/lexer/cursor.h"
#include "frontend/lexer/escape.h"
#include "support/text/builder.h"
#include "support/text/utf8.h"

#include <string.h>
int __Lexer_Scan_String__(__Lexer__ *__Lexer_State__,
                          __Token__ *__Out_Token__,
                          __Source_Position__ __Start__,
                          size_t __Start_Offset__,
                          int __Guillemet__)
{
    __Text_Builder__ __Builder__;
    const char *__Closing__ = __Guillemet__ ? "»" : "\"";
    size_t __Closing_Length__ = strlen(__Closing__);
    __Text_Builder_Init__(&__Builder__);

    while (__Lexer_Has__(__Lexer_State__, 1U))
    {
        if (__Lexer_Matches__(__Lexer_State__, __Closing__))
        {
            char *__Decoded__ = NULL;
            __Lexer_Advance_Bytes__(__Lexer_State__, __Closing_Length__);
            __Decoded__ =
                __Arena_Copy_Text__(&__Lexer_State__->__Literal_Arena__,
                                    __Builder__.__Data__ == NULL ? "" : __Builder__.__Data__,
                                    __Builder__.__Length__);
            if (__Decoded__ == NULL)
            {
                __Text_Builder_Destroy__(&__Builder__);
                return __Lexer_Fail__(__Lexer_State__,
                                      __E1100_Internal_Context_Error__,
                                      __Start__,
                                      __Diag_Word_Syntax_Internal_Oom__);
            }
            {
                __Text_Slice__ __Text__;
                int __Success__;
                __Text__.__Data__ = __Decoded__;
                __Text__.__Length__ = __Builder__.__Length__;
                __Success__ = __Lexer_Set_Token_Text__(__Lexer_State__,
                                                       __Out_Token__,
                                                       __Token_LIT_STR__,
                                                       __Start__,
                                                       __Start_Offset__,
                                                       __Text__);
                __Text_Builder_Destroy__(&__Builder__);
                return __Success__;
            }
        }
        if (__Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'\n')
        {
            __Text_Builder_Destroy__(&__Builder__);
            return __Lexer_Fail__(__Lexer_State__,
                                  __E0001_Syntax_Error__,
                                  __Start__,
                                  __Diag_Word_Lex_Unterminated_String__);
        }
        if (__Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'\\')
        {
            __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
            if (!__Lexer_Scan_Escape__(__Lexer_State__, &__Builder__, __Start__))
            {
                __Text_Builder_Destroy__(&__Builder__);
                return 0;
            }
        }
        else
        {
            unsigned char __Byte__ = __Lexer_Peek__(__Lexer_State__, 0U);
            if (__Byte__ >= 0x80U)
            {
                __Utf8_Decode_Result__ __Decoded__ = __Utf8_Decode__(
                    (const unsigned char *)__Lexer_State__->__Source__->__Bytes__ +
                        __Lexer_State__->__Offset__,
                    __Lexer_State__->__Source__->__Length__ - __Lexer_State__->__Offset__);
                if (!__Decoded__.__Valid__)
                {
                    __Text_Builder_Destroy__(&__Builder__);
                    return __Lexer_Fail__(__Lexer_State__,
                                          __E1001_Invalid_UTF8_Byte__,
                                          __Start__,
                                          __Diag_Word_Lex_Invalid_Utf8_String__);
                }
                if (!__Text_Builder_Append__(&__Builder__,
                                             __Lexer_State__->__Source__->__Bytes__ +
                                                 __Lexer_State__->__Offset__,
                                             __Decoded__.__Width__))
                {
                    __Text_Builder_Destroy__(&__Builder__);
                    return __Lexer_Fail__(__Lexer_State__,
                                          __E1100_Internal_Context_Error__,
                                          __Start__,
                                          __Diag_Word_Syntax_Internal_Oom__);
                }
                __Lexer_Advance_Bytes__(__Lexer_State__, __Decoded__.__Width__);
            }
            else
            {
                if (!__Text_Builder_Append_Byte__(&__Builder__, __Byte__))
                {
                    __Text_Builder_Destroy__(&__Builder__);
                    return __Lexer_Fail__(__Lexer_State__,
                                          __E1100_Internal_Context_Error__,
                                          __Start__,
                                          __Diag_Word_Syntax_Internal_Oom__);
                }
                __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
            }
        }
    }
    __Text_Builder_Destroy__(&__Builder__);
    return __Lexer_Fail__(
        __Lexer_State__, __E0001_Syntax_Error__, __Start__, __Diag_Word_Lex_Unterminated_String__);
}

int __Lexer_Scan_Character__(__Lexer__ *__Lexer_State__,
                             __Token__ *__Out_Token__,
                             __Source_Position__ __Start__,
                             size_t __Start_Offset__)
{
    uint32_t __Codepoint__ = 0U;
    if (!__Lexer_Has__(__Lexer_State__, 1U))
    {
        return __Lexer_Fail__(__Lexer_State__,
                              __E0001_Syntax_Error__,
                              __Start__,
                              __Diag_Word_Lex_Unterminated_Character__);
    }
    if (__Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'\\')
    {
        unsigned char __Escape__ = 0U;
        __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
        if (!__Lexer_Has__(__Lexer_State__, 1U))
        {
            return __Lexer_Fail__(__Lexer_State__,
                                  __E0001_Syntax_Error__,
                                  __Start__,
                                  __Diag_Word_Lex_Unterminated_Character_Escape__);
        }
        __Escape__ = __Lexer_Peek__(__Lexer_State__, 0U);
        __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
        switch (__Escape__)
        {
            case 'n':
                __Codepoint__ = (uint32_t)'\n';
                break;
            case 'r':
                __Codepoint__ = (uint32_t)'\r';
                break;
            case 't':
                __Codepoint__ = (uint32_t)'\t';
                break;
            case '\\':
                __Codepoint__ = (uint32_t)'\\';
                break;
            case '\'':
                __Codepoint__ = (uint32_t)'\'';
                break;
            case 'x':
                if (!__Lexer_Parse_Hex_Escape__(__Lexer_State__, 2U, &__Codepoint__))
                {
                    return __Lexer_Fail__(__Lexer_State__,
                                          __E0001_Syntax_Error__,
                                          __Start__,
                                          __Diag_Word_Lex_Bad_Character_Escape__);
                }
                break;
            case 'u':
                if (!__Lexer_Parse_Hex_Escape__(__Lexer_State__, 4U, &__Codepoint__))
                {
                    return __Lexer_Fail__(__Lexer_State__,
                                          __E0001_Syntax_Error__,
                                          __Start__,
                                          __Diag_Word_Lex_Bad_Character_Escape__);
                }
                break;
            case 'U':
                if (!__Lexer_Parse_Hex_Escape__(__Lexer_State__, 8U, &__Codepoint__))
                {
                    return __Lexer_Fail__(__Lexer_State__,
                                          __E0001_Syntax_Error__,
                                          __Start__,
                                          __Diag_Word_Lex_Bad_Character_Escape__);
                }
                break;
            default:
                return __Lexer_Fail__(__Lexer_State__,
                                      __E0001_Syntax_Error__,
                                      __Start__,
                                      __Diag_Word_Lex_Bad_Character_Escape__);
        }
    }
    else
    {
        __Utf8_Decode_Result__ __Decoded__ =
            __Utf8_Decode__((const unsigned char *)__Lexer_State__->__Source__->__Bytes__ +
                                __Lexer_State__->__Offset__,
                            __Lexer_State__->__Source__->__Length__ - __Lexer_State__->__Offset__);
        if (!__Decoded__.__Valid__)
        {
            return __Lexer_Fail__(__Lexer_State__,
                                  __E1001_Invalid_UTF8_Byte__,
                                  __Start__,
                                  __Diag_Word_Lex_Invalid_Utf8_Character__);
        }
        __Codepoint__ = __Decoded__.__Codepoint__;
        __Lexer_Advance_Bytes__(__Lexer_State__, __Decoded__.__Width__);
    }

    if (!__Utf8_Is_Scalar__(__Codepoint__))
    {
        return __Lexer_Fail__(__Lexer_State__,
                              __E0001_Syntax_Error__,
                              __Start__,
                              __Diag_Word_Lex_Bad_Character_Escape__);
    }

    if (!__Lexer_Has__(__Lexer_State__, 1U) ||
        __Lexer_Peek__(__Lexer_State__, 0U) != (unsigned char)'\'')
    {
        return __Lexer_Fail__(__Lexer_State__,
                              __E0001_Syntax_Error__,
                              __Start__,
                              __Diag_Word_Lex_Unterminated_Character__);
    }
    __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
    return __Lexer_Set_Token_Character__(__Lexer_State__,
                                         __Out_Token__,
                                         __Token_LIT_CHAR__,
                                         __Start__,
                                         __Start_Offset__,
                                         __Codepoint__);
}
