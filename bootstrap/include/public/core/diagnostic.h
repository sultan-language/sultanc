#ifndef SULTANC__CORE_DIAGNOSTIC_H__
#define SULTANC__CORE_DIAGNOSTIC_H__

#include "source.h"

#include <stddef.h>
#include <stdint.h>

typedef enum
{
    __Diagnostic_Severity_Error__,
} __Diagnostic_Severity__;

typedef enum
{
#define SULTANC_DIAGNOSTIC_WORDING_KEY(__Key__, __Placeholders__) __Key__,
#include "diagnostic_wording_keys.def"
#undef SULTANC_DIAGNOSTIC_WORDING_KEY
    __Diagnostic_Wording_Key_Count__
} __Diagnostic_Wording_Key__;

typedef enum
{
#define SULTANC_DIAGNOSTIC(__Id__, __Code__, __Severity__, __Message_Key__) __Id__,
#include "diagnostic_catalog.def"
#undef SULTANC_DIAGNOSTIC
    __Error_Id_Count__
} __Error_Id__;

typedef enum
{
    __Diagnostic_Argument_None__,
    __Diagnostic_Argument_Value__,
    __Diagnostic_Argument_Expected__,
    __Diagnostic_Argument_Found__,
    __Diagnostic_Argument_Index__,
    __Diagnostic_Argument_Length__,
    __Diagnostic_Argument_Range__,
    __Diagnostic_Argument_Pattern__,
    __Diagnostic_Argument_Path__,
    __Diagnostic_Argument_Function__,
} __Diagnostic_Argument_Key__;

typedef enum
{
    __Diagnostic_Span_Role_None__,
    __Diagnostic_Span_Role_Primary__,
    __Diagnostic_Span_Role_Invalid_Use__,
    __Diagnostic_Span_Role_Move_Origin__,
    __Diagnostic_Span_Role_Original_Owner__,
    __Diagnostic_Span_Role_Uninitialized_Declaration__,
    __Diagnostic_Span_Role_Conflicting_Borrow__,
    __Diagnostic_Span_Role_Mutable_Borrow_Start__,
    __Diagnostic_Span_Role_Reference_Escape__,
    __Diagnostic_Span_Role_Local_Created__,
    __Diagnostic_Span_Role_Scope_End__,
    __Diagnostic_Span_Role_Index_Value__,
    __Diagnostic_Span_Role_Expected_Here__,
    __Diagnostic_Span_Role_Parser_Discovery__,
    __Diagnostic_Span_Role_Unreachable_Pattern__,
    __Diagnostic_Span_Role_Earlier_Coverage__
} __Diagnostic_Span_Role__;

typedef enum
{
    __Diagnostic_Fix_Machine_Applicable__,
} __Diagnostic_Fix_Applicability__;

#define __Diagnostic_Max_Arguments__ 12U
#define __Diagnostic_Max_Related_Spans__ 8U
#define __Diagnostic_Max_Notes__ 6U
#define __Diagnostic_Max_Help__ 6U
#define __Diagnostic_Max_Fixes__ 4U
#define __Diagnostic_Max_Trace__ 8U
#define __Diagnostic_Max_Source_Name__ 768U
#define __Diagnostic_Max_Source_Line__ 2048U
#define __Diagnostic_Max_Argument_Value__ 256U
#define __Diagnostic_Max_Fix_Text__ 256U

typedef struct
{
    __Diagnostic_Argument_Key__ __Key__;
    char __Value__[__Diagnostic_Max_Argument_Value__];
} __Diagnostic_Argument__;

typedef struct
{
    char __Source_Name__[__Diagnostic_Max_Source_Name__];
    __Source_Position__ __Start__;
    __Source_Position__ __End__;
    size_t __Line_Start_Offset__;
    size_t __Excerpt_Start_Offset__;
    char __Line_Text__[__Diagnostic_Max_Source_Line__];
    size_t __Line_Text_Length__;
    int __Has_Source__;
} __Diagnostic_Source_Span__;

typedef struct
{
    __Diagnostic_Source_Span__ __Span__;
    __Diagnostic_Span_Role__ __Role__;
} __Diagnostic_Related_Span__;

typedef struct
{
    __Diagnostic_Wording_Key__ __Message_Key__;
} __Diagnostic_Annotation__;

typedef struct
{
    __Diagnostic_Source_Span__ __Span__;
    char __Replacement__[__Diagnostic_Max_Fix_Text__];
    __Diagnostic_Fix_Applicability__ __Applicability__;
    __Diagnostic_Wording_Key__ __Message_Key__;
} __Diagnostic_Fix__;

