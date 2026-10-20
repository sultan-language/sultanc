/* Checks whether lvalues can be assigned. */

#include "semantic/body_internal.h"
#include "kernel/name/name.h"

/* Checks the body assignable. */
int __Body_Check_Assignable__(__Semantic_Body_Context__ *__Context__,
                              __Ast_Lvalue__ *__Lvalue__,
                              __Ast_Type__ **__Out_Type__,
                              __Semantic_Local__ **__Out_Local__)
{
    /* References the local. */
    __Semantic_Local__ *__Local__ = NULL;
    if (!__Body_Infer_Lvalue_For_Write__(__Context__, __Lvalue__, __Out_Type__, &__Local__))
    {
        return 0;
    }
    if (__Lvalue__->__Kind__ == __Ast_Lvalue_Base__)
    {
        if (__Local__ == NULL)
        {
            /* A writable module-level value was already validated (متغير wrapper)
             * by the write inference above; it has no local safety state. */
            __Semantic_Global_Entry__ *__Global__ = NULL;
            if (__Lvalue__->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__ ||
                __Name_Resolve_Global__(__Context__->__Semantic__,
                                        __Context__->__Function__->__Unit__,
                                        __Lvalue__->__As__.__Base__.__As__.__Identifier__,
                                        &__Global__) != __Name_Lookup_Found__ ||
                __Global__ == NULL)
            {
                return 0;
            }
            if (__Out_Local__ != NULL)
            {
                *__Out_Local__ = NULL;
            }
            return 1;
        }
        if (!__Local__->__Mutable__ &&
            __Local__->__Safety__.__State__ == __Safety_Value_Initialized__)
        {
            return __Body_Fail__(
                __Context__, __E0703_Write_To_Immutable__, __Lvalue__->__Header__.__Span__);
        }
        if (!__Safety_Fact_Can_Write__(&__Local__->__Safety__))
        {
            return __Body_Fail__(
                __Context__, __E1302_Borrow_Conflict__, __Lvalue__->__Header__.__Span__);
        }
    }
    if (__Out_Local__ != NULL)
    {
        *__Out_Local__ = __Local__;
    }
    return 1;
}
