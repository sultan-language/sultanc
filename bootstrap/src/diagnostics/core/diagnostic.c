/* Builds, updates, and copies structured diagnostics. */

#include "diagnostics/core/internal.h"
#include "diagnostics/catalog/catalog.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

/* Finds the diagnostic argument. */
static __Diagnostic_Argument__ *__Diagnostic_Find_Argument__(__Diagnostic__ *__Diagnostic_State__,
                                                             __Diagnostic_Argument_Key__ __Key__)
{
    /* Tracks the index. */
    size_t __Index__ = 0U;

    for (__Index__ = 0U; __Index__ < __Diagnostic_State__->__Argument_Count__; ++__Index__)
    {
        if (__Diagnostic_State__->__Arguments__[__Index__].__Key__ == __Key__)
        {
            return &__Diagnostic_State__->__Arguments__[__Index__];
        }
    }

    if (__Diagnostic_State__->__Argument_Count__ >= __Diagnostic_Max_Arguments__)
    {
        return NULL;
    }

    __Diagnostic_State__->__Arguments__[__Diagnostic_State__->__Argument_Count__].__Key__ = __Key__;
    ++__Diagnostic_State__->__Argument_Count__;
    return &__Diagnostic_State__->__Arguments__[__Diagnostic_State__->__Argument_Count__ - 1U];
}

/* Resets the diagnostic. */
void __Diagnostic_Reset__(__Diagnostic__ *__Diagnostic_State__)
{
    if (__Diagnostic_State__ == NULL)
    {
        return;
    }
    memset(__Diagnostic_State__, 0, sizeof(*__Diagnostic_State__));
}

/* Begins the diagnostic. */
void __Diagnostic_Begin__(__Diagnostic__ *__Diagnostic_State__,
                          __Error_Id__ __Id__,
                          __Source_Span__ __Span__)
{
    if (__Diagnostic_State__ == NULL)
    {
        return;
    }

    __Diagnostic_Reset__(__Diagnostic_State__);
    __Diagnostic_State__->__Id__ = __Id__;
    __Diagnostic_State__->__Severity__ = __Diagnostic_Severity_Of__(__Id__);
    __Diagnostic_State__->__Message_Key__ = __Diagnostic_Default_Message_Key__(__Id__);
    __Diagnostic_State__->__Primary_Role__ = __Diagnostic_Span_Role_Primary__;
    __Diagnostic_Capture_Source_Span__(&__Diagnostic_State__->__Primary_Span__, __Span__);
}

/* Begins the diagnostic without source. */
void __Diagnostic_Begin_Without_Source__(__Diagnostic__ *__Diagnostic_State__, __Error_Id__ __Id__)
{
    /* Stores the span. */
    __Source_Span__ __Span__;

    memset(&__Span__, 0, sizeof(__Span__));
    __Diagnostic_Begin__(__Diagnostic_State__, __Id__, __Span__);
}

/* Sets the diagnostic message key. */
void __Diagnostic_Set_Message_Key__(__Diagnostic__ *__Diagnostic_State__,
                                    __Diagnostic_Wording_Key__ __Message_Key__)
{
    if (__Diagnostic_State__ == NULL ||
        (size_t)__Message_Key__ >= (size_t)__Diagnostic_Wording_Key_Count__)
    {
        return;
    }
    __Diagnostic_State__->__Message_Key__ = __Message_Key__;
}

/* Sets the diagnostic primary role. */
void __Diagnostic_Set_Primary_Role__(__Diagnostic__ *__Diagnostic_State__,
                                     __Diagnostic_Span_Role__ __Role__)
{
    if (__Diagnostic_State__ == NULL)
    {
        return;
    }
    __Diagnostic_State__->__Primary_Role__ = __Role__;
}

/* Sets the diagnostic argument C string. */
int __Diagnostic_Set_Argument_Cstr__(__Diagnostic__ *__Diagnostic_State__,
                                     __Diagnostic_Argument_Key__ __Key__,
                                     const char *__Value__)
{
    /* References the argument. */
    __Diagnostic_Argument__ *__Argument__ = NULL;

    if (__Diagnostic_State__ == NULL || __Key__ == __Diagnostic_Argument_None__ ||
        __Value__ == NULL)
    {
        return 0;
    }

    __Argument__ = __Diagnostic_Find_Argument__(__Diagnostic_State__, __Key__);
    if (__Argument__ == NULL)
    {
        return 0;
    }

    (void)snprintf(__Argument__->__Value__, sizeof(__Argument__->__Value__), "%s", __Value__);
    return 1;
}

/* Sets the diagnostic argument text. */
int __Diagnostic_Set_Argument_Text__(__Diagnostic__ *__Diagnostic_State__,
                                     __Diagnostic_Argument_Key__ __Key__,
                                     __Text_Slice__ __Value__)
{
    /* References the argument. */
    __Diagnostic_Argument__ *__Argument__ = NULL;
    /* Stores the length. */
    size_t __Length__ = 0U;

    if (__Diagnostic_State__ == NULL || __Key__ == __Diagnostic_Argument_None__ ||
        (__Value__.__Data__ == NULL && __Value__.__Length__ != 0U))
    {
        return 0;
    }

    __Argument__ = __Diagnostic_Find_Argument__(__Diagnostic_State__, __Key__);
    if (__Argument__ == NULL)
    {
        return 0;
    }

    __Length__ = __Value__.__Length__;
    if (__Length__ >= sizeof(__Argument__->__Value__))
    {
        __Length__ = sizeof(__Argument__->__Value__) - 1U;
    }

    if (__Length__ != 0U)
    {
        memcpy(__Argument__->__Value__, __Value__.__Data__, __Length__);
    }
    __Argument__->__Value__[__Length__] = '\0';
    return 1;
}

