#include "support/memory/alignment.h"

size_t __Align_Up__(size_t __Value__, size_t __Alignment__)
{
    size_t __Mask__;

    __Mask__ = __Alignment__ - 1U;
    return (__Value__ + __Mask__) & ~__Mask__;
}
