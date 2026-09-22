#include "frontend/parser/diagnostic.h"
#include "frontend/parser/internal.h"

static int __Parser_Fail_With_Id__(__Parser__ *__Parser_State__,
                                   __Error_Id__ __Id__,
                                   __Diagnostic_Wording_Key__ __Message_Key__)
{
    if (__Parser_State__ == NULL)
    {
        return 0;
    }

    __Diagnostic_Begin__(
        __Parser_State__->__Diagnostic__, __Id__, __Parser_State__->__Current__.__Span__);
    __Diagnostic_Set_Message_Key__(__Parser_State__->__Diagnostic__, __Message_Key__);
    __Parser_State__->__Failed__ = 1;
    return 0;
}

int __Parser_Fail__(__Parser__ *__Parser_State__, __Diagnostic_Wording_Key__ __Message_Key__)
{
    return __Parser_Fail_With_Id__(__Parser_State__, __E0001_Syntax_Error__, __Message_Key__);
}

int __Parser_Fail_Internal__(__Parser__ *__Parser_State__,
                             __Diagnostic_Wording_Key__ __Message_Key__)
{
    return __Parser_Fail_With_Id__(
        __Parser_State__, __E1100_Internal_Context_Error__, __Message_Key__);
}
