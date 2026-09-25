/* Declares growable text builders. */

#ifndef SULTANC__SUPPORT_TEXT_BUILDER_H__
#define SULTANC__SUPPORT_TEXT_BUILDER_H__

#include <stddef.h>

/* Defines the text builder structure. */
typedef struct
{
    /* References the data. */
    char *__Data__;
    /* Stores the length. */
    size_t __Length__;
    /* Stores the capacity. */
    size_t __Capacity__;
} __Text_Builder__;

/* Initializes the text builder. */
void __Text_Builder_Init__(__Text_Builder__ *__Builder__);

/* Appends the text builder. */
int __Text_Builder_Append__(__Text_Builder__ *__Builder__, const void *__Data__, size_t __Length__);

/* Appends the text builder byte. */
int __Text_Builder_Append_Byte__(__Text_Builder__ *__Builder__, unsigned char __Byte__);

/* Releases the text builder. */
void __Text_Builder_Destroy__(__Text_Builder__ *__Builder__);

#endif
