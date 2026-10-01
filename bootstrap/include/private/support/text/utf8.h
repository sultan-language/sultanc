/* Declares UTF-8 validation and encoding helpers. */

#ifndef SULTANC__SUPPORT_TEXT_UTF8_H__
#define SULTANC__SUPPORT_TEXT_UTF8_H__

#include <stddef.h>
#include <stdint.h>

/* Defines the UTF 8 decode result structure. */
typedef struct
{
    /* Stores the codepoint. */
    uint32_t __Codepoint__;
    /* Stores the width. */
    size_t __Width__;
    /* Tracks the valid state. */
    int __Valid__;
} __Utf8_Decode_Result__;

/* Decodes the UTF 8. */
__Utf8_Decode_Result__ __Utf8_Decode__(const unsigned char *__Bytes__, size_t __Remaining__);

/* Checks whether the UTF 8 is continuation byte. */
int __Utf8_Is_Continuation_Byte__(unsigned char __Byte__);

/* Checks whether the UTF 8 is scalar. */
int __Utf8_Is_Scalar__(uint32_t __Codepoint__);

/* Encodes the UTF 8. */
size_t __Utf8_Encode__(uint32_t __Codepoint__, unsigned char __Out__[4]);

#endif
