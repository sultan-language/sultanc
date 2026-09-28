/* Declares bootstrap executable-image byte-buffer helpers. */

#ifndef SULTANC_BOOTSTRAP_LINK_BYTE_BUFFER_H
#define SULTANC_BOOTSTRAP_LINK_BYTE_BUFFER_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint8_t *data;
    size_t size;
    size_t capacity;
} __Bootstrap_Byte_Buffer__;

void __Bootstrap_Byte_Buffer_Init__(__Bootstrap_Byte_Buffer__ *buffer);
void __Bootstrap_Byte_Buffer_Destroy__(__Bootstrap_Byte_Buffer__ *buffer);
int __Bootstrap_Byte_Buffer_Reserve__(__Bootstrap_Byte_Buffer__ *buffer, size_t capacity);
int __Bootstrap_Byte_Buffer_Resize_Zero__(__Bootstrap_Byte_Buffer__ *buffer, size_t size);
int __Bootstrap_Byte_Buffer_Append__(__Bootstrap_Byte_Buffer__ *buffer, const void *data, size_t size);
int __Bootstrap_Byte_Buffer_Append_U8__(__Bootstrap_Byte_Buffer__ *buffer, uint8_t value);
int __Bootstrap_Byte_Buffer_Append_U16_LE__(__Bootstrap_Byte_Buffer__ *buffer, uint16_t value);
int __Bootstrap_Byte_Buffer_Append_U32_LE__(__Bootstrap_Byte_Buffer__ *buffer, uint32_t value);
int __Bootstrap_Byte_Buffer_Append_U64_LE__(__Bootstrap_Byte_Buffer__ *buffer, uint64_t value);
int __Bootstrap_Byte_Buffer_Append_U32_BE__(__Bootstrap_Byte_Buffer__ *buffer, uint32_t value);
int __Bootstrap_Byte_Buffer_Pad_To__(__Bootstrap_Byte_Buffer__ *buffer, size_t offset);
int __Bootstrap_Byte_Buffer_Write_U32_LE__(__Bootstrap_Byte_Buffer__ *buffer,
                                           size_t offset,
                                           uint32_t value);
int __Bootstrap_Byte_Buffer_Write_U64_LE__(__Bootstrap_Byte_Buffer__ *buffer,
                                           size_t offset,
                                           uint64_t value);

#endif
