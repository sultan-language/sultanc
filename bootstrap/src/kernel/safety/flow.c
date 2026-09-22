#include "kernel/safety/flow.h"
#include "kernel/memory/memory.h"

void __Safety_Fact_Init__(__Safety_Value_Fact__ *__Fact__, int __Initialized__)
{
    if (__Fact__ == NULL)
    {
        return;
    }

    __Fact__->__State__ =
        __Initialized__ ? __Safety_Value_Initialized__ : __Safety_Value_Uninitialized__;
    __Fact__->__Immutable_Borrows__ = 0U;
    __Fact__->__Mutable_Borrow__ = 0;
    __Fact__->__Borrowed_From__ = SIZE_MAX;
    __Fact__->__Borrow_Kind__ = __Safety_Borrow_None__;
    __Fact__->__Lifetime_Region__ = SIZE_MAX;
    __Fact__->__Origin_Parameter__ = SIZE_MAX;
    __Fact__->__Has_Origin_Span__ = 0;
    __Fact__->__Has_Move_Span__ = 0;
    __Fact__->__Has_Active_Borrow_Span__ = 0;
    __Fact__->__Mutable_Borrow_Holder__ = SIZE_MAX;
}

int __Safety_Fact_Can_Read__(const __Safety_Value_Fact__ *__Fact__)
{
    return __Fact__ != NULL && __Fact__->__State__ == __Safety_Value_Initialized__ &&
           !__Fact__->__Mutable_Borrow__;
}

int __Safety_Fact_Can_Write__(const __Safety_Value_Fact__ *__Fact__)
{
    return __Fact__ != NULL && !__Fact__->__Mutable_Borrow__ &&
           __Fact__->__Immutable_Borrows__ == 0U;
}

int __Safety_Fact_Can_Move__(const __Safety_Value_Fact__ *__Fact__)
{
    return __Fact__ != NULL && __Fact__->__State__ == __Safety_Value_Initialized__ &&
           !__Fact__->__Mutable_Borrow__ && __Fact__->__Immutable_Borrows__ == 0U;
}

int __Safety_Fact_Acquire_Shared_At__(__Safety_Value_Fact__ *__Owner__, __Source_Span__ __Span__)
{
    if (__Owner__ == NULL || __Owner__->__State__ != __Safety_Value_Initialized__ ||
        __Owner__->__Mutable_Borrow__)
    {
        return 0;
    }
    ++__Owner__->__Immutable_Borrows__;
    if (!__Owner__->__Has_Active_Borrow_Span__)
    {
        __Owner__->__Active_Borrow_Span__ = __Span__;
        __Owner__->__Has_Active_Borrow_Span__ = 1;
    }
    return 1;
}

int __Safety_Fact_Acquire_Mutable_At__(__Safety_Value_Fact__ *__Owner__,
                                       __Source_Span__ __Span__,
                                       size_t __Holder_Index__)
{
    if (__Owner__ == NULL || __Owner__->__State__ != __Safety_Value_Initialized__ ||
        __Owner__->__Mutable_Borrow__ || __Owner__->__Immutable_Borrows__ != 0U)
    {
        return 0;
    }
    __Owner__->__Mutable_Borrow__ = 1;
    __Owner__->__Active_Borrow_Span__ = __Span__;
    __Owner__->__Has_Active_Borrow_Span__ = 1;
    __Owner__->__Mutable_Borrow_Holder__ = __Holder_Index__;
    return 1;
}

void __Safety_Fact_Set_Mutable_Borrow_Holder__(__Safety_Value_Fact__ *__Owner__,
                                               size_t __Holder_Index__)
{
    if (__Owner__ != NULL && __Owner__->__Mutable_Borrow__)
    {
        __Owner__->__Mutable_Borrow_Holder__ = __Holder_Index__;
    }
}

void __Safety_Fact_Release_Borrow__(__Safety_Value_Fact__ *__Owner__,
                                    __Safety_Borrow_Kind__ __Kind__)
{
    if (__Owner__ == NULL)
    {
        return;
    }
    if (__Kind__ == __Safety_Borrow_Shared__)
    {
        if (__Owner__->__Immutable_Borrows__ != 0U)
        {
            --__Owner__->__Immutable_Borrows__;
        }
    }
    else if (__Kind__ == __Safety_Borrow_Mutable__)
    {
        __Owner__->__Mutable_Borrow__ = 0;
        __Owner__->__Mutable_Borrow_Holder__ = SIZE_MAX;
    }

    if (__Owner__->__Immutable_Borrows__ == 0U && !__Owner__->__Mutable_Borrow__)
    {
        __Owner__->__Has_Active_Borrow_Span__ = 0;
    }
}

void __Safety_Fact_Mark_Initialized_At__(__Safety_Value_Fact__ *__Fact__, __Source_Span__ __Span__)
{
    if (__Fact__ != NULL)
    {
        __Fact__->__State__ = __Safety_Value_Initialized__;
        if (!__Fact__->__Has_Origin_Span__)
        {
            __Fact__->__Origin_Span__ = __Span__;
            __Fact__->__Has_Origin_Span__ = 1;
        }
    }
}

