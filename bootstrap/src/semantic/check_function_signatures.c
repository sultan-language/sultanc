#include "semantic/check_function_signatures.h"
#include "semantic/check_type_reference.h"
#include "kernel/layout/layout.h"

int __Semantic_Check_Function_Signatures__(__Semantic_Context__ *__Context__)
{
    size_t __Index__ = 0U;

    for (__Index__ = 0U; __Index__ < __Context__->__Functions__.__Count__; ++__Index__)
    {
        __Semantic_Function_Entry__ *__Function__ =
            (__Semantic_Function_Entry__ *)__Vector_At__(&__Context__->__Functions__, __Index__);
        size_t __Parameter_Index__ = 0U;

        if (__Function__ == NULL)
        {
            continue;
        }
        __Context__->__Active_Unit__ = __Function__->__Unit__;
        for (__Parameter_Index__ = 0U;
             __Parameter_Index__ < __Function__->__Function__->__Parameter_Count__;
             ++__Parameter_Index__)
        {
            if (!__Semantic_Check_Type_Reference__(
                    __Context__,
                    __Function__->__Function__->__Parameters__[__Parameter_Index__]
                        .__Slot__.__Type__))
            {
                return 0;
            }
        }
        if (!__Semantic_Check_Type_Reference__(__Context__,
                                               __Function__->__Function__->__Output__.__Type__))
        {
            return 0;
        }
    }
    __Context__->__Active_Unit__ = NULL;
    return 1;
}
