/* Resolves built-in function identities. */

#include "kernel/name/name.h"
#include "frontend/identifier_identity.h"

/* Expands the builtin name macro. */
#define SULTANC__BUILTIN_NAME__(literal_) (__Text_Slice__){(literal_), sizeof(literal_) - 1U}

/* Finds the name builtin function. */
__Name_Builtin_Function__ __Name_Find_Builtin_Function__(__Text_Slice__ __Name__)
{
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("length")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("الطول")))
    {
        return __Name_Builtin_Length__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("append")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("ألحق")))
    {
        return __Name_Builtin_Append__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("swap")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدّل")))
    {
        return __Name_Builtin_Swap__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_معمارية_المضيف")))
    {
        return __Name_Builtin_Host_Architecture__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_منصة_المضيف")))
    {
        return __Name_Builtin_Host_Platform__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_بيئة_المضيف")))
    {
        return __Name_Builtin_Host_Environment__;
    }
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("حرر")) ||
        __Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("release")))
        return __Name_Builtin_Release__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_نداء_نواة_خام")))
        return __Name_Builtin_Raw_Kernel_Call__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_بتات_ع64")))
        return __Name_Builtin_F64_Bits__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ع64_من_بتات")))
        return __Name_Builtin_F64_From_Bits__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_بتات_ع32")))
        return __Name_Builtin_F32_Bits__;
    if (__Identifier_Identity_Equals__(__Name__, SULTANC__BUILTIN_NAME__("بدائي_ع32_من_بتات")))
        return __Name_Builtin_F32_From_Bits__;
    return __Name_Builtin_None__;
}

/* Checks whether the name builtin is direct source. */
int __Name_Builtin_Is_Direct_Source__(__Name_Builtin_Function__ __Builtin__)
{
    return __Builtin__ == __Name_Builtin_Length__ || __Builtin__ == __Name_Builtin_Append__ ||
           __Builtin__ == __Name_Builtin_Swap__ || __Builtin__ == __Name_Builtin_Release__ ||
           __Builtin__ == __Name_Builtin_Raw_Kernel_Call__;
}
