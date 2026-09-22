#include "core/source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int __Source_Load__(const char *__Path__, __Source_File__ *__Out_Source__)
{
    FILE *__File__ = NULL;
    long __Size__ = 0;
    size_t __Read__ = 0;
    char *__Bytes__ = NULL;

    if (__Path__ == NULL || __Out_Source__ == NULL)
    {
        return 0;
    }

    memset(__Out_Source__, 0, sizeof(*__Out_Source__));
    __File__ = fopen(__Path__, "rb");
    if (__File__ == NULL)
    {
        return 0;
    }
    if (fseek(__File__, 0, SEEK_END) != 0)
    {
        fclose(__File__);
        return 0;
    }
    __Size__ = ftell(__File__);
    if (__Size__ < 0 || fseek(__File__, 0, SEEK_SET) != 0)
    {
        fclose(__File__);
        return 0;
    }

    __Bytes__ = (char *)malloc((size_t)__Size__ + 1U);
    if (__Bytes__ == NULL)
    {
        fclose(__File__);
        return 0;
    }
    __Read__ = fread(__Bytes__, 1U, (size_t)__Size__, __File__);
    fclose(__File__);
    if (__Read__ != (size_t)__Size__)
    {
        free(__Bytes__);
        return 0;
    }

    __Bytes__[__Read__] = '\0';
    __Out_Source__->__Name__ = __Path__;
    __Out_Source__->__Bytes__ = __Bytes__;
    __Out_Source__->__Length__ = __Read__;
    return 1;
}
