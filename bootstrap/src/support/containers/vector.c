#include "support/containers/vector.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void __Vector_Init__(__Vector__ *__Vector_State__, size_t __Element_Size__)
{
    if (__Vector_State__ == NULL)
    {
        return;
    }
    __Vector_State__->__Data__ = NULL;
    __Vector_State__->__Count__ = 0U;
    __Vector_State__->__Capacity__ = 0U;
    __Vector_State__->__Element_Size__ = __Element_Size__;
}

static int __Vector_Reserve__(__Vector__ *__Vector_State__, size_t __Needed__)
{
    size_t __Capacity__ = 0U;
    unsigned char *__Data__ = NULL;
    if (__Vector_State__->__Capacity__ >= __Needed__)
    {
        return 1;
    }
    __Capacity__ = __Vector_State__->__Capacity__ == 0U ? 8U : __Vector_State__->__Capacity__;
    while (__Capacity__ < __Needed__)
    {
        if (__Capacity__ > SIZE_MAX / 2U)
        {
            return 0;
        }
        __Capacity__ *= 2U;
    }
    if (__Vector_State__->__Element_Size__ != 0U &&
        __Capacity__ > SIZE_MAX / __Vector_State__->__Element_Size__)
    {
        return 0;
    }
    __Data__ = (unsigned char *)realloc(__Vector_State__->__Data__,
                                        __Capacity__ * __Vector_State__->__Element_Size__);
    if (__Data__ == NULL)
    {
        return 0;
    }
    __Vector_State__->__Data__ = __Data__;
    __Vector_State__->__Capacity__ = __Capacity__;
    return 1;
}

void *__Vector_Push__(__Vector__ *__Vector_State__, const void *__Element__)
{
    void *__Destination__ = NULL;
    if (__Vector_State__ == NULL || __Vector_State__->__Element_Size__ == 0U || __Element__ == NULL)
    {
        return NULL;
    }
    if (!__Vector_Reserve__(__Vector_State__, __Vector_State__->__Count__ + 1U))
    {
        return NULL;
    }
    __Destination__ = __Vector_State__->__Data__ +
                      __Vector_State__->__Count__ * __Vector_State__->__Element_Size__;
    memcpy(__Destination__, __Element__, __Vector_State__->__Element_Size__);
    ++__Vector_State__->__Count__;
    return __Destination__;
}

const void *__Vector_At_Const__(const __Vector__ *__Vector_State__, size_t __Index__)
{
    if (__Vector_State__ == NULL || __Index__ >= __Vector_State__->__Count__)
    {
        return NULL;
    }
    return __Vector_State__->__Data__ + __Index__ * __Vector_State__->__Element_Size__;
}

void *__Vector_At__(__Vector__ *__Vector_State__, size_t __Index__)
{
    return (void *)__Vector_At_Const__(__Vector_State__, __Index__);
}

void __Vector_Destroy__(__Vector__ *__Vector_State__)
{
    if (__Vector_State__ == NULL)
    {
        return;
    }
    free(__Vector_State__->__Data__);
    __Vector_State__->__Data__ = NULL;
    __Vector_State__->__Count__ = 0U;
    __Vector_State__->__Capacity__ = 0U;
}