void __Safety_Fact_Mark_Moved_At__(__Safety_Value_Fact__ *__Fact__, __Source_Span__ __Span__)
{
    if (__Fact__ != NULL)
    {
        __Fact__->__State__ = __Safety_Value_Moved__;
        __Fact__->__Immutable_Borrows__ = 0U;
        __Fact__->__Mutable_Borrow__ = 0;
        __Fact__->__Move_Span__ = __Span__;
        __Fact__->__Has_Move_Span__ = 1;
        __Fact__->__Has_Active_Borrow_Span__ = 0;
        __Fact__->__Mutable_Borrow_Holder__ = SIZE_MAX;
    }
}

__Safety_Value_Fact__ __Safety_Fact_Merge__(const __Safety_Value_Fact__ *__Left__,
                                            const __Safety_Value_Fact__ *__Right__)
{
    __Safety_Value_Fact__ __Result__;

    __Safety_Fact_Init__(&__Result__, 0);
    if (__Left__ == NULL || __Right__ == NULL)
    {
        return __Result__;
    }

    if (__Left__->__State__ == __Right__->__State__)
    {
        __Result__.__State__ = __Left__->__State__;
    }
    else if (__Left__->__State__ == __Safety_Value_Moved__ ||
             __Right__->__State__ == __Safety_Value_Moved__ ||
             __Left__->__State__ == __Safety_Value_Maybe_Moved__ ||
             __Right__->__State__ == __Safety_Value_Maybe_Moved__)
    {
        __Result__.__State__ = __Safety_Value_Maybe_Moved__;
    }
    else
    {
        __Result__.__State__ = __Safety_Value_Maybe_Uninitialized__;
    }

    __Result__.__Immutable_Borrows__ =
        __Left__->__Immutable_Borrows__ > __Right__->__Immutable_Borrows__
            ? __Left__->__Immutable_Borrows__
            : __Right__->__Immutable_Borrows__;
    __Result__.__Mutable_Borrow__ = __Left__->__Mutable_Borrow__ || __Right__->__Mutable_Borrow__;
    if (__Left__->__Mutable_Borrow_Holder__ == __Right__->__Mutable_Borrow_Holder__)
    {
        __Result__.__Mutable_Borrow_Holder__ = __Left__->__Mutable_Borrow_Holder__;
    }

    if (__Left__->__Has_Origin_Span__)
    {
        __Result__.__Origin_Span__ = __Left__->__Origin_Span__;
        __Result__.__Has_Origin_Span__ = 1;
    }
    else if (__Right__->__Has_Origin_Span__)
    {
        __Result__.__Origin_Span__ = __Right__->__Origin_Span__;
        __Result__.__Has_Origin_Span__ = 1;
    }
    if (__Left__->__Has_Move_Span__)
    {
        __Result__.__Move_Span__ = __Left__->__Move_Span__;
        __Result__.__Has_Move_Span__ = 1;
    }
    else if (__Right__->__Has_Move_Span__)
    {
        __Result__.__Move_Span__ = __Right__->__Move_Span__;
        __Result__.__Has_Move_Span__ = 1;
    }
    if (__Left__->__Has_Active_Borrow_Span__)
    {
        __Result__.__Active_Borrow_Span__ = __Left__->__Active_Borrow_Span__;
        __Result__.__Has_Active_Borrow_Span__ = 1;
    }
    else if (__Right__->__Has_Active_Borrow_Span__)
    {
        __Result__.__Active_Borrow_Span__ = __Right__->__Active_Borrow_Span__;
        __Result__.__Has_Active_Borrow_Span__ = 1;
    }

    if (__Left__->__Borrowed_From__ == __Right__->__Borrowed_From__ &&
        __Left__->__Borrow_Kind__ == __Right__->__Borrow_Kind__)
    {
        __Result__.__Borrowed_From__ = __Left__->__Borrowed_From__;
        __Result__.__Borrow_Kind__ = __Left__->__Borrow_Kind__;
        __Result__.__Lifetime_Region__ =
            __Left__->__Lifetime_Region__ > __Right__->__Lifetime_Region__
                ? __Left__->__Lifetime_Region__
                : __Right__->__Lifetime_Region__;
        if (__Left__->__Origin_Parameter__ == __Right__->__Origin_Parameter__)
        {
            __Result__.__Origin_Parameter__ = __Left__->__Origin_Parameter__;
        }
    }
    return __Result__;
}

int __Safety_Type_Is_Reference__(__Ast_Type__ *__Type__, int *__Out_Mutable__)
{
    while (__Type__ != NULL && __Type__->__Kind__ == __Ast_Type_Mutable__)
    {
        __Type__ = __Type__->__As__.__Inner__;
    }
    if (__Type__ == NULL || __Type__->__Kind__ != __Ast_Type_Reference__)
    {
        return 0;
    }
    if (__Out_Mutable__ != NULL)
    {
        *__Out_Mutable__ = __Type__->__As__.__Inner__ != NULL &&
                           __Type__->__As__.__Inner__->__Kind__ == __Ast_Type_Mutable__;
    }
    return 1;
}

int __Safety_Type_Is_Move_Only__(__Semantic_Context__ *__Context__, __Ast_Type__ *__Type__)
{
    int __Mutable_Reference__ = 0;
    return __Memory_Type_Is_Owned__(__Context__, __Type__) ||
           (__Safety_Type_Is_Reference__(__Type__, &__Mutable_Reference__) &&
            __Mutable_Reference__) ||
           (!__Memory_Type_Is_View__(__Type__) &&
            __Memory_Type_Contains_View__(__Context__, __Type__));
}
