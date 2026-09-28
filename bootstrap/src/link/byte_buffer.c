/* Implements bootstrap executable-image byte-buffer helpers. */

#include "link/byte_buffer.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

void __Bootstrap_Byte_Buffer_Init__(__Bootstrap_Byte_Buffer__ *buffer)
{
    if (buffer == NULL)
        return;
    buffer->data = NULL;
    buffer->size = 0U;
    buffer->capacity = 0U;
}

void __Bootstrap_Byte_Buffer_Destroy__(__Bootstrap_Byte_Buffer__ *buffer)
{
    if (buffer == NULL)
        return;
    free(buffer->data);
    __Bootstrap_Byte_Buffer_Init__(buffer);
}

int __Bootstrap_Byte_Buffer_Reserve__(__Bootstrap_Byte_Buffer__ *buffer, size_t capacity)
{
    uint8_t *replacement;
    size_t grown;

    if (buffer == NULL)
        return 0;
    if (capacity <= buffer->capacity)
        return 1;
    grown = buffer->capacity == 0U ? 256U : buffer->capacity;
    while (grown < capacity)
    {
        if (grown > SIZE_MAX / 2U)
        {
            grown = capacity;
            break;
        }
        grown *= 2U;
    }
    replacement = (uint8_t *)realloc(buffer->data, grown);
    if (replacement == NULL)
        return 0;
    buffer->data = replacement;
    buffer->capacity = grown;
    return 1;
}

int __Bootstrap_Byte_Buffer_Resize_Zero__(__Bootstrap_Byte_Buffer__ *buffer, size_t size)
{
    size_t old_size;

    if (buffer == NULL)
        return 0;
    old_size = buffer->size;
    if (!__Bootstrap_Byte_Buffer_Reserve__(buffer, size))
        return 0;
    if (size > old_size)
        memset(buffer->data + old_size, 0, size - old_size);
    buffer->size = size;
    return 1;
}

int __Bootstrap_Byte_Buffer_Append__(__Bootstrap_Byte_Buffer__ *buffer,
                                     const void *data,
                                     size_t size)
{
    if (buffer == NULL || (size != 0U && data == NULL) || size > SIZE_MAX - buffer->size)
        return 0;
    if (!__Bootstrap_Byte_Buffer_Reserve__(buffer, buffer->size + size))
        return 0;
    if (size != 0U)
        memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
    return 1;
}

int __Bootstrap_Byte_Buffer_Append_U8__(__Bootstrap_Byte_Buffer__ *buffer, uint8_t value)
{
    return __Bootstrap_Byte_Buffer_Append__(buffer, &value, 1U);
}

int __Bootstrap_Byte_Buffer_Append_U16_LE__(__Bootstrap_Byte_Buffer__ *buffer, uint16_t value)
{
    uint8_t bytes[2];
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8U);
    return __Bootstrap_Byte_Buffer_Append__(buffer, bytes, sizeof(bytes));
}

int __Bootstrap_Byte_Buffer_Append_U32_LE__(__Bootstrap_Byte_Buffer__ *buffer, uint32_t value)
{
    uint8_t bytes[4];
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8U);
    bytes[2] = (uint8_t)(value >> 16U);
    bytes[3] = (uint8_t)(value >> 24U);
    return __Bootstrap_Byte_Buffer_Append__(buffer, bytes, sizeof(bytes));
}

int __Bootstrap_Byte_Buffer_Append_U64_LE__(__Bootstrap_Byte_Buffer__ *buffer, uint64_t value)
{
    uint8_t bytes[8];
    size_t i;
    for (i = 0U; i < sizeof(bytes); ++i)
        bytes[i] = (uint8_t)(value >> (i * 8U));
    return __Bootstrap_Byte_Buffer_Append__(buffer, bytes, sizeof(bytes));
}

int __Bootstrap_Byte_Buffer_Append_U32_BE__(__Bootstrap_Byte_Buffer__ *buffer, uint32_t value)
{
    uint8_t bytes[4];
    bytes[0] = (uint8_t)(value >> 24U);
    bytes[1] = (uint8_t)(value >> 16U);
    bytes[2] = (uint8_t)(value >> 8U);
    bytes[3] = (uint8_t)value;
    return __Bootstrap_Byte_Buffer_Append__(buffer, bytes, sizeof(bytes));
}

int __Bootstrap_Byte_Buffer_Pad_To__(__Bootstrap_Byte_Buffer__ *buffer, size_t offset)
{
    if (buffer == NULL || offset < buffer->size)
        return 0;
    return __Bootstrap_Byte_Buffer_Resize_Zero__(buffer, offset);
}

int __Bootstrap_Byte_Buffer_Write_U32_LE__(__Bootstrap_Byte_Buffer__ *buffer,
                                           size_t offset,
                                           uint32_t value)
{
    if (buffer == NULL || offset > buffer->size || 4U > buffer->size - offset)
        return 0;
    buffer->data[offset] = (uint8_t)value;
    buffer->data[offset + 1U] = (uint8_t)(value >> 8U);
    buffer->data[offset + 2U] = (uint8_t)(value >> 16U);
    buffer->data[offset + 3U] = (uint8_t)(value >> 24U);
    return 1;
}

int __Bootstrap_Byte_Buffer_Write_U64_LE__(__Bootstrap_Byte_Buffer__ *buffer,
                                           size_t offset,
                                           uint64_t value)
{
    size_t i;
    if (buffer == NULL || offset > buffer->size || 8U > buffer->size - offset)
        return 0;
    for (i = 0U; i < 8U; ++i)
        buffer->data[offset + i] = (uint8_t)(value >> (i * 8U));
    return 1;
}
