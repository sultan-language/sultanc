#ifndef SULTANC__KERNEL_SAFETY_FLOW_H__
#define SULTANC__KERNEL_SAFETY_FLOW_H__

#include "core/source.h"
#include "frontend/ast/type.h"
#include "semantic/context.h"

#include <stdint.h>

typedef enum
{
    __Safety_Value_Uninitialized__,
    __Safety_Value_Initialized__,
    __Safety_Value_Moved__,
    __Safety_Value_Maybe_Uninitialized__,
    __Safety_Value_Maybe_Moved__
} __Safety_Value_State__;

typedef enum
{
    __Safety_Borrow_None__,
    __Safety_Borrow_Shared__,
    __Safety_Borrow_Mutable__
} __Safety_Borrow_Kind__;

typedef struct
{
    __Safety_Value_State__ __State__;
    size_t __Immutable_Borrows__;
    int __Mutable_Borrow__;

    /*
     * Reference provenance. SIZE_MAX means the value is not a locally-tracked
     * reference (for example a function parameter supplied by the caller).
     */
    size_t __Borrowed_From__;
    __Safety_Borrow_Kind__ __Borrow_Kind__;
    size_t __Lifetime_Region__;
    size_t __Origin_Parameter__;

    /* Causal provenance retained by SSF for structured diagnostics. */
    __Source_Span__ __Origin_Span__;
    int __Has_Origin_Span__;
    __Source_Span__ __Move_Span__;
    int __Has_Move_Span__;
    __Source_Span__ __Active_Borrow_Span__;
    int __Has_Active_Borrow_Span__;
    size_t __Mutable_Borrow_Holder__;
} __Safety_Value_Fact__;

typedef struct
{
    int __Falls_Through__;
} __Safety_Control_Flow__;

void __Safety_Fact_Init__(__Safety_Value_Fact__ *__Fact__, int __Initialized__);

int __Safety_Fact_Can_Read__(const __Safety_Value_Fact__ *__Fact__);

int __Safety_Fact_Can_Write__(const __Safety_Value_Fact__ *__Fact__);

int __Safety_Fact_Can_Move__(const __Safety_Value_Fact__ *__Fact__);

int __Safety_Fact_Acquire_Shared_At__(__Safety_Value_Fact__ *__Owner__, __Source_Span__ __Span__);

int __Safety_Fact_Acquire_Mutable_At__(__Safety_Value_Fact__ *__Owner__,
                                       __Source_Span__ __Span__,
                                       size_t __Holder_Index__);

void __Safety_Fact_Set_Mutable_Borrow_Holder__(__Safety_Value_Fact__ *__Owner__,
                                               size_t __Holder_Index__);

void __Safety_Fact_Release_Borrow__(__Safety_Value_Fact__ *__Owner__,
                                    __Safety_Borrow_Kind__ __Kind__);

void __Safety_Fact_Mark_Initialized_At__(__Safety_Value_Fact__ *__Fact__, __Source_Span__ __Span__);

void __Safety_Fact_Mark_Moved_At__(__Safety_Value_Fact__ *__Fact__, __Source_Span__ __Span__);

__Safety_Value_Fact__ __Safety_Fact_Merge__(const __Safety_Value_Fact__ *__Left__,
                                            const __Safety_Value_Fact__ *__Right__);

int __Safety_Type_Is_Move_Only__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__);

int __Safety_Type_Is_Reference__(__Ast_Type__ *__Type__, int *__Out_Mutable__);

#endif
