#include "support/memory/arena.h"
#include "support/memory/alignment.h"

#include <stdlib.h>
#include <string.h>

static __Arena_Block__ *__Arena_New_Block__(size_t __Capacity__)
{
    __Arena_Block__ *__Block__ = (__Arena_Block__ *)malloc(sizeof(*__Block__) + __Capacity__);
    if (__Block__ == NULL)
    {
        return NULL;
    }
    __Block__->__Next__ = NULL;
    __Block__->__Used__ = 0U;
    __Block__->__Capacity__ = __Capacity__;
    return __Block__;
}

void __Arena_Init__(__Arena__ *__Arena_State__, size_t __Default_Block_Size__)
{
    if (__Arena_State__ == NULL)
    {
        return;
    }
    __Arena_State__->__First__ = NULL;
    __Arena_State__->__Current__ = NULL;
    __Arena_State__->__Default_Block_Size__ =
        __Default_Block_Size__ < 1024U ? 1024U : __Default_Block_Size__;
}

void *__Arena_Allocate__(__Arena__ *__Arena_State__, size_t __Size__, size_t __Alignment__)
{
    __Arena_Block__ *__Block__ = NULL;
    size_t __Offset__ = 0U;
    size_t __Capacity__ = 0U;

    if (__Arena_State__ == NULL || __Size__ == 0U)
    {
        return NULL;
    }
    if (__Alignment__ == 0U || (__Alignment__ & (__Alignment__ - 1U)) != 0U)
    {
        return NULL;
    }

    __Block__ = __Arena_State__->__Current__;
    if (__Block__ != NULL)
    {
        __Offset__ = __Align_Up__(__Block__->__Used__, __Alignment__);
    }
    if (__Block__ == NULL || __Offset__ + __Size__ > __Block__->__Capacity__)
    {
        __Capacity__ = __Arena_State__->__Default_Block_Size__;
        if (__Capacity__ < __Size__ + __Alignment__)
        {
            __Capacity__ = __Size__ + __Alignment__;
        }
        __Block__ = __Arena_New_Block__(__Capacity__);
        if (__Block__ == NULL)
        {
            return NULL;
        }
        if (__Arena_State__->__Current__ != NULL)
        {
            __Arena_State__->__Current__->__Next__ = __Block__;
        }
        else
        {
            __Arena_State__->__First__ = __Block__;
        }
        __Arena_State__->__Current__ = __Block__;
        __Offset__ = __Align_Up__(0U, __Alignment__);
    }

    __Block__->__Used__ = __Offset__ + __Size__;
    return __Block__->__Data__ + __Offset__;
}

char *__Arena_Copy_Text__(__Arena__ *__Arena_State__, const char *__Text__, size_t __Length__)
{
    char *__Copy__ = (char *)__Arena_Allocate__(__Arena_State__, __Length__ + 1U, _Alignof(char));
    if (__Copy__ == NULL)
    {
        return NULL;
    }
    if (__Length__ != 0U)
    {
        memcpy(__Copy__, __Text__, __Length__);
    }
    __Copy__[__Length__] = '\0';
    return __Copy__;
}

void __Arena_Destroy__(__Arena__ *__Arena_State__)
{
    __Arena_Block__ *__Block__ = NULL;
    __Arena_Block__ *__Next__ = NULL;
    if (__Arena_State__ == NULL)
    {
        return;
    }
    __Block__ = __Arena_State__->__First__;
    while (__Block__ != NULL)
    {
        __Next__ = __Block__->__Next__;
        free(__Block__);
        __Block__ = __Next__;
    }
    __Arena_State__->__First__ = NULL;
    __Arena_State__->__Current__ = NULL;
}
