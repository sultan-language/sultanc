/* Scans string and character literals. */

#include "frontend/lexer/literal.h"
#include "frontend/lexer/cursor.h"
#include "frontend/lexer/escape.h"
#include "support/text/builder.h"
#include "support/text/utf8.h"

#include <string.h>
/* Finalizes a scanned string token. */
static int __Lexer_Finish_String__(__Lexer__ *__Lexer_State__,
                                   __Token__ *__Out_Token__,
                                   __Source_Position__ __Start__,
                                   size_t __Start_Offset__,
                                   __Text_Builder__ *__Builder__)
{
    /* References the decoded text. */
    char *__Decoded__ = __Arena_Copy_Text__(&__Lexer_State__->__Literal_Arena__,
                                            __Builder__->__Data__ == NULL ? "" : __Builder__->__Data__,
                                            __Builder__->__Length__);
    if (__Decoded__ == NULL)
    {
        return __Lexer_Fail__(__Lexer_State__,
                              __E1100_Internal_Context_Error__,
                              __Start__,
                              __Diag_Word_Syntax_Internal_Oom__);
    }
    {
        /* Stores the token text. */
        __Text_Slice__ __Text__;
        __Text__.__Data__ = __Decoded__;
        __Text__.__Length__ = __Builder__->__Length__;
        return __Lexer_Set_Token_Text__(__Lexer_State__,
                                        __Out_Token__,
                                        __Token_LIT_STR__,
                                        __Start__,
                                        __Start_Offset__,
                                        __Text__);
    }
}

/* Appends one source character without interpreting escapes. */
static int __Lexer_Append_Raw_Character__(__Lexer__ *__Lexer_State__,
                                          __Text_Builder__ *__Builder__,
                                          __Source_Position__ __Start__)
{
    /* Stores the current byte. */
    unsigned char __Byte__ = __Lexer_Peek__(__Lexer_State__, 0U);
    if (__Byte__ >= 0x80U)
    {
        /* Stores the decoded character. */
        __Utf8_Decode_Result__ __Decoded__ = __Utf8_Decode__(
            (const unsigned char *)__Lexer_State__->__Source__->__Bytes__ +
                __Lexer_State__->__Offset__,
            __Lexer_State__->__Source__->__Length__ - __Lexer_State__->__Offset__);
        if (!__Decoded__.__Valid__)
        {
            return __Lexer_Fail__(__Lexer_State__,
                                  __E1001_Invalid_UTF8_Byte__,
                                  __Start__,
                                  __Diag_Word_Lex_Invalid_Utf8_String__);
        }
        if (!__Text_Builder_Append__(__Builder__,
                                     __Lexer_State__->__Source__->__Bytes__ +
                                         __Lexer_State__->__Offset__,
                                     __Decoded__.__Width__))
        {
            return __Lexer_Fail__(__Lexer_State__,
                                  __E1100_Internal_Context_Error__,
                                  __Start__,
                                  __Diag_Word_Syntax_Internal_Oom__);
        }
        __Lexer_Advance_Bytes__(__Lexer_State__, __Decoded__.__Width__);
        return 1;
    }
    if (!__Text_Builder_Append_Byte__(__Builder__, __Byte__))
    {
        return __Lexer_Fail__(__Lexer_State__,
                              __E1100_Internal_Context_Error__,
                              __Start__,
                              __Diag_Word_Syntax_Internal_Oom__);
    }
    __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
    return 1;
}

