#include "semantic/body_internal.h"

int __Body_Check_Box_Init__(__Semantic_Body_Context__ *__Context__,
                            __Ast_Statement__ *__Statement__)
{
    __Ast_Type__ *__Destination_Type__ = NULL;
    __Semantic_Local__ *__Local__ = NULL;
    __Resolved_Type__ __Resolved__;

    if (!__Body_Check_Assignable__(__Context__,
                                   __Statement__->__As__.__Box__.__Destination__,
                                   &__Destination_Type__,
                                   &__Local__) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Destination_Type__, &__Resolved__))
    {
        return 0;
    }

    if (__Resolved__.__Kind__ != __Resolved_Type_Box__ || __Resolved__.__Inner__ == NULL)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Statement__->__Header__.__Span__);
    }

    if (!__Body_Check_Atom_Compatible__(__Context__,
                                        __Resolved__.__Inner__,
                                        &__Statement__->__As__.__Box__.__Value__,
                                        __Statement__->__Header__.__Span__) ||
        (__Safety_Type_Is_Move_Only__(__Context__->__Semantic__, __Resolved__.__Inner__) &&
         !__Body_Safety_Move_Atom__(__Context__,
                                    &__Statement__->__As__.__Box__.__Value__,
                                    __Statement__->__Header__.__Span__)))
    {
        return 0;
    }

    if (__Local__ != NULL)
    {
        __Safety_Fact_Mark_Initialized_At__(&__Local__->__Safety__,
                                            __Statement__->__Header__.__Span__);
    }
    return 1;
}
