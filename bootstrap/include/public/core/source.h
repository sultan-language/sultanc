/* Defines source files, positions, and text slices. */

#ifndef SULTANC__CORE_SOURCE_H__
#define SULTANC__CORE_SOURCE_H__

#include <stddef.h>
#include <stdint.h>

/* Defines the source file structure. */
typedef struct
{
    /* References the name. */
    const char *__Name__;
    /* References the bytes. */
    char *__Bytes__;
    /* Stores the length. */
    size_t __Length__;
} __Source_File__;

/* Defines the source position structure. */
typedef struct
{
    /* Borrowed source identity carried by parser and semantic positions. */
    const __Source_File__ *__Source__;
    /* Stores the offset. */
    size_t __Offset__;
    /* Stores the line. */
    uint32_t __Line__;
    /* Stores the column. */
    uint32_t __Column__;
} __Source_Position__;

/* Defines the source span structure. */
typedef struct
{
    /* Stores the start. */
    __Source_Position__ __Start__;
    /* Stores the end. */
    __Source_Position__ __End__;
} __Source_Span__;

/* Defines the text slice structure. */
typedef struct
{
    /* References the data. */
    const char *__Data__;
    /* Stores the length. */
    size_t __Length__;
} __Text_Slice__;

/* Loads the source. */
int __Source_Load__(const char *__Path__, __Source_File__ *__Out_Source__);

/* Releases the source. */
void __Source_Destroy__(__Source_File__ *__Source__);

/* Returns the source. */
__Text_Slice__
__Source_Slice__(const __Source_File__ *__Source__, size_t __Start__, size_t __End__);

/* Compares the text C string. */
int __Text_Equals_Cstr__(__Text_Slice__ __Text__, const char *__Cstr__);

#endif
