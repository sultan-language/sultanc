#include "frontend/lexer/literal.h"
#include "frontend/lexer/cursor.h"
#include "frontend/lexer/character.h"

#include <errno.h>
#include <stdlib.h>

static int __Lexer_Digit_For_Base__(unsigned char __Byte__, int __Base__)
{
    if (__Base__ == 2)
    {
        return __Byte__ == (unsigned char)'0' || __Byte__ == (unsigned char)'1';
    }
    if (__Base__ == 16)
    {
        return __Lexer_Is_Hex_Digit__(__Byte__);
    }
    return __Lexer_Is_Decimal_Digit__(__Byte__);
}

/* Numeric separators are permitted only between two digits of the active radix. */
static int __Lexer_Number_Separators_Valid__(__Text_Slice__ __Spelling__)
{
    size_t __Index__ = 0U;
    int __Base__ = 10;
    if (__Spelling__.__Length__ >= 2U && __Spelling__.__Data__[0] == '0' &&
        __Spelling__.__Data__[1] == 'b')
    {
        __Base__ = 2;
    }
    else if (__Spelling__.__Length__ >= 2U && __Spelling__.__Data__[0] == '0' &&
             __Spelling__.__Data__[1] == 'x')
    {
        __Base__ = 16;
    }
    for (__Index__ = 0U; __Index__ < __Spelling__.__Length__; ++__Index__)
    {
        if (__Spelling__.__Data__[__Index__] == '_')
        {
            if (__Index__ == 0U || __Index__ + 1U >= __Spelling__.__Length__ ||
                !__Lexer_Digit_For_Base__((unsigned char)__Spelling__.__Data__[__Index__ - 1U],
                                          __Base__) ||
                !__Lexer_Digit_For_Base__((unsigned char)__Spelling__.__Data__[__Index__ + 1U],
                                          __Base__))
            {
                return 0;
            }
        }
    }
    return 1;
}
static int __Lexer_Parse_Integer_Value__(__Text_Slice__ __Spelling__, int64_t *__Out_Value__)
{
    char __Buffer__[256];
    char *__End__ = NULL;
    size_t __Input__ = 0U;
    size_t __Output__ = 0U;
    int __Base__ = 10;
    if (__Spelling__.__Length__ >= sizeof(__Buffer__))
    {
        return 0;
    }
    for (__Input__ = 0U; __Input__ < __Spelling__.__Length__; ++__Input__)
    {
        char __Character__ = __Spelling__.__Data__[__Input__];
        if (__Character__ != '_')
        {
            __Buffer__[__Output__++] = __Character__;
        }
    }
    __Buffer__[__Output__] = '\0';
    if (__Output__ >= 2U && __Buffer__[0] == '0' && __Buffer__[1] == 'b')
    {
        size_t __Index__ = 2U;
        uint64_t __Value__ = 0U;
        if (__Output__ == 2U)
        {
            return 0;
        }
        for (; __Index__ < __Output__; ++__Index__)
        {
            if (__Buffer__[__Index__] != '0' && __Buffer__[__Index__] != '1')
            {
                return 0;
            }
            if (__Value__ > ((uint64_t)INT64_MAX >> 1U))
            {
                return 0;
            }
            __Value__ = (__Value__ << 1U) | (uint64_t)(__Buffer__[__Index__] - '0');
        }
        *__Out_Value__ = (int64_t)__Value__;
        return 1;
    }
    if (__Output__ >= 2U && __Buffer__[0] == '0' && __Buffer__[1] == 'x')
    {
        __Base__ = 16;
    }
    errno = 0;
    *__Out_Value__ = strtoll(__Buffer__, &__End__, __Base__);
    return errno != ERANGE && __End__ != __Buffer__ && *__End__ == '\0';
}

static void __Lexer_Scan_Decimal_Digits__(__Lexer__ *__Lexer_State__)
{
    while (__Lexer_Has__(__Lexer_State__, 1U))
    {
        unsigned char __Byte__ = __Lexer_Peek__(__Lexer_State__, 0U);
        if (!__Lexer_Is_Decimal_Digit__(__Byte__) && __Byte__ != (unsigned char)'_')
        {
            break;
        }
        __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
    }
}

