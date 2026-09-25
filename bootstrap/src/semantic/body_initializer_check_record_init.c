/* Checks record initializers. */

#include "semantic/body_internal.h"
#include "support/text/equality.h"

#include <stdlib.h>

/* Checks the body record initializer. */
int __Body_Check_Record_Init__(__Semantic_Body_Context__ *__Context__,
                               __Ast_Statement__ *__Statement__)
{
    /* References the destination type. */
    __Ast_Type__ *__Destination_Type__ = NULL;
    /* References the local. */
    __Semantic_Local__ *__Local__ = NULL;
    /* Stores the resolved. */
    __Resolved_Type__ __Resolved__;
    /* References the declaration. */
    __Ast_Type_Declaration__ *__Declaration__ = NULL;
    /* Tracks the index. */
    size_t __Index__ = 0U;
    /* References the seen. */
    unsigned char *__Seen__ = NULL;
    if (!__Body_Check_Assignable__(__Context__,
                                   __Statement__->__As__.__Record__.__Destination__,
                                   &__Destination_Type__,
                                   &__Local__) ||
        !__Type_Resolve__(__Context__->__Semantic__, __Destination_Type__, &__Resolved__))
    {
        return 0;
    }
    if (__Resolved__.__Kind__ != __Resolved_Type_Struct__ || __Resolved__.__Named__ == NULL)
    {
        return __Body_Fail__(
            __Context__, __E0400_Mismatched_Types__, __Statement__->__Header__.__Span__);
    }
    __Declaration__ = __Resolved__.__Named__->__Declaration__;
    if (__Declaration__->__As__.__Struct__.__Count__ != 0U)
    {
        __Seen__ = (unsigned char *)calloc(__Declaration__->__As__.__Struct__.__Count__, 1U);
        if (__Seen__ == NULL)
        {
            return __Body_Fail__(
                __Context__, __E1100_Internal_Context_Error__, __Statement__->__Header__.__Span__);
        }
    }
    for (__Index__ = 0U; __Index__ < __Statement__->__As__.__Record__.__Field_Count__; ++__Index__)
    {
        /* References the input. */
        __Ast_Record_Input__ *__Input__ = &__Statement__->__As__.__Record__.__Fields__[__Index__];
        /* Tracks the field index. */
        size_t __Field_Index__ = 0U;
        /* Tracks the found state. */
        int __Found__ = 0;
        for (__Field_Index__ = 0U; __Field_Index__ < __Declaration__->__As__.__Struct__.__Count__;
             ++__Field_Index__)
        {
            /* References the field. */
            __Ast_Struct_Field__ *__Field__ =
                &__Declaration__->__As__.__Struct__.__Fields__[__Field_Index__];
            if (__Text_Slice_Equals__(__Field__->__Name__, __Input__->__Name__))
            {
                if (__Seen__[__Field_Index__])
                {
                    free(__Seen__);
                    return __Body_Fail__(__Context__,
                                         __E0615_Record_Field_Duplicate__,
                                         __Statement__->__Header__.__Span__);
                }
                __Seen__[__Field_Index__] = 1U;
                if (!__Body_Check_Atom_Compatible__(__Context__,
                                                    __Field__->__Slot__.__Type__,
                                                    &__Input__->__Value__,
                                                    __Statement__->__Header__.__Span__) ||
                    !__Body_Safety_Consume_Contained_Atom__(__Context__,
                                                            __Field__->__Slot__.__Type__,
                                                            &__Input__->__Value__,
                                                            __Local__,
                                                            __Statement__->__Header__.__Span__))
                {
                    free(__Seen__);
                    return 0;
                }
                __Found__ = 1;
                break;
            }
        }
        if (!__Found__)
        {
            free(__Seen__);
            return __Body_Fail__(
                __Context__, __E0502_Unknown_Record_Member__, __Statement__->__Header__.__Span__);
        }
    }
    for (__Index__ = 0U; __Index__ < __Declaration__->__As__.__Struct__.__Count__; ++__Index__)
    {
        if (!__Seen__[__Index__])
        {
            free(__Seen__);
            return __Body_Fail__(
                __Context__, __E0614_Record_Field_Missing__, __Statement__->__Header__.__Span__);
        }
    }
    free(__Seen__);
    if (__Local__ != NULL)
    {
        __Safety_Fact_Mark_Initialized_At__(&__Local__->__Safety__,
                                            __Statement__->__Header__.__Span__);
    }
    return 1;
}
