#include "semantic/diagnostic.h"
#include "semantic/check_function_signatures.h"
#include "kernel/type/type.h"

int __Semantic_Check_Entry_Point__(__Semantic_Context__ *__Context__)
{
    __Resolved_Type__ __Main_Output__;

    if (__Context__->__Main__ == NULL)
    {
        return 0;
    }
    if (__Context__->__Main__->__Function__->__Parameter_Count__ != 0U)
    {
        return __Semantic_Fail__(__Context__,
                                 __E0408_Main_Bad_Signature__,
                                 __Context__->__Main__->__Function__->__Header__.__Span__);
    }
    if (!__Type_Resolve__(__Context__,
                          __Context__->__Main__->__Function__->__Output__.__Type__,
                          &__Main_Output__))
    {
        return 0;
    }
    if (__Main_Output__.__Kind__ != __Resolved_Type_Signed_Integer__ &&
        __Main_Output__.__Kind__ != __Resolved_Type_Unsigned_Integer__)
    {
        return __Semantic_Fail__(__Context__,
                                 __E0408_Main_Bad_Signature__,
                                 __Context__->__Main__->__Function__->__Header__.__Span__);
    }
    return 1;
}
