#ifndef SULTANC__SUPPORT_TEXT_UTF8_H__
#define SULTANC__SUPPORT_TEXT_UTF8_H__

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint32_t __Codepoint__;
    size_t __Width__;
    int __Valid__;
} __Utf8_Decode_Result__;

__Utf8_Decode_Result__ __Utf8_Decode__(const unsigned char *__Bytes__, size_t __Remaining__);

int __Utf8_Is_Continuation_Byte__(unsigned char __Byte__);

int __Utf8_Is_Scalar__(uint32_t __Codepoint__);

size_t __Utf8_Encode__(uint32_t __Codepoint__, unsigned char __Out__[4]);

#endif
