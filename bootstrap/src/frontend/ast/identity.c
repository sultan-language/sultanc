#include "frontend/ast/identity.h"
#include "frontend/identifier_identity.h"

int __Ast_Base_Identity_Matches__(__Ast_Lvalue_Base_Kind__ __Left_Kind__,
                                  __Text_Slice__ __Left_Name__,
                                  __Temporary_Id__ __Left_Temporary__,
                                  __Ast_Lvalue_Base_Kind__ __Right_Kind__,
                                  __Text_Slice__ __Right_Name__,
                                  __Temporary_Id__ __Right_Temporary__)
{
    if (__Left_Kind__ != __Right_Kind__)
    {
        return 0;
    }
    if (__Left_Kind__ == __Ast_Lvalue_Base_Identifier__)
    {
        return __Identifier_Identity_Equals__(__Left_Name__, __Right_Name__);
    }
    return __Left_Temporary__ == __Right_Temporary__;
}

int __Ast_Base_Identity_Matches_Lvalue__(__Ast_Lvalue_Base_Kind__ __Kind__,
                                         __Text_Slice__ __Name__,
                                         __Temporary_Id__ __Temporary__,
                                         const __Ast_Lvalue__ *__Lvalue__)
{
    __Text_Slice__ __Lvalue_Name__ = {0};
    __Temporary_Id__ __Lvalue_Temporary__ = 0U;

    if (__Lvalue__ == NULL || __Lvalue__->__Kind__ != __Ast_Lvalue_Base__)
    {
        return 0;
    }
    if (__Lvalue__->__As__.__Base__.__Kind__ == __Ast_Lvalue_Base_Identifier__)
    {
        __Lvalue_Name__ = __Lvalue__->__As__.__Base__.__As__.__Identifier__;
    }
    else
    {
        __Lvalue_Temporary__ = __Lvalue__->__As__.__Base__.__As__.__Temporary__;
    }
    return __Ast_Base_Identity_Matches__(__Kind__,
                                         __Name__,
                                         __Temporary__,
                                         __Lvalue__->__As__.__Base__.__Kind__,
                                         __Lvalue_Name__,
                                         __Lvalue_Temporary__);
}
