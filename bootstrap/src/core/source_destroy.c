#include "core/source.h"

#include <stdlib.h>

void __Source_Destroy__(__Source_File__ *__Source__)
{
    if (__Source__ == NULL)
    {
        return;
    }
    free(__Source__->__Bytes__);
    __Source__->__Bytes__ = NULL;
    __Source__->__Length__ = 0U;
    __Source__->__Name__ = NULL;
}
