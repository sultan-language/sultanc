#include "core/source.h"

__Text_Slice__ __Source_Slice__(const __Source_File__ *__Source__, size_t __Start__, size_t __End__)
{
    __Text_Slice__ __Result__ = {0};
    if (__Source__ == NULL || __Start__ > __End__ || __End__ > __Source__->__Length__)
    {
        return __Result__;
    }
    __Result__.__Data__ = __Source__->__Bytes__ + __Start__;
    __Result__.__Length__ = __End__ - __Start__;
    return __Result__;
}
