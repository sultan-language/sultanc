#ifndef SULTANC__KERNEL_MEMORY_MEMORY_H__
#define SULTANC__KERNEL_MEMORY_MEMORY_H__

#include "kernel/type/type.h"

#include <stddef.h>

typedef struct
{
    size_t __Minimum_Capacity__;
    size_t __Growth_Factor__;
} __Memory_Vector_Growth_Policy__;

/* True when moving the value transfers at least one release obligation. */
int __Memory_Type_Is_Owned__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__);

/* True for a direct non-owning view whose lifetime is tied to another value. */
int __Memory_Type_Is_View__(const __Ast_Type__ *__Type__);

/* True when a value directly or recursively contains a reference/slice view. */
int __Memory_Type_Contains_View__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__);

const __Memory_Vector_Growth_Policy__ *__Memory_Vector_Growth_Policy_View__(void);

int __Memory_Element_Bytes__(size_t __Element_Size__,
                             size_t __Element_Count__,
                             size_t *__Out_Bytes__);

size_t __Memory_Vector_Initial_Capacity__(size_t __Length__);

int __Memory_Vector_Grow_Capacity__(size_t __Current_Capacity__,
                                    size_t __Required_Capacity__,
                                    size_t *__Out_Capacity__);

#endif
