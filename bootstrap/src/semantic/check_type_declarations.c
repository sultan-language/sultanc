/* Validates type declarations. */

#include "semantic/check_type_declarations.h"
#include "semantic/check_type_reference.h"
#include "kernel/layout/layout.h"

/* Checks the semantic type declarations. */
int __Semantic_Check_Type_Declarations__(__Semantic_Context__ *__Context__)
{
    /* Tracks the index. */
    size_t __Index__ = 0U;

    for (__Index__ = 0U; __Index__ < __Context__->__Types__.__Count__; ++__Index__)
    {
        /* References the type. */
        __Semantic_Type_Entry__ *__Type__ =
            (__Semantic_Type_Entry__ *)__Vector_At__(&__Context__->__Types__, __Index__);
        /* References the declaration. */
        __Ast_Type_Declaration__ *__Declaration__ = NULL;
        /* Tracks the inner index. */
        size_t __Inner_Index__ = 0U;
        /* Stores the size. */
        size_t __Size__ = 0U;
        /* Stores the alignment. */
        size_t __Alignment__ = 0U;
        /* Stores the named type. */
        __Ast_Type__ __Named_Type__;

        if (__Type__ == NULL)
        {
            continue;
        }
        __Context__->__Active_Unit__ = __Type__->__Unit__;
        __Declaration__ = __Type__->__Declaration__;
        if (__Declaration__->__Kind__ == __Ast_Type_Decl_Struct__)
        {
            for (__Inner_Index__ = 0U;
                 __Inner_Index__ < __Declaration__->__As__.__Struct__.__Count__;
                 ++__Inner_Index__)
            {
                if (!__Semantic_Check_Type_Reference__(
                        __Context__,
                        __Declaration__->__As__.__Struct__.__Fields__[__Inner_Index__]
                            .__Slot__.__Type__))
                {
                    return 0;
                }
            }
        }
        else
        {
            for (__Inner_Index__ = 0U; __Inner_Index__ < __Declaration__->__As__.__Enum__.__Count__;
                 ++__Inner_Index__)
            {
                /* References the constructor. */
                __Ast_Enum_Constructor__ *__Constructor__ =
                    &__Declaration__->__As__.__Enum__.__Constructors__[__Inner_Index__];
                /* Tracks the payload index. */
                size_t __Payload_Index__ = 0U;

                for (__Payload_Index__ = 0U; __Payload_Index__ < __Constructor__->__Payload_Count__;
                     ++__Payload_Index__)
                {
                    if (!__Semantic_Check_Type_Reference__(
                            __Context__,
                            __Constructor__->__Payload_Slots__[__Payload_Index__].__Type__))
                    {
                        return 0;
                    }
                }
            }
        }

        __Named_Type__.__Kind__ = __Ast_Type_Named__;
        __Named_Type__.__As__.__Named__.__Name__ = __Type__->__Name__;
        if (!__Layout_Type__(__Context__, &__Named_Type__, &__Size__, &__Alignment__))
        {
            return 0;
        }
    }
    __Context__->__Active_Unit__ = NULL;
    return 1;
}
