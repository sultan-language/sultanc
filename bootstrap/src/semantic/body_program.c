#include "semantic/diagnostic.h"
#include "semantic/body_internal.h"
#include "kernel/memory/memory.h"

#include <string.h>

static int __Check_Function__(__Semantic_Context__ *__Semantic__,
                              __Semantic_Function_Entry__ *__Function__)
{
    __Semantic_Body_Context__ __Context__;
    size_t __Index__ = 0U;
    __Resolved_Type__ __Output__;
    memset(&__Context__, 0, sizeof(__Context__));
    __Context__.__Semantic__ = __Semantic__;
    __Context__.__Function__ = __Function__;
    __Semantic__->__Active_Unit__ = __Function__->__Unit__;
    __Vector_Init__(&__Context__.__Locals__, sizeof(__Semantic_Local__));
    __Arena_Init__(&__Context__.__Synthetic_Types__, 2048U);
    for (__Index__ = 0U; __Index__ < __Function__->__Function__->__Parameter_Count__; ++__Index__)
    {
        __Ast_Function_Parameter__ *__Parameter__ =
            &__Function__->__Function__->__Parameters__[__Index__];
        __Semantic_Local__ __Local__;
        memset(&__Local__, 0, sizeof(__Local__));
        __Local__.__Name_Kind__ = __Ast_Lvalue_Base_Identifier__;
        __Local__.__Name__ = __Parameter__->__Name__;
        __Local__.__Type__ = __Parameter__->__Slot__.__Type__;
        __Local__.__Slot__ = &__Parameter__->__Slot__;
        __Local__.__Mutable__ =
            (__Local__.__Type__ != NULL && __Local__.__Type__->__Kind__ == __Ast_Type_Mutable__);
        __Local__.__Is_Parameter__ = 1;
        __Safety_Fact_Init__(&__Local__.__Safety__, 1);
        __Local__.__Declaration_Span__ = __Function__->__Function__->__Header__.__Span__;
        __Safety_Fact_Mark_Initialized_At__(&__Local__.__Safety__, __Local__.__Declaration_Span__);
        {
            int __Is_View__ = __Safety_Type_Is_Reference__(__Local__.__Type__, NULL);

            if (!__Is_View__ && __Memory_Type_Contains_View__(__Semantic__, __Local__.__Type__))
            {
                __Is_View__ = 1;
            }
            if (__Is_View__)
            {
                __Local__.__Safety__.__Lifetime_Region__ = 0U;
                __Local__.__Safety__.__Origin_Parameter__ = __Index__;
            }
        }
        __Local__.__Scope_Depth__ = 0U;
        if (__Body_Name_Exists_In_Current_Scope__(&__Context__, &__Local__))
        {
            __Arena_Destroy__(&__Context__.__Synthetic_Types__);
            __Vector_Destroy__(&__Context__.__Locals__);
            return __Semantic_Fail__(__Semantic__,
                                     __E0205_Duplicate_Declaration__,
                                     __Function__->__Function__->__Header__.__Span__);
        }
        if (__Vector_Push__(&__Context__.__Locals__, &__Local__) == NULL)
        {
            __Arena_Destroy__(&__Context__.__Synthetic_Types__);
            __Vector_Destroy__(&__Context__.__Locals__);
            return __Semantic_Fail__(__Semantic__,
                                     __E1100_Internal_Context_Error__,
                                     __Function__->__Function__->__Header__.__Span__);
        }
    }
    __Context__.__Falls_Through__ = 1;
    if (!__Body_Check_Block__(&__Context__, __Function__->__Function__->__Body__))
    {
        __Arena_Destroy__(&__Context__.__Synthetic_Types__);
        __Vector_Destroy__(&__Context__.__Locals__);
        return 0;
    }
    if (!__Type_Resolve__(
            __Semantic__, __Function__->__Function__->__Output__.__Type__, &__Output__))
    {
        __Arena_Destroy__(&__Context__.__Synthetic_Types__);
        __Vector_Destroy__(&__Context__.__Locals__);
        return 0;
    }
    if (__Context__.__Falls_Through__ && __Output__.__Kind__ != __Resolved_Type_Void__)
    {
        __Arena_Destroy__(&__Context__.__Synthetic_Types__);
        __Vector_Destroy__(&__Context__.__Locals__);
        return __Semantic_Fail__(__Semantic__,
                                 __E1308_Missing_Return_Path__,
                                 __Function__->__Function__->__Header__.__Span__);
    }
    __Arena_Destroy__(&__Context__.__Synthetic_Types__);
    __Vector_Destroy__(&__Context__.__Locals__);
    return 1;
}

int __Semantic_Check_Bodies__(__Semantic_Context__ *__Context__)
{
    size_t __Index__ = 0U;
    if (__Context__ == NULL)
    {
        return 0;
    }
    for (__Index__ = 0U; __Index__ < __Context__->__Functions__.__Count__; ++__Index__)
    {
        __Semantic_Function_Entry__ *__Function__ =
            (__Semantic_Function_Entry__ *)__Vector_At__(&__Context__->__Functions__, __Index__);
        if (__Function__ != NULL && !__Check_Function__(__Context__, __Function__))
        {
            return 0;
        }
    }
    return 1;
}
