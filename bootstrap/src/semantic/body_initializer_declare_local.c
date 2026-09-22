#include "semantic/body_internal.h"

#include <string.h>

int __Body_Declare_Local__(__Semantic_Body_Context__ *__Context__,
                           __Ast_Variable_Declaration__ *__Declaration__,
                           __Source_Span__ __Span__)
{
    __Semantic_Local__ __Local__;
    memset(&__Local__, 0, sizeof(__Local__));
    __Local__.__Name_Kind__ = __Declaration__->__Name_Kind__;
    if (__Local__.__Name_Kind__ == __Ast_Lvalue_Base_Identifier__)
    {
        __Local__.__Name__ = __Declaration__->__Name__.__Identifier__;
    }
    else
    {
        __Local__.__Temporary__ = __Declaration__->__Name__.__Temporary__;
    }
    __Local__.__Type__ = __Declaration__->__Slot__.__Type__;
    __Local__.__Slot__ = &__Declaration__->__Slot__;
    __Local__.__Mutable__ =
        (__Local__.__Type__ != NULL && __Local__.__Type__->__Kind__ == __Ast_Type_Mutable__);
    __Safety_Fact_Init__(&__Local__.__Safety__, 0);
    __Local__.__Scope_Depth__ = __Context__->__Scope_Depth__;
    __Local__.__Declaration_Span__ = __Span__;
    if (__Body_Name_Exists_In_Current_Scope__(__Context__, &__Local__))
    {
        return __Body_Fail__(__Context__,
                             __Local__.__Name_Kind__ == __Ast_Lvalue_Base_Temporary__
                                 ? __E0206_Duplicate_Temporary__
                                 : __E0205_Duplicate_Declaration__,
                             __Span__);
    }
    if (__Local__.__Type__ != NULL)
    {
        size_t __Size__ = 0U, __Alignment__ = 0U;
        if (!__Layout_Type__(
                __Context__->__Semantic__, __Local__.__Type__, &__Size__, &__Alignment__))
        {
            return 0;
        }
    }
    if (__Vector_Push__(&__Context__->__Locals__, &__Local__) == NULL)
    {
        return __Body_Fail__(__Context__, __E1100_Internal_Context_Error__, __Span__);
    }
    return 1;
}