/* Sets the diagnostic argument u 64. */
int __Diagnostic_Set_Argument_U64__(__Diagnostic__ *__Diagnostic_State__,
                                    __Diagnostic_Argument_Key__ __Key__,
                                    uint64_t __Value__)
{
    /* Stores the buffer. */
    char __Buffer__[32];

    (void)snprintf(__Buffer__, sizeof(__Buffer__), "%" PRIu64, __Value__);
    return __Diagnostic_Set_Argument_Cstr__(__Diagnostic_State__, __Key__, __Buffer__);
}

/* Adds the diagnostic related span. */
int __Diagnostic_Add_Related_Span__(__Diagnostic__ *__Diagnostic_State__,
                                    __Source_Span__ __Span__,
                                    __Diagnostic_Span_Role__ __Role__)
{
    /* References the related. */
    __Diagnostic_Related_Span__ *__Related__ = NULL;

    if (__Diagnostic_State__ == NULL ||
        __Diagnostic_State__->__Related_Span_Count__ >= __Diagnostic_Max_Related_Spans__)
    {
        return 0;
    }

    __Related__ =
        &__Diagnostic_State__->__Related_Spans__[__Diagnostic_State__->__Related_Span_Count__];
    __Diagnostic_Capture_Source_Span__(&__Related__->__Span__, __Span__);
    __Related__->__Role__ = __Role__;
    ++__Diagnostic_State__->__Related_Span_Count__;
    return 1;
}

/* Adds the diagnostic note. */
int __Diagnostic_Add_Note__(__Diagnostic__ *__Diagnostic_State__,
                            __Diagnostic_Wording_Key__ __Message_Key__)
{
    if (__Diagnostic_State__ == NULL ||
        __Diagnostic_State__->__Note_Count__ >= __Diagnostic_Max_Notes__)
    {
        return 0;
    }

    __Diagnostic_State__->__Notes__[__Diagnostic_State__->__Note_Count__].__Message_Key__ =
        __Message_Key__;
    ++__Diagnostic_State__->__Note_Count__;
    return 1;
}

/* Adds the diagnostic help. */
int __Diagnostic_Add_Help__(__Diagnostic__ *__Diagnostic_State__,
                            __Diagnostic_Wording_Key__ __Message_Key__)
{
    if (__Diagnostic_State__ == NULL ||
        __Diagnostic_State__->__Help_Count__ >= __Diagnostic_Max_Help__)
    {
        return 0;
    }

    __Diagnostic_State__->__Help__[__Diagnostic_State__->__Help_Count__].__Message_Key__ =
        __Message_Key__;
    ++__Diagnostic_State__->__Help_Count__;
    return 1;
}

/* Adds the diagnostic fix. */
int __Diagnostic_Add_Fix__(__Diagnostic__ *__Diagnostic_State__,
                           __Source_Span__ __Span__,
                           const char *__Replacement__,
                           __Diagnostic_Fix_Applicability__ __Applicability__,
                           __Diagnostic_Wording_Key__ __Message_Key__)
{
    /* References the fix. */
    __Diagnostic_Fix__ *__Fix__ = NULL;

    if (__Diagnostic_State__ == NULL || __Replacement__ == NULL ||
        __Diagnostic_State__->__Fix_Count__ >= __Diagnostic_Max_Fixes__)
    {
        return 0;
    }

    __Fix__ = &__Diagnostic_State__->__Fixes__[__Diagnostic_State__->__Fix_Count__];
    __Diagnostic_Capture_Source_Span__(&__Fix__->__Span__, __Span__);
    (void)snprintf(
        __Fix__->__Replacement__, sizeof(__Fix__->__Replacement__), "%s", __Replacement__);
    __Fix__->__Applicability__ = __Applicability__;
    __Fix__->__Message_Key__ = __Message_Key__;
    ++__Diagnostic_State__->__Fix_Count__;
    return 1;
}

/* Adds the diagnostic trace. */
int __Diagnostic_Add_Trace__(__Diagnostic__ *__Diagnostic_State__,
                             __Diagnostic_Wording_Key__ __Message_Key__,
                             const __Source_Span__ *__Optional_Span__)
{
    /* References the entry. */
    __Diagnostic_Trace_Entry__ *__Entry__ = NULL;

    if (__Diagnostic_State__ == NULL ||
        __Diagnostic_State__->__Trace_Count__ >= __Diagnostic_Max_Trace__)
    {
        return 0;
    }

    __Entry__ = &__Diagnostic_State__->__Trace__[__Diagnostic_State__->__Trace_Count__];
    __Entry__->__Message_Key__ = __Message_Key__;
    if (__Optional_Span__ != NULL)
    {
        __Diagnostic_Capture_Source_Span__(&__Entry__->__Span__, *__Optional_Span__);
        __Entry__->__Has_Span__ = 1;
    }
    ++__Diagnostic_State__->__Trace_Count__;
    return 1;
}

/* Copies the diagnostic. */
void __Diagnostic_Copy__(__Diagnostic__ *__Destination__, const __Diagnostic__ *__Source__)
{
    if (__Destination__ == NULL || __Source__ == NULL)
    {
        return;
    }
    *__Destination__ = *__Source__;
}
