#ifndef SULTANC__CORE_SOURCE_H__
#define SULTANC__CORE_SOURCE_H__

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    const char *__Name__;
    char *__Bytes__;
    size_t __Length__;
} __Source_File__;

typedef struct
{
    /*
     * Borrowed source identity.
     *
     * Every real parser/semantic position carries the originating source unit.
     * Diagnostics copy the name and excerpt before the source can be destroyed.
     */
    const __Source_File__ *__Source__;
    size_t __Offset__;
    uint32_t __Line__;
    uint32_t __Column__;
} __Source_Position__;

typedef struct
{
    __Source_Position__ __Start__;
    __Source_Position__ __End__;
} __Source_Span__;

typedef struct
{
    const char *__Data__;
    size_t __Length__;
} __Text_Slice__;

int __Source_Load__(const char *__Path__, __Source_File__ *__Out_Source__);

void __Source_Destroy__(__Source_File__ *__Source__);

__Text_Slice__
__Source_Slice__(const __Source_File__ *__Source__, size_t __Start__, size_t __End__);

int __Text_Equals_Cstr__(__Text_Slice__ __Text__, const char *__Cstr__);

#endif
