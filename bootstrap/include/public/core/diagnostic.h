/* Defines the public structured diagnostic model. */

#ifndef SULTANC__CORE_DIAGNOSTIC_H__
#define SULTANC__CORE_DIAGNOSTIC_H__

#include "source.h"

#include <stddef.h>
#include <stdint.h>

/* Defines the diagnostic severity values. */
typedef enum
{
    /* Represents the diagnostic severity error value. */
    __Diagnostic_Severity_Error__,
} __Diagnostic_Severity__;

/* Defines the diagnostic wording key values. */
typedef enum
{
/* Expands the sultanc diagnostic wording key macro. */
#define SULTANC_DIAGNOSTIC_WORDING_KEY(__Key__, __Placeholders__) __Key__,
#include "diagnostic_wording_keys.def"
#undef SULTANC_DIAGNOSTIC_WORDING_KEY
    /* Represents the diagnostic wording key count value. */
    __Diagnostic_Wording_Key_Count__
} __Diagnostic_Wording_Key__;

/* Defines the error ID values. */
typedef enum
{
/* Expands the sultanc diagnostic macro. */
#define SULTANC_DIAGNOSTIC(__Id__, __Code__, __Severity__, __Message_Key__) __Id__,
#include "diagnostic_catalog.def"
#undef SULTANC_DIAGNOSTIC
    /* Represents the error ID count value. */
    __Error_Id_Count__
} __Error_Id__;

/* Defines the diagnostic argument key values. */
typedef enum
{
    /* Represents the diagnostic argument none value. */
    __Diagnostic_Argument_None__,
    /* Represents the diagnostic argument value value. */
    __Diagnostic_Argument_Value__,
    /* Represents the diagnostic argument expected value. */
    __Diagnostic_Argument_Expected__,
    /* Represents the diagnostic argument found value. */
    __Diagnostic_Argument_Found__,
    /* Represents the diagnostic argument index value. */
    __Diagnostic_Argument_Index__,
    /* Represents the diagnostic argument length value. */
    __Diagnostic_Argument_Length__,
    /* Represents the diagnostic argument range value. */
    __Diagnostic_Argument_Range__,
    /* Represents the diagnostic argument pattern value. */
    __Diagnostic_Argument_Pattern__,
    /* Represents the diagnostic argument path value. */
    __Diagnostic_Argument_Path__,
    /* Represents the diagnostic argument function value. */
    __Diagnostic_Argument_Function__,
} __Diagnostic_Argument_Key__;

/* Defines the diagnostic span role values. */
typedef enum
{
    /* Represents the diagnostic span role none value. */
    __Diagnostic_Span_Role_None__,
    /* Represents the diagnostic span role primary value. */
    __Diagnostic_Span_Role_Primary__,
    /* Represents the diagnostic span role invalid use value. */
    __Diagnostic_Span_Role_Invalid_Use__,
    /* Represents the diagnostic span role move origin value. */
    __Diagnostic_Span_Role_Move_Origin__,
    /* Represents the diagnostic span role original owner value. */
    __Diagnostic_Span_Role_Original_Owner__,
    /* Represents the diagnostic span role uninitialized declaration value. */
    __Diagnostic_Span_Role_Uninitialized_Declaration__,
    /* Represents the diagnostic span role conflicting borrow value. */
    __Diagnostic_Span_Role_Conflicting_Borrow__,
    /* Represents the diagnostic span role mutable borrow start value. */
    __Diagnostic_Span_Role_Mutable_Borrow_Start__,
    /* Represents the diagnostic span role reference escape value. */
    __Diagnostic_Span_Role_Reference_Escape__,
    /* Represents the diagnostic span role local created value. */
    __Diagnostic_Span_Role_Local_Created__,
    /* Represents the diagnostic span role scope end value. */
    __Diagnostic_Span_Role_Scope_End__,
    /* Represents the diagnostic span role index value value. */
    __Diagnostic_Span_Role_Index_Value__,
    /* Represents the diagnostic span role expected here value. */
    __Diagnostic_Span_Role_Expected_Here__,
    /* Represents the diagnostic span role parser discovery value. */
    __Diagnostic_Span_Role_Parser_Discovery__,
    /* Represents the diagnostic span role unreachable pattern value. */
    __Diagnostic_Span_Role_Unreachable_Pattern__,
    /* Represents the diagnostic span role earlier coverage value. */
    __Diagnostic_Span_Role_Earlier_Coverage__
} __Diagnostic_Span_Role__;

