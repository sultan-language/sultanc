#include "support/text/utf8.h"

__Utf8_Decode_Result__ __Utf8_Decode__(const unsigned char *__Bytes__, size_t __Remaining__)
{
    __Utf8_Decode_Result__ __Result__ = {0U, 0U, 0};
    uint32_t __Codepoint__ = 0U;
    size_t __Width__ = 0U;
    size_t __Index__ = 0U;
    unsigned char __First__ = 0U;

    if (__Bytes__ == NULL || __Remaining__ == 0U)
    {
        return __Result__;
    }
    __First__ = __Bytes__[0];
    if (__First__ <= 0x7FU)
    {
        __Result__.__Codepoint__ = __First__;
        __Result__.__Width__ = 1U;
        __Result__.__Valid__ = 1;
        return __Result__;
    }
    if (__First__ >= 0xC2U && __First__ <= 0xDFU)
    {
        __Width__ = 2U;
        __Codepoint__ = __First__ & 0x1FU;
    }
    else if (__First__ >= 0xE0U && __First__ <= 0xEFU)
    {
        __Width__ = 3U;
        __Codepoint__ = __First__ & 0x0FU;
    }
    else if (__First__ >= 0xF0U && __First__ <= 0xF4U)
    {
        __Width__ = 4U;
        __Codepoint__ = __First__ & 0x07U;
    }
    else
    {
        return __Result__;
    }
    if (__Remaining__ < __Width__)
    {
        return __Result__;
    }
    for (__Index__ = 1U; __Index__ < __Width__; ++__Index__)
    {
        unsigned char __Byte__ = __Bytes__[__Index__];
        if (!__Utf8_Is_Continuation_Byte__(__Byte__))
        {
            return __Result__;
        }
        __Codepoint__ = (__Codepoint__ << 6U) | (uint32_t)(__Byte__ & 0x3FU);
    }
    if ((__Width__ == 2U && __Codepoint__ < 0x80U) || (__Width__ == 3U && __Codepoint__ < 0x800U) ||
        (__Width__ == 4U && __Codepoint__ < 0x10000U) || !__Utf8_Is_Scalar__(__Codepoint__))
    {
        return __Result__;
    }
    __Result__.__Codepoint__ = __Codepoint__;
    __Result__.__Width__ = __Width__;
    __Result__.__Valid__ = 1;
    return __Result__;
}