typedef struct
{
    __Diagnostic_Wording_Key__ __Message_Key__;
    __Diagnostic_Source_Span__ __Span__;
    int __Has_Span__;
} __Diagnostic_Trace_Entry__;

typedef struct
{
    __Error_Id__ __Id__;
    __Diagnostic_Severity__ __Severity__;
    __Diagnostic_Wording_Key__ __Message_Key__;
    __Diagnostic_Source_Span__ __Primary_Span__;
    __Diagnostic_Span_Role__ __Primary_Role__;
    __Diagnostic_Argument__ __Arguments__[__Diagnostic_Max_Arguments__];
    size_t __Argument_Count__;
    __Diagnostic_Related_Span__ __Related_Spans__[__Diagnostic_Max_Related_Spans__];
    size_t __Related_Span_Count__;
    __Diagnostic_Annotation__ __Notes__[__Diagnostic_Max_Notes__];
    size_t __Note_Count__;
    __Diagnostic_Annotation__ __Help__[__Diagnostic_Max_Help__];
    size_t __Help_Count__;
    __Diagnostic_Fix__ __Fixes__[__Diagnostic_Max_Fixes__];
    size_t __Fix_Count__;
    __Diagnostic_Trace_Entry__ __Trace__[__Diagnostic_Max_Trace__];
    size_t __Trace_Count__;
} __Diagnostic__;

const char *__Diagnostic_Code__(__Error_Id__ __Id__);

const char *__Diagnostic_Name__(__Error_Id__ __Id__);

__Diagnostic_Severity__ __Diagnostic_Severity_Of__(__Error_Id__ __Id__);

__Diagnostic_Wording_Key__ __Diagnostic_Default_Message_Key__(__Error_Id__ __Id__);

void __Diagnostic_Reset__(__Diagnostic__ *__Diagnostic_State__);

void __Diagnostic_Begin__(__Diagnostic__ *__Diagnostic_State__,
                          __Error_Id__ __Id__,
                          __Source_Span__ __Span__);

void __Diagnostic_Begin_Without_Source__(__Diagnostic__ *__Diagnostic_State__, __Error_Id__ __Id__);

void __Diagnostic_Set_Message_Key__(__Diagnostic__ *__Diagnostic_State__,
                                    __Diagnostic_Wording_Key__ __Message_Key__);

void __Diagnostic_Set_Primary_Role__(__Diagnostic__ *__Diagnostic_State__,
                                     __Diagnostic_Span_Role__ __Role__);

int __Diagnostic_Set_Argument_Cstr__(__Diagnostic__ *__Diagnostic_State__,
                                     __Diagnostic_Argument_Key__ __Key__,
                                     const char *__Value__);

int __Diagnostic_Set_Argument_Text__(__Diagnostic__ *__Diagnostic_State__,
                                     __Diagnostic_Argument_Key__ __Key__,
                                     __Text_Slice__ __Value__);

int __Diagnostic_Set_Argument_U64__(__Diagnostic__ *__Diagnostic_State__,
                                    __Diagnostic_Argument_Key__ __Key__,
                                    uint64_t __Value__);

int __Diagnostic_Add_Related_Span__(__Diagnostic__ *__Diagnostic_State__,
                                    __Source_Span__ __Span__,
                                    __Diagnostic_Span_Role__ __Role__);

int __Diagnostic_Add_Note__(__Diagnostic__ *__Diagnostic_State__,
                            __Diagnostic_Wording_Key__ __Message_Key__);

int __Diagnostic_Add_Help__(__Diagnostic__ *__Diagnostic_State__,
                            __Diagnostic_Wording_Key__ __Message_Key__);

int __Diagnostic_Add_Fix__(__Diagnostic__ *__Diagnostic_State__,
                           __Source_Span__ __Span__,
                           const char *__Replacement__,
                           __Diagnostic_Fix_Applicability__ __Applicability__,
                           __Diagnostic_Wording_Key__ __Message_Key__);

int __Diagnostic_Add_Trace__(__Diagnostic__ *__Diagnostic_State__,
                             __Diagnostic_Wording_Key__ __Message_Key__,
                             const __Source_Span__ *__Optional_Span__);

void __Diagnostic_Copy__(__Diagnostic__ *__Destination__, const __Diagnostic__ *__Source__);

#endif
