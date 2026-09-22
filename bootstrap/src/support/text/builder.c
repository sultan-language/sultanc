#include "support/text/builder.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void __Text_Builder_Init__(__Text_Builder__ *__Builder__)
{
    if (__Builder__ == NULL)
    {
        return;
    }
    __Builder__->__Data__ = NULL;
    __Builder__->__Length__ = 0U;
    __Builder__->__Capacity__ = 0U;
}

static int __Text_Builder_Reserve__(__Text_Builder__ *__Builder__, size_t __Needed__)
{
    size_t __Capacity__ = 0U;
    char *__Data__ = NULL;
    if (__Builder__->__Capacity__ >= __Needed__)
    {
        return 1;
    }
    __Capacity__ = __Builder__->__Capacity__ == 0U ? 64U : __Builder__->__Capacity__;
    while (__Capacity__ < __Needed__)
    {
        if (__Capacity__ > (SIZE_MAX / 2U))
        {
            return 0;
        }
        __Capacity__ *= 2U;
    }
    __Data__ = (char *)realloc(__Builder__->__Data__, __Capacity__);
    if (__Data__ == NULL)
    {
        return 0;
    }
    __Builder__->__Data__ = __Data__;
    __Builder__->__Capacity__ = __Capacity__;
    return 1;
}

int __Text_Builder_Append__(__Text_Builder__ *__Builder__, const void *__Data__, size_t __Length__)
{
    if (__Builder__ == NULL || (__Data__ == NULL && __Length__ != 0U))
    {
        return 0;
    }
    if (!__Text_Builder_Reserve__(__Builder__, __Builder__->__Length__ + __Length__ + 1U))
    {
        return 0;
    }
    if (__Length__ != 0U)
    {
        memcpy(__Builder__->__Data__ + __Builder__->__Length__, __Data__, __Length__);
    }
    __Builder__->__Length__ += __Length__;
    __Builder__->__Data__[__Builder__->__Length__] = '\0';
    return 1;
}

int __Text_Builder_Append_Byte__(__Text_Builder__ *__Builder__, unsigned char __Byte__)
{
    return __Text_Builder_Append__(__Builder__, &__Byte__, 1U);
}

void __Text_Builder_Destroy__(__Text_Builder__ *__Builder__)
{
    if (__Builder__ == NULL)
    {
        return;
    }
    free(__Builder__->__Data__);
    __Builder__->__Data__ = NULL;
    __Builder__->__Length__ = 0U;
    __Builder__->__Capacity__ = 0U;
}
