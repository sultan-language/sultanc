/* Captures source spans and excerpts for diagnostics. */

#include "diagnostics/core/internal.h"
#include "support/text/utf8.h"

#include <stdio.h>
#include <string.h>

/* Finds the diagnostic line start. */
static size_t __Diagnostic_Find_Line_Start__(const __Source_File__ *__Source__, size_t __Offset__)
{
    /* Stores the cursor. */
    size_t __Cursor__ = __Offset__ > __Source__->__Length__ ? __Source__->__Length__ : __Offset__;

    while (__Cursor__ > 0U && __Source__->__Bytes__[__Cursor__ - 1U] != '\n')
    {
        --__Cursor__;
    }
    return __Cursor__;
}

/* Finds the diagnostic line end. */
static size_t __Diagnostic_Find_Line_End__(const __Source_File__ *__Source__, size_t __Offset__)
{
    /* Stores the cursor. */
    size_t __Cursor__ = __Offset__ > __Source__->__Length__ ? __Source__->__Length__ : __Offset__;

    while (__Cursor__ < __Source__->__Length__ && __Source__->__Bytes__[__Cursor__] != '\n' &&
           __Source__->__Bytes__[__Cursor__] != '\r')
    {
        ++__Cursor__;
    }
    return __Cursor__;
}

/* Returns the diagnostic clamp excerpt start. */
static size_t __Diagnostic_Clamp_Excerpt_Start__(const __Source_File__ *__Source__,
                                                 size_t __Line_Start__,
                                                 size_t __Line_End__,
                                                 size_t __Focus_Offset__)
{
    /* Stores the capacity. */
    size_t __Capacity__ = __Diagnostic_Max_Source_Line__ - 1U;
    /* Stores the start. */
    size_t __Start__ = __Line_Start__;

    if (__Line_End__ - __Line_Start__ <= __Capacity__)
    {
        return __Start__;
    }

    if (__Focus_Offset__ > __Line_Start__ + (__Capacity__ / 2U))
    {
        __Start__ = __Focus_Offset__ - (__Capacity__ / 2U);
    }
    if (__Start__ + __Capacity__ > __Line_End__)
    {
        __Start__ = __Line_End__ - __Capacity__;
    }

    /*
     * Do not begin an excerpt in the middle of a UTF-8 continuation sequence.
     */
    while (__Start__ > __Line_Start__ &&
           __Utf8_Is_Continuation_Byte__((unsigned char)__Source__->__Bytes__[__Start__]))
    {
        --__Start__;
    }
    return __Start__;
}

/* Captures the diagnostic source span. */
void __Diagnostic_Capture_Source_Span__(__Diagnostic_Source_Span__ *__Destination__,
                                        __Source_Span__ __Source_Span__)
{
    /* References the source. */
    const __Source_File__ *__Source__ = __Source_Span__.__Start__.__Source__;
    /* Stores the line start. */
    size_t __Line_Start__ = 0U;
    /* Stores the line end. */
    size_t __Line_End__ = 0U;
    /* Stores the excerpt start. */
    size_t __Excerpt_Start__ = 0U;
    /* Stores the copy length. */
    size_t __Copy_Length__ = 0U;

    if (__Destination__ == NULL)
    {
        return;
    }

    memset(__Destination__, 0, sizeof(*__Destination__));
    __Destination__->__Start__ = __Source_Span__.__Start__;
    __Destination__->__End__ = __Source_Span__.__End__;

    /* Copy source identity so diagnostics never retain source-unit pointers. */
    __Destination__->__Start__.__Source__ = NULL;
    __Destination__->__End__.__Source__ = NULL;

    if (__Source__ == NULL || __Source__->__Name__ == NULL || __Source__->__Bytes__ == NULL)
    {
        return;
    }

    (void)snprintf(__Destination__->__Source_Name__,
                   sizeof(__Destination__->__Source_Name__),
                   "%s",
                   __Source__->__Name__);

    __Line_Start__ =
        __Diagnostic_Find_Line_Start__(__Source__, __Source_Span__.__Start__.__Offset__);
    __Line_End__ = __Diagnostic_Find_Line_End__(__Source__, __Source_Span__.__Start__.__Offset__);
    __Excerpt_Start__ = __Diagnostic_Clamp_Excerpt_Start__(
        __Source__, __Line_Start__, __Line_End__, __Source_Span__.__Start__.__Offset__);

    __Copy_Length__ = __Line_End__ - __Excerpt_Start__;
    if (__Copy_Length__ >= sizeof(__Destination__->__Line_Text__))
    {
        __Copy_Length__ = sizeof(__Destination__->__Line_Text__) - 1U;
    }

    if (__Copy_Length__ != 0U)
    {
        memcpy(__Destination__->__Line_Text__,
               __Source__->__Bytes__ + __Excerpt_Start__,
               __Copy_Length__);
    }
    __Destination__->__Line_Text__[__Copy_Length__] = '\0';
    __Destination__->__Line_Text_Length__ = __Copy_Length__;
    __Destination__->__Line_Start_Offset__ = __Line_Start__;
    __Destination__->__Excerpt_Start_Offset__ = __Excerpt_Start__;
    __Destination__->__Has_Source__ = 1;
}
