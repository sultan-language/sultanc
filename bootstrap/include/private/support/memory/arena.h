#ifndef SULTANC__SUPPORT_MEMORY_ARENA_H__
#define SULTANC__SUPPORT_MEMORY_ARENA_H__

#include <stddef.h>

typedef struct __Arena_Block__ __Arena_Block__;

struct __Arena_Block__
{
    __Arena_Block__ *__Next__;
    size_t __Used__;
    size_t __Capacity__;
    unsigned char __Data__[];
};

typedef struct
{
    __Arena_Block__ *__First__;
    __Arena_Block__ *__Current__;
    size_t __Default_Block_Size__;
} __Arena__;

void __Arena_Init__(__Arena__ *__Arena__, size_t __Default_Block_Size__);

void *__Arena_Allocate__(__Arena__ *__Arena__, size_t __Size__, size_t __Alignment__);

char *__Arena_Copy_Text__(__Arena__ *__Arena__, const char *__Text__, size_t __Length__);

void __Arena_Destroy__(__Arena__ *__Arena__);

#endif
