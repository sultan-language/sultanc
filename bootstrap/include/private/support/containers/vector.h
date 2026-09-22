#ifndef SULTANC__SUPPORT_CONTAINERS_VECTOR_H__
#define SULTANC__SUPPORT_CONTAINERS_VECTOR_H__

#include <stddef.h>

typedef struct
{
    unsigned char *__Data__;
    size_t __Count__;
    size_t __Capacity__;
    size_t __Element_Size__;
} __Vector__;

void __Vector_Init__(__Vector__ *__Vector_State__, size_t __Element_Size__);

void *__Vector_Push__(__Vector__ *__Vector_State__, const void *__Element__);

void *__Vector_At__(__Vector__ *__Vector_State__, size_t __Index__);

const void *__Vector_At_Const__(const __Vector__ *__Vector_State__, size_t __Index__);

void __Vector_Destroy__(__Vector__ *__Vector_State__);

#endif
