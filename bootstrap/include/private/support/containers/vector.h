/* Declares the bootstrap dynamic vector. */

#ifndef SULTANC__SUPPORT_CONTAINERS_VECTOR_H__
#define SULTANC__SUPPORT_CONTAINERS_VECTOR_H__

#include <stddef.h>

/* Defines the vector structure. */
typedef struct
{
    /* References the data. */
    unsigned char *__Data__;
    /* Stores the count. */
    size_t __Count__;
    /* Stores the capacity. */
    size_t __Capacity__;
    /* Stores the element size. */
    size_t __Element_Size__;
} __Vector__;

/* Initializes the vector. */
void __Vector_Init__(__Vector__ *__Vector_State__, size_t __Element_Size__);

/* Adds the vector. */
void *__Vector_Push__(__Vector__ *__Vector_State__, const void *__Element__);

/* Returns the vector at. */
void *__Vector_At__(__Vector__ *__Vector_State__, size_t __Index__);

/* Returns the vector at const. */
const void *__Vector_At_Const__(const __Vector__ *__Vector_State__, size_t __Index__);

/* Releases the vector. */
void __Vector_Destroy__(__Vector__ *__Vector_State__);

#endif
