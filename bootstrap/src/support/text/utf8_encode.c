/* Validates and encodes UTF-8 scalar values. */

#include "support/text/utf8.h"

/* Checks whether the UTF 8 is continuation byte. */
int __Utf8_Is_Continuation_Byte__(unsigned char __Byte__)
{
    return (__Byte__ & 0xC0U) == 0x80U;
}

/* Checks whether the UTF 8 is scalar. */
int __Utf8_Is_Scalar__(uint32_t __Codepoint__)
{
    return __Codepoint__ <= 0x10FFFFU && !(__Codepoint__ >= 0xD800U && __Codepoint__ <= 0xDFFFU);
}

/* Encodes the UTF 8. */
size_t __Utf8_Encode__(uint32_t __Codepoint__, unsigned char __Out__[4])
{
    if (__Out__ == NULL || !__Utf8_Is_Scalar__(__Codepoint__))
    {
        return 0U;
    }
    if (__Codepoint__ <= 0x7FU)
    {
        __Out__[0] = (unsigned char)__Codepoint__;
        return 1U;
    }
    if (__Codepoint__ <= 0x7FFU)
    {
        __Out__[0] = (unsigned char)(0xC0U | (__Codepoint__ >> 6U));
        __Out__[1] = (unsigned char)(0x80U | (__Codepoint__ & 0x3FU));
        return 2U;
    }
    if (__Codepoint__ <= 0xFFFFU)
    {
        __Out__[0] = (unsigned char)(0xE0U | (__Codepoint__ >> 12U));
        __Out__[1] = (unsigned char)(0x80U | ((__Codepoint__ >> 6U) & 0x3FU));
        __Out__[2] = (unsigned char)(0x80U | (__Codepoint__ & 0x3FU));
        return 3U;
    }
    __Out__[0] = (unsigned char)(0xF0U | (__Codepoint__ >> 18U));
    __Out__[1] = (unsigned char)(0x80U | ((__Codepoint__ >> 12U) & 0x3FU));
    __Out__[2] = (unsigned char)(0x80U | ((__Codepoint__ >> 6U) & 0x3FU));
    __Out__[3] = (unsigned char)(0x80U | (__Codepoint__ & 0x3FU));
    return 4U;
}