/* Defines the diagnostic fix applicability values. */
typedef enum
{
    /* Represents the diagnostic fix machine applicable value. */
    __Diagnostic_Fix_Machine_Applicable__,
} __Diagnostic_Fix_Applicability__;

/* Defines the diagnostic max arguments macro. */
#define __Diagnostic_Max_Arguments__ 12U
/* Defines the diagnostic max related spans macro. */
#define __Diagnostic_Max_Related_Spans__ 8U
/* Defines the diagnostic max notes macro. */
#define __Diagnostic_Max_Notes__ 6U
/* Defines the diagnostic max help macro. */
#define __Diagnostic_Max_Help__ 6U
/* Defines the diagnostic max fixes macro. */
#define __Diagnostic_Max_Fixes__ 4U
/* Defines the diagnostic max trace macro. */
#define __Diagnostic_Max_Trace__ 8U
/* Defines the diagnostic max source name macro. */
#define __Diagnostic_Max_Source_Name__ 768U
/* Defines the diagnostic max source line macro. */
#define __Diagnostic_Max_Source_Line__ 2048U
/* Defines the diagnostic max argument value macro. */
#define __Diagnostic_Max_Argument_Value__ 256U
/* Defines the diagnostic max fix text macro. */
#define __Diagnostic_Max_Fix_Text__ 256U

/* Defines the diagnostic argument structure. */
typedef struct
{
    /* Stores the key. */
    __Diagnostic_Argument_Key__ __Key__;
    /* Stores the value. */
    char __Value__[__Diagnostic_Max_Argument_Value__];
} __Diagnostic_Argument__;

/* Defines the diagnostic source span structure. */
typedef struct
{
    /* Stores the source name. */
    char __Source_Name__[__Diagnostic_Max_Source_Name__];
    /* Stores the start. */
    __Source_Position__ __Start__;
    /* Stores the end. */
    __Source_Position__ __End__;
    /* Stores the line start offset. */
    size_t __Line_Start_Offset__;
    /* Stores the excerpt start offset. */
    size_t __Excerpt_Start_Offset__;
    /* Stores the line text. */
    char __Line_Text__[__Diagnostic_Max_Source_Line__];
    /* Stores the line text length. */
    size_t __Line_Text_Length__;
    /* Tracks whether the source is present. */
    int __Has_Source__;
} __Diagnostic_Source_Span__;

/* Defines the diagnostic related span structure. */
typedef struct
{
    /* Stores the span. */
    __Diagnostic_Source_Span__ __Span__;
    /* Stores the role. */
    __Diagnostic_Span_Role__ __Role__;
} __Diagnostic_Related_Span__;

/* Defines the diagnostic annotation structure. */
typedef struct
{
    /* Stores the message key. */
    __Diagnostic_Wording_Key__ __Message_Key__;
} __Diagnostic_Annotation__;

/* Defines the diagnostic fix structure. */
typedef struct
{
    /* Stores the span. */
    __Diagnostic_Source_Span__ __Span__;
    /* Stores the replacement. */
    char __Replacement__[__Diagnostic_Max_Fix_Text__];
    /* Stores the applicability. */
    __Diagnostic_Fix_Applicability__ __Applicability__;
    /* Stores the message key. */
    __Diagnostic_Wording_Key__ __Message_Key__;
} __Diagnostic_Fix__;

/* Defines the diagnostic trace entry structure. */
typedef struct
{
    /* Stores the message key. */
    __Diagnostic_Wording_Key__ __Message_Key__;
    /* Stores the span. */
    __Diagnostic_Source_Span__ __Span__;
    /* Tracks whether the span is present. */
    int __Has_Span__;
} __Diagnostic_Trace_Entry__;

