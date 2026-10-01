/* Defines move, borrow, and initialization facts. */

#ifndef SULTANC__KERNEL_SAFETY_FLOW_H__
#define SULTANC__KERNEL_SAFETY_FLOW_H__

#include "core/source.h"
#include "frontend/ast/type.h"
#include "semantic/context.h"

#include <stdint.h>

/* Defines the safety value state values. */
typedef enum
{
    /* Represents the safety value uninitialized value. */
    __Safety_Value_Uninitialized__,
    /* Represents the safety value initialized value. */
    __Safety_Value_Initialized__,
    /* Represents the safety value moved value. */
    __Safety_Value_Moved__,
    /* Represents the safety value maybe uninitialized value. */
    __Safety_Value_Maybe_Uninitialized__,
    /* Represents the safety value maybe moved value. */
    __Safety_Value_Maybe_Moved__
} __Safety_Value_State__;

/* Defines the safety borrow kind values. */
typedef enum
{
    /* Represents the safety borrow none value. */
    __Safety_Borrow_None__,
    /* Represents the safety borrow shared value. */
    __Safety_Borrow_Shared__,
    /* Represents the safety borrow mutable value. */
    __Safety_Borrow_Mutable__
} __Safety_Borrow_Kind__;

/* Defines the safety value fact structure. */
typedef struct
{
    /* Stores the state. */
    __Safety_Value_State__ __State__;
    /* Stores the immutable borrows. */
    size_t __Immutable_Borrows__;
    /* Tracks the mutable borrow state. */
    int __Mutable_Borrow__;

    /* Reference provenance; SIZE_MAX marks an untracked external reference. */
    size_t __Borrowed_From__;
    /* Stores the borrow kind. */
    __Safety_Borrow_Kind__ __Borrow_Kind__;
    /* Stores the lifetime region. */
    size_t __Lifetime_Region__;
    /* Stores the origin parameter. */
    size_t __Origin_Parameter__;

    /* Causal provenance retained by SSF for structured diagnostics. */
    __Source_Span__ __Origin_Span__;
    /* Tracks whether the origin span is present. */
    int __Has_Origin_Span__;
    /* Stores the move span. */
    __Source_Span__ __Move_Span__;
    /* Tracks whether the move span is present. */
    int __Has_Move_Span__;
    /* Tracks the active borrow span state. */
    __Source_Span__ __Active_Borrow_Span__;
    /* Tracks whether the active borrow span is present. */
    int __Has_Active_Borrow_Span__;
    /* Tracks the mutable borrow holder state. */
    size_t __Mutable_Borrow_Holder__;
} __Safety_Value_Fact__;

/* Defines the safety control flow structure. */
typedef struct
{
    /* Tracks the falls through state. */
    int __Falls_Through__;
} __Safety_Control_Flow__;

/* Initializes the safety fact. */
void __Safety_Fact_Init__(__Safety_Value_Fact__ *__Fact__, int __Initialized__);

/* Checks whether the safety fact can read. */
int __Safety_Fact_Can_Read__(const __Safety_Value_Fact__ *__Fact__);

/* Checks whether the safety fact can write. */
int __Safety_Fact_Can_Write__(const __Safety_Value_Fact__ *__Fact__);

/* Checks whether the safety fact can move. */
int __Safety_Fact_Can_Move__(const __Safety_Value_Fact__ *__Fact__);

/* Returns the safety fact acquire shared at. */
int __Safety_Fact_Acquire_Shared_At__(__Safety_Value_Fact__ *__Owner__, __Source_Span__ __Span__);

/* Returns the safety fact acquire mutable at. */
int __Safety_Fact_Acquire_Mutable_At__(__Safety_Value_Fact__ *__Owner__,
                                       __Source_Span__ __Span__,
                                       size_t __Holder_Index__);

/* Sets the safety fact mutable borrow holder. */
void __Safety_Fact_Set_Mutable_Borrow_Holder__(__Safety_Value_Fact__ *__Owner__,
                                               size_t __Holder_Index__);

/* Releases the safety fact borrow. */
void __Safety_Fact_Release_Borrow__(__Safety_Value_Fact__ *__Owner__,
                                    __Safety_Borrow_Kind__ __Kind__);

/* Marks the safety fact initialized at. */
void __Safety_Fact_Mark_Initialized_At__(__Safety_Value_Fact__ *__Fact__, __Source_Span__ __Span__);

/* Marks the safety fact moved at. */
void __Safety_Fact_Mark_Moved_At__(__Safety_Value_Fact__ *__Fact__, __Source_Span__ __Span__);

/* Merges the safety fact. */
__Safety_Value_Fact__ __Safety_Fact_Merge__(const __Safety_Value_Fact__ *__Left__,
                                            const __Safety_Value_Fact__ *__Right__);

/* Checks whether the safety type is move only. */
int __Safety_Type_Is_Move_Only__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__);

/* Checks whether the safety type is reference. */
int __Safety_Type_Is_Reference__(__Ast_Type__ *__Type__, int *__Out_Mutable__);

#endif
