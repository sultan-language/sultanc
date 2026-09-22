#include "kernel/type/type.h"

int __Type_Compatible__(__Semantic_Context__ *__Context__,
                        __Ast_Type__ *__Left__,
                        __Ast_Type__ *__Right__)
{
    __Resolved_Type__ __A__;
    __Resolved_Type__ __B__;
    __Ast_Type__ *__Left_Base__ = __Type_Unwrap_Mutable__(__Left__);
    __Ast_Type__ *__Right_Base__ = __Type_Unwrap_Mutable__(__Right__);
    if (__Left_Base__ == NULL || __Right_Base__ == NULL)
    {
        return 0;
    }
    if (!__Type_Resolve__(__Context__, __Left_Base__, &__A__) ||
        !__Type_Resolve__(__Context__, __Right_Base__, &__B__))
    {
        return 0;
    }
    if (__A__.__Kind__ != __B__.__Kind__)
    {
        return 0;
    }
    if ((__A__.__Kind__ == __Resolved_Type_Signed_Integer__ ||
         __A__.__Kind__ == __Resolved_Type_Unsigned_Integer__ ||
         __A__.__Kind__ == __Resolved_Type_Float__) &&
        __A__.__Bits__ != __B__.__Bits__)
    {
        return 0;
    }
    if (__A__.__Kind__ == __Resolved_Type_Reference__ || __A__.__Kind__ == __Resolved_Type_Box__ ||
        __A__.__Kind__ == __Resolved_Type_Option__ || __A__.__Kind__ == __Resolved_Type_Vector__)
    {
        if (__A__.__Kind__ == __Resolved_Type_Reference__ &&
            __A__.__Reference_Mutable__ != __B__.__Reference_Mutable__)
        {
            return 0;
        }
        return __Type_Compatible__(__Context__, __A__.__Inner__, __B__.__Inner__);
    }
    if (__A__.__Kind__ == __Resolved_Type_Result__)
    {
        return __Type_Compatible__(__Context__,
                                   __Left_Base__->__As__.__Result__.__Ok__,
                                   __Right_Base__->__As__.__Result__.__Ok__) &&
               __Type_Compatible__(__Context__,
                                   __Left_Base__->__As__.__Result__.__Error__,
                                   __Right_Base__->__As__.__Result__.__Error__);
    }
    if ((__A__.__Kind__ == __Resolved_Type_Struct__ || __A__.__Kind__ == __Resolved_Type_Enum__) &&
        __A__.__Named__ != __B__.__Named__)
    {
        return 0;
    }
    return 1;
}