/* Defines the diagnostic structure. */
typedef struct
{
    /* Stores the ID. */
    __Error_Id__ __Id__;
    /* Stores the severity. */
    __Diagnostic_Severity__ __Severity__;
    /* Stores the message key. */
    __Diagnostic_Wording_Key__ __Message_Key__;
    /* Stores the primary span. */
    __Diagnostic_Source_Span__ __Primary_Span__;
    /* Stores the primary role. */
    __Diagnostic_Span_Role__ __Primary_Role__;
    /* Stores the call arguments. */
    __Diagnostic_Argument__ __Arguments__[__Diagnostic_Max_Arguments__];
    /* Stores the argument count. */
    size_t __Argument_Count__;
    /* Stores the related spans. */
    __Diagnostic_Related_Span__ __Related_Spans__[__Diagnostic_Max_Related_Spans__];
    /* Stores the related span count. */
    size_t __Related_Span_Count__;
    /* Stores the notes. */
    __Diagnostic_Annotation__ __Notes__[__Diagnostic_Max_Notes__];
    /* Stores the note count. */
    size_t __Note_Count__;
    /* Stores the help. */
    __Diagnostic_Annotation__ __Help__[__Diagnostic_Max_Help__];
    /* Stores the help count. */
    size_t __Help_Count__;
    /* Stores the fixes. */
    __Diagnostic_Fix__ __Fixes__[__Diagnostic_Max_Fixes__];
    /* Stores the fix count. */
    size_t __Fix_Count__;
    /* Stores the trace. */
    __Diagnostic_Trace_Entry__ __Trace__[__Diagnostic_Max_Trace__];
    /* Stores the trace count. */
    size_t __Trace_Count__;
} __Diagnostic__;

/* Returns the diagnostic code. */
const char *__Diagnostic_Code__(__Error_Id__ __Id__);

/* Returns the diagnostic name. */
const char *__Diagnostic_Name__(__Error_Id__ __Id__);

/* Returns the diagnostic severity of. */
__Diagnostic_Severity__ __Diagnostic_Severity_Of__(__Error_Id__ __Id__);

/* Returns the diagnostic default message key. */
__Diagnostic_Wording_Key__ __Diagnostic_Default_Message_Key__(__Error_Id__ __Id__);

/* Resets the diagnostic. */
void __Diagnostic_Reset__(__Diagnostic__ *__Diagnostic_State__);

/* Begins the diagnostic. */
void __Diagnostic_Begin__(__Diagnostic__ *__Diagnostic_State__,
                          __Error_Id__ __Id__,
                          __Source_Span__ __Span__);

/* Begins the diagnostic without source. */
void __Diagnostic_Begin_Without_Source__(__Diagnostic__ *__Diagnostic_State__, __Error_Id__ __Id__);

/* Sets the diagnostic message key. */
void __Diagnostic_Set_Message_Key__(__Diagnostic__ *__Diagnostic_State__,
                                    __Diagnostic_Wording_Key__ __Message_Key__);

/* Sets the diagnostic primary role. */
void __Diagnostic_Set_Primary_Role__(__Diagnostic__ *__Diagnostic_State__,
                                     __Diagnostic_Span_Role__ __Role__);

/* Sets the diagnostic argument C string. */
int __Diagnostic_Set_Argument_Cstr__(__Diagnostic__ *__Diagnostic_State__,
                                     __Diagnostic_Argument_Key__ __Key__,
                                     const char *__Value__);

/* Sets the diagnostic argument text. */
int __Diagnostic_Set_Argument_Text__(__Diagnostic__ *__Diagnostic_State__,
                                     __Diagnostic_Argument_Key__ __Key__,
                                     __Text_Slice__ __Value__);

/* Sets the diagnostic argument u 64. */
int __Diagnostic_Set_Argument_U64__(__Diagnostic__ *__Diagnostic_State__,
                                    __Diagnostic_Argument_Key__ __Key__,
                                    uint64_t __Value__);

/* Adds the diagnostic related span. */
int __Diagnostic_Add_Related_Span__(__Diagnostic__ *__Diagnostic_State__,
                                    __Source_Span__ __Span__,
                                    __Diagnostic_Span_Role__ __Role__);

/* Adds the diagnostic note. */
int __Diagnostic_Add_Note__(__Diagnostic__ *__Diagnostic_State__,
                            __Diagnostic_Wording_Key__ __Message_Key__);

/* Adds the diagnostic help. */
int __Diagnostic_Add_Help__(__Diagnostic__ *__Diagnostic_State__,
                            __Diagnostic_Wording_Key__ __Message_Key__);

/* Adds the diagnostic fix. */
int __Diagnostic_Add_Fix__(__Diagnostic__ *__Diagnostic_State__,
                           __Source_Span__ __Span__,
                           const char *__Replacement__,
                           __Diagnostic_Fix_Applicability__ __Applicability__,
                           __Diagnostic_Wording_Key__ __Message_Key__);

/* Adds the diagnostic trace. */
int __Diagnostic_Add_Trace__(__Diagnostic__ *__Diagnostic_State__,
                             __Diagnostic_Wording_Key__ __Message_Key__,
                             const __Source_Span__ *__Optional_Span__);

/* Copies the diagnostic. */
void __Diagnostic_Copy__(__Diagnostic__ *__Destination__, const __Diagnostic__ *__Source__);

#endif
