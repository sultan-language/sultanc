#include "support/text/equality.h"

#include <string.h>

int __Text_Slice_Equals__(__Text_Slice__ __Left__, __Text_Slice__ __Right__)
{
    if (__Left__.__Length__ != __Right__.__Length__)
    {
        return 0;
    }

    if (__Left__.__Length__ == 0U)
    {
        return 1;
    }

    return memcmp(__Left__.__Data__, __Right__.__Data__, __Left__.__Length__) == 0;
}