/* Scans an escaped or balanced raw string. */
int __Lexer_Scan_String__(__Lexer__ *__Lexer_State__,
                          __Token__ *__Out_Token__,
                          __Source_Position__ __Start__,
                          size_t __Start_Offset__,
                          int __Guillemet__)
{
    /* Stores the decoded string bytes. */
    __Text_Builder__ __Builder__;
    __Text_Builder_Init__(&__Builder__);

    if (__Guillemet__)
    {
        /* Tracks balanced guillemet nesting. */
        size_t __Depth__ = 1U;
        while (__Lexer_Has__(__Lexer_State__, 1U))
        {
            if (__Lexer_Matches__(__Lexer_State__, "«"))
            {
                if (__Depth__ >= 256U)
                {
                    __Text_Builder_Destroy__(&__Builder__);
                    return __Lexer_Fail__(__Lexer_State__,
                                          __E0001_Syntax_Error__,
                                          __Start__,
                                          __Diag_Word_Lex_Unterminated_String__);
                }
                if (!__Text_Builder_Append__(&__Builder__,
                                             __Lexer_State__->__Source__->__Bytes__ +
                                                 __Lexer_State__->__Offset__,
                                             strlen("«")))
                {
                    __Text_Builder_Destroy__(&__Builder__);
                    return __Lexer_Fail__(__Lexer_State__,
                                          __E1100_Internal_Context_Error__,
                                          __Start__,
                                          __Diag_Word_Syntax_Internal_Oom__);
                }
                __Lexer_Advance_Bytes__(__Lexer_State__, strlen("«"));
                ++__Depth__;
                continue;
            }
            if (__Lexer_Matches__(__Lexer_State__, "»"))
            {
                --__Depth__;
                if (__Depth__ == 0U)
                {
                    /* Stores whether token creation succeeded. */
                    int __Success__;
                    __Lexer_Advance_Bytes__(__Lexer_State__, strlen("»"));
                    __Success__ = __Lexer_Finish_String__(
                        __Lexer_State__, __Out_Token__, __Start__, __Start_Offset__, &__Builder__);
                    __Text_Builder_Destroy__(&__Builder__);
                    return __Success__;
                }
                if (!__Text_Builder_Append__(&__Builder__,
                                             __Lexer_State__->__Source__->__Bytes__ +
                                                 __Lexer_State__->__Offset__,
                                             strlen("»")))
                {
                    __Text_Builder_Destroy__(&__Builder__);
                    return __Lexer_Fail__(__Lexer_State__,
                                          __E1100_Internal_Context_Error__,
                                          __Start__,
                                          __Diag_Word_Syntax_Internal_Oom__);
                }
                __Lexer_Advance_Bytes__(__Lexer_State__, strlen("»"));
                continue;
            }
            if (!__Lexer_Append_Raw_Character__(__Lexer_State__, &__Builder__, __Start__))
            {
                __Text_Builder_Destroy__(&__Builder__);
                return 0;
            }
        }
        __Text_Builder_Destroy__(&__Builder__);
        return __Lexer_Fail__(
            __Lexer_State__, __E0001_Syntax_Error__, __Start__, __Diag_Word_Lex_Unterminated_String__);
    }

    while (__Lexer_Has__(__Lexer_State__, 1U))
    {
        if (__Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'"')
        {
            /* Stores whether token creation succeeded. */
            int __Success__;
            __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
            __Success__ = __Lexer_Finish_String__(
                __Lexer_State__, __Out_Token__, __Start__, __Start_Offset__, &__Builder__);
            __Text_Builder_Destroy__(&__Builder__);
            return __Success__;
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
        else if (!__Lexer_Append_Raw_Character__(__Lexer_State__, &__Builder__, __Start__))
        {
            __Text_Builder_Destroy__(&__Builder__);
            return 0;
        }
    }
    __Text_Builder_Destroy__(&__Builder__);
    return __Lexer_Fail__(
        __Lexer_State__, __E0001_Syntax_Error__, __Start__, __Diag_Word_Lex_Unterminated_String__);
}

/* Scans the lexer character. */
int __Lexer_Scan_Character__(__Lexer__ *__Lexer_State__,
                             __Token__ *__Out_Token__,
                             __Source_Position__ __Start__,
                             size_t __Start_Offset__)
{
    /* Stores the codepoint. */
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
        /* Stores the escape. */
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
        /* Stores the decoded. */
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
