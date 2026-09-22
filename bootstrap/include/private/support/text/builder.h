#ifndef SULTANC__SUPPORT_TEXT_BUILDER_H__
#define SULTANC__SUPPORT_TEXT_BUILDER_H__

#include <stddef.h>

typedef struct
{
    char *__Data__;
    size_t __Length__;
    size_t __Capacity__;
} __Text_Builder__;

void __Text_Builder_Init__(__Text_Builder__ *__Builder__);

int __Text_Builder_Append__(__Text_Builder__ *__Builder__, const void *__Data__, size_t __Length__);

int __Text_Builder_Append_Byte__(__Text_Builder__ *__Builder__, unsigned char __Byte__);

void __Text_Builder_Destroy__(__Text_Builder__ *__Builder__);

#endif
