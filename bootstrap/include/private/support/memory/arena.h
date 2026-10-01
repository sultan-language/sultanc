/* Declares bootstrap arena allocation. */

#ifndef SULTANC__SUPPORT_MEMORY_ARENA_H__
#define SULTANC__SUPPORT_MEMORY_ARENA_H__

#include <stddef.h>

/* Defines the arena block structure. */
typedef struct __Arena_Block__ __Arena_Block__;

/* Defines the arena block structure. */
struct __Arena_Block__
{
    /* References the next. */
    __Arena_Block__ *__Next__;
    /* Stores the used. */
    size_t __Used__;
    /* Stores the capacity. */
    size_t __Capacity__;
    /* Stores the data. */
    unsigned char __Data__[];
};

/* Defines the arena structure. */
typedef struct
{
    /* References the first. */
    __Arena_Block__ *__First__;
    /* References the current. */
    __Arena_Block__ *__Current__;
    /* Stores the default block size. */
    size_t __Default_Block_Size__;
} __Arena__;

/* Initializes the arena. */
void __Arena_Init__(__Arena__ *__Arena__, size_t __Default_Block_Size__);

/* Allocates the arena. */
void *__Arena_Allocate__(__Arena__ *__Arena__, size_t __Size__, size_t __Alignment__);

/* Copies the arena text. */
char *__Arena_Copy_Text__(__Arena__ *__Arena__, const char *__Text__, size_t __Length__);

/* Releases the arena. */
void __Arena_Destroy__(__Arena__ *__Arena__);

#endif
