/* Scans the next source token. */

#include "frontend/lexer/scanning.h"
#include "frontend/identifier_identity.h"
#include "frontend/lexer/lexer.h"
#include "frontend/lexer/cursor.h"
#include "frontend/lexer/character.h"
#include "frontend/lexer/keyword.h"
#include "frontend/lexer/literal.h"
#include "frontend/lexer/operator.h"
#include "support/text/utf8.h"

#include <string.h>

/* Decodes the lexer at. */
static __Utf8_Decode_Result__ __Lexer_Decode_At__(const __Lexer__ *__Lexer_State__,
                                                  size_t __Offset__)
{
    if (__Offset__ >= __Lexer_State__->__Source__->__Length__)
    {
        /* Stores the empty. */
        __Utf8_Decode_Result__ __Empty__ = {0U, 0U, 0};
        return __Empty__;
    }
    return __Utf8_Decode__((const unsigned char *)__Lexer_State__->__Source__->__Bytes__ +
                               __Offset__,
                           __Lexer_State__->__Source__->__Length__ - __Offset__);
}

/* Returns the lexer. */
int __Lexer_Next__(__Lexer__ *__Lexer_State__, __Token__ *__Out_Token__)
{
    /* Stores the start. */
    __Source_Position__ __Start__;
    /* Stores the start offset. */
    size_t __Start_Offset__ = 0U;

    if (__Lexer_State__ == NULL || __Out_Token__ == NULL || __Lexer_State__->__Source__ == NULL ||
        __Lexer_State__->__Failed__)
    {
        return 0;
    }

    for (;;)
    {
        while (__Lexer_Has__(__Lexer_State__, 1U))
        {
            /* Stores the byte. */
            unsigned char __Byte__ = __Lexer_Peek__(__Lexer_State__, 0U);
            if (__Byte__ == (unsigned char)' ' || __Byte__ == (unsigned char)'\t' ||
                __Byte__ == (unsigned char)'\r' || __Byte__ == (unsigned char)'\n')
            {
                __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
            }
            else
            {
                break;
            }
        }
        if (__Lexer_Matches__(__Lexer_State__, "//"))
        {
            while (__Lexer_Has__(__Lexer_State__, 1U) &&
                   __Lexer_Peek__(__Lexer_State__, 0U) != (unsigned char)'\n')
            {
                __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
            }
            continue;
        }
        break;
    }

    __Start__ = __Lexer_Position__(__Lexer_State__);
    __Start_Offset__ = __Lexer_State__->__Offset__;
    if (!__Lexer_Has__(__Lexer_State__, 1U))
    {
        return __Lexer_Set_Token__(
            __Lexer_State__, __Out_Token__, __Token_EOF__, __Start__, __Start_Offset__);
    }

    if (__Lexer_Matches__(__Lexer_State__, "«"))
    {
        __Lexer_Advance_Bytes__(__Lexer_State__, strlen("«"));
        return __Lexer_Scan_String__(
            __Lexer_State__, __Out_Token__, __Start__, __Start_Offset__, 1);
    }
    if (__Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'"')
    {
        __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
        return __Lexer_Scan_String__(
            __Lexer_State__, __Out_Token__, __Start__, __Start_Offset__, 0);
    }
    if (__Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'\'')
    {
        __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
        return __Lexer_Scan_Character__(
            __Lexer_State__, __Out_Token__, __Start__, __Start_Offset__);
    }

    if (__Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'_')
    {
        /* Tracks whether the ID rest is present. */
        int __Has_Id_Rest__ = 0;
        if (__Lexer_Has__(__Lexer_State__, 2U))
        {
            /* Stores the next byte. */
            unsigned char __Next_Byte__ = __Lexer_Peek__(__Lexer_State__, 1U);
            if (__Lexer_Is_Decimal_Digit__(__Next_Byte__))
            {
                /* Stores the value. */
                uint64_t __Value__ = 0U;
                __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
                while (__Lexer_Has__(__Lexer_State__, 1U) &&
                       __Lexer_Is_Decimal_Digit__(__Lexer_Peek__(__Lexer_State__, 0U)))
                {
                    /* Stores the digit. */
                    uint64_t __Digit__ =
                        (uint64_t)(__Lexer_Peek__(__Lexer_State__, 0U) - (unsigned char)'0');
                    if (__Value__ > (((uint64_t)INT64_MAX - __Digit__) / 10U))
                    {
                        return __Lexer_Fail__(__Lexer_State__,
                                              __E0001_Syntax_Error__,
                                              __Start__,
                                              __Diag_Word_Lex_Invalid_Integer__);
                    }
                    __Value__ = (__Value__ * 10U) + __Digit__;
                    __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
                }
                return __Lexer_Set_Token_Index__(__Lexer_State__,
                                                 __Out_Token__,
                                                 __Token_UNDERSCORE_WITH_INDEX__,
                                                 __Start__,
                                                 __Start_Offset__,
                                                 (int64_t)__Value__);
            }
            {
                /* Stores the next. */
                __Utf8_Decode_Result__ __Next__ =
                    __Lexer_Decode_At__(__Lexer_State__, __Start_Offset__ + 1U);
                __Has_Id_Rest__ =
                    __Next__.__Valid__ && __Lexer_Is_Id_Rest__(__Next__.__Codepoint__);
            }
        }
        if (!__Has_Id_Rest__)
        {
            __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
            return __Lexer_Set_Token__(__Lexer_State__,
                                       __Out_Token__,
                                       __Token_UNDERSCORE_OPERATOR__,
                                       __Start__,
                                       __Start_Offset__);
        }
    }

    if (__Lexer_Is_Decimal_Digit__(__Lexer_Peek__(__Lexer_State__, 0U)))
    {
        return __Lexer_Scan_Number__(__Lexer_State__, __Out_Token__, __Start__, __Start_Offset__);
    }

    {
        /* Stores the first. */
        __Utf8_Decode_Result__ __First__ = __Lexer_Decode_At__(__Lexer_State__, __Start_Offset__);
        if (__First__.__Valid__ && __Lexer_Is_Id_Start__(__First__.__Codepoint__))
        {
            /* Stores the keyword. */
            __Keyword_Result__ __Keyword__;
            /* Stores the text. */
            __Text_Slice__ __Text__;
            while (__Lexer_Has__(__Lexer_State__, 1U))
            {
                /* Stores the current. */
                __Utf8_Decode_Result__ __Current__ =
                    __Lexer_Decode_At__(__Lexer_State__, __Lexer_State__->__Offset__);
                if (!__Current__.__Valid__)
                {
                    return __Lexer_Fail__(__Lexer_State__,
                                          __E1001_Invalid_UTF8_Byte__,
                                          __Start__,
                                          __Diag_Word_Lex_Invalid_Initial_Utf8__);
                }
                if (!__Lexer_Is_Id_Rest__(__Current__.__Codepoint__))
                {
                    break;
                }
                __Lexer_Advance_Bytes__(__Lexer_State__, __Current__.__Width__);
            }
            __Text__ = __Source_Slice__(
                __Lexer_State__->__Source__, __Start_Offset__, __Lexer_State__->__Offset__);
            if (!__Identifier_Identity_Has_Content__(__Text__))
            {
                return __Lexer_Fail__(__Lexer_State__,
                                      __E0001_Syntax_Error__,
                                      __Start__,
                                      __Diag_Word_Lex_Unexpected_Character__);
            }
            __Keyword__ = __Keyword_Lookup__(__Text__);
            if (__Keyword__.__Found__)
            {
                if (__Keyword__.__Kind__ == __Token_LIT_BOOL__)
                {
                    return __Lexer_Set_Token_Boolean__(__Lexer_State__,
                                                       __Out_Token__,
                                                       __Keyword__.__Kind__,
                                                       __Start__,
                                                       __Start_Offset__,
                                                       __Keyword__.__Boolean_Value__);
                }
                return __Lexer_Set_Token__(__Lexer_State__,
                                           __Out_Token__,
                                           __Keyword__.__Kind__,
                                           __Start__,
                                           __Start_Offset__);
            }
            return __Lexer_Set_Token__(
                __Lexer_State__, __Out_Token__, __Token_IDENTIFIER__, __Start__, __Start_Offset__);
        }
    }

    if (__Lexer_Scan_Operator__(__Lexer_State__, __Out_Token__, __Start__, __Start_Offset__))
    {
        return 1;
    }

    if (__Lexer_Peek__(__Lexer_State__, 0U) >= 0x80U)
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
                                  __Diag_Word_Lex_Invalid_Initial_Utf8__);
        }
    }
    __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
    return __Lexer_Fail__(
        __Lexer_State__, __E0001_Syntax_Error__, __Start__, __Diag_Word_Lex_Unexpected_Character__);
}