int __Lexer_Scan_Number__(__Lexer__ *__Lexer_State__,
                          __Token__ *__Out_Token__,
                          __Source_Position__ __Start__,
                          size_t __Start_Offset__)
{
    int __Decimal_Float__ = 0;
    if (__Lexer_Matches__(__Lexer_State__, "0b"))
    {
        __Lexer_Advance_Bytes__(__Lexer_State__, 2U);
        while (__Lexer_Has__(__Lexer_State__, 1U))
        {
            unsigned char __Byte__ = __Lexer_Peek__(__Lexer_State__, 0U);
            if (__Byte__ != (unsigned char)'0' && __Byte__ != (unsigned char)'1' &&
                __Byte__ != (unsigned char)'_')
            {
                break;
            }
            __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
        }
    }
    else if (__Lexer_Matches__(__Lexer_State__, "0x"))
    {
        __Lexer_Advance_Bytes__(__Lexer_State__, 2U);
        while (__Lexer_Has__(__Lexer_State__, 1U))
        {
            unsigned char __Byte__ = __Lexer_Peek__(__Lexer_State__, 0U);
            if (!__Lexer_Is_Hex_Digit__(__Byte__) && __Byte__ != (unsigned char)'_')
            {
                break;
            }
            __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
        }
    }
    else
    {
        __Lexer_Scan_Decimal_Digits__(__Lexer_State__);
        if (__Lexer_Has__(__Lexer_State__, 2U) &&
            __Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'.' &&
            __Lexer_Is_Decimal_Digit__(__Lexer_Peek__(__Lexer_State__, 1U)))
        {
            __Decimal_Float__ = 1;
            __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
            __Lexer_Scan_Decimal_Digits__(__Lexer_State__);
        }
        if (__Lexer_Has__(__Lexer_State__, 1U) &&
            (__Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'e' ||
             __Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'E'))
        {
            size_t __Saved_Offset__ = __Lexer_State__->__Offset__;
            uint32_t __Saved_Line__ = __Lexer_State__->__Line__;
            uint32_t __Saved_Column__ = __Lexer_State__->__Column__;
            __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
            if (__Lexer_Has__(__Lexer_State__, 1U) &&
                (__Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'+' ||
                 __Lexer_Peek__(__Lexer_State__, 0U) == (unsigned char)'-'))
            {
                __Lexer_Advance_Bytes__(__Lexer_State__, 1U);
            }
            if (__Lexer_Has__(__Lexer_State__, 1U) &&
                __Lexer_Is_Decimal_Digit__(__Lexer_Peek__(__Lexer_State__, 0U)))
            {
                __Decimal_Float__ = 1;
                __Lexer_Scan_Decimal_Digits__(__Lexer_State__);
            }
            else
            {
                __Lexer_State__->__Offset__ = __Saved_Offset__;
                __Lexer_State__->__Line__ = __Saved_Line__;
                __Lexer_State__->__Column__ = __Saved_Column__;
            }
        }
    }

    {
        __Text_Slice__ __Spelling__ = __Source_Slice__(
            __Lexer_State__->__Source__, __Start_Offset__, __Lexer_State__->__Offset__);
        if (!__Lexer_Number_Separators_Valid__(__Spelling__))
        {
            return __Lexer_Fail__(__Lexer_State__,
                                  __E0001_Syntax_Error__,
                                  __Start__,
                                  __Diag_Word_Lex_Invalid_Integer__);
        }
    }

    if (__Decimal_Float__)
    {
        return __Lexer_Set_Token__(
            __Lexer_State__, __Out_Token__, __Token_LIT_FLO__, __Start__, __Start_Offset__);
    }

    {
        __Integer_Literal__ __Integer__;
        __Integer__.__Spelling__ = __Source_Slice__(
            __Lexer_State__->__Source__, __Start_Offset__, __Lexer_State__->__Offset__);
        if (!__Lexer_Parse_Integer_Value__(__Integer__.__Spelling__, &__Integer__.__Value__))
        {
            return __Lexer_Fail__(__Lexer_State__,
                                  __E0001_Syntax_Error__,
                                  __Start__,
                                  __Diag_Word_Lex_Invalid_Integer__);
        }
        return __Lexer_Set_Token_Integer__(__Lexer_State__,
                                           __Out_Token__,
                                           __Token_LIT_INT__,
                                           __Start__,
                                           __Start_Offset__,
                                           __Integer__);
    }
}
