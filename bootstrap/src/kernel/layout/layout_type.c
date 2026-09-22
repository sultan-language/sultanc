#include "semantic/diagnostic.h"
#include "kernel/layout/layout.h"
#include "semantic/type_named.h"
#include "kernel/type/type.h"
#include "support/memory/alignment.h"

static int __Named_Layout__(__Semantic_Context__ *__Context__,
                            __Semantic_Type_Entry__ *__Entry__,
                            size_t *__Out_Size__,
                            size_t *__Out_Alignment__)
{
    __Ast_Type_Declaration__ *__Declaration__ = NULL;
    size_t __Size__ = 0U;
    size_t __Alignment__ = 1U;

    if (__Entry__ == NULL || __Entry__->__Declaration__ == NULL)
    {
        return __Semantic_Fail__(__Context__, __E0409_Unresolved_Type__, (__Source_Span__){0});
    }

    if (__Entry__->__Layout_State__ == 2)
    {
        *__Out_Size__ = __Entry__->__Size__;
        *__Out_Alignment__ = __Entry__->__Alignment__;
        return 1;
    }

    if (__Entry__->__Layout_State__ == 1)
    {
        return __Semantic_Fail__(
            __Context__, __E0308_Infinite_Recursive_Type__, (__Source_Span__){0});
    }

    __Entry__->__Layout_State__ = 1;
    __Declaration__ = __Entry__->__Declaration__;

    if (__Declaration__->__Kind__ == __Ast_Type_Decl_Struct__)
    {
        size_t __Index__ = 0U;

        for (__Index__ = 0U; __Index__ < __Declaration__->__As__.__Struct__.__Count__; ++__Index__)
        {
            __Ast_Struct_Field__ *__Field__ =
                &__Declaration__->__As__.__Struct__.__Fields__[__Index__];
            size_t __Field_Size__ = 0U;
            size_t __Field_Alignment__ = 0U;

            if (!__Layout_Type__(__Context__,
                                 __Field__->__Slot__.__Type__,
                                 &__Field_Size__,
                                 &__Field_Alignment__))
            {
                return 0;
            }

            __Size__ = __Align_Up__(__Size__, __Field_Alignment__);
            __Size__ += __Field_Size__;
            if (__Field_Alignment__ > __Alignment__)
            {
                __Alignment__ = __Field_Alignment__;
            }
        }

        __Size__ = __Align_Up__(__Size__, __Alignment__);
    }
    else
    {
        size_t __Constructor_Index__ = 0U;
        size_t __Payload_Max__ = 0U;
        size_t __Payload_Alignment__ = 1U;

        for (__Constructor_Index__ = 0U;
             __Constructor_Index__ < __Declaration__->__As__.__Enum__.__Count__;
             ++__Constructor_Index__)
        {
            __Ast_Enum_Constructor__ *__Constructor__ =
                &__Declaration__->__As__.__Enum__.__Constructors__[__Constructor_Index__];
            size_t __Payload_Size__ = 0U;
            size_t __Index__ = 0U;
            size_t __Constructor_Alignment__ = 1U;

            for (__Index__ = 0U; __Index__ < __Constructor__->__Payload_Count__; ++__Index__)
            {
                size_t __Field_Size__ = 0U;
                size_t __Field_Alignment__ = 0U;

                if (!__Layout_Type__(__Context__,
                                     __Constructor__->__Payload_Slots__[__Index__].__Type__,
                                     &__Field_Size__,
                                     &__Field_Alignment__))
                {
                    return 0;
                }

                __Payload_Size__ = __Align_Up__(__Payload_Size__, __Field_Alignment__);
                __Payload_Size__ += __Field_Size__;
                if (__Field_Alignment__ > __Constructor_Alignment__)
                {
                    __Constructor_Alignment__ = __Field_Alignment__;
                }
            }

            __Payload_Size__ = __Align_Up__(__Payload_Size__, __Constructor_Alignment__);
            if (__Payload_Size__ > __Payload_Max__)
            {
                __Payload_Max__ = __Payload_Size__;
            }
            if (__Constructor_Alignment__ > __Payload_Alignment__)
            {
                __Payload_Alignment__ = __Constructor_Alignment__;
            }
        }

        __Alignment__ = __Payload_Alignment__ > 8U ? __Payload_Alignment__ : 8U;
        __Size__ = __Align_Up__(8U, __Payload_Alignment__) + __Payload_Max__;
        __Size__ = __Align_Up__(__Size__, __Alignment__);
    }

    __Entry__->__Size__ = __Size__;
    __Entry__->__Alignment__ = __Alignment__;
    __Entry__->__Layout_State__ = 2;
    *__Out_Size__ = __Size__;
    *__Out_Alignment__ = __Alignment__;
    return 1;
}

int __Layout_Type__(__Semantic_Context__ *__Context__,
                    __Ast_Type__ *__Type__,
                    size_t *__Out_Size__,
                    size_t *__Out_Alignment__)
{
    __Ast_Type__ *__Base__ = __Type_Unwrap_Mutable__(__Type__);
    size_t __Size__ = 0U;
    size_t __Alignment__ = 1U;

    if (__Base__ == NULL)
    {
        return __Semantic_Fail__(__Context__, __E0409_Unresolved_Type__, (__Source_Span__){0});
    }

    switch (__Base__->__Kind__)
    {
        case __Ast_Type_Any__:
        case __Ast_Type_Integer__:
        case __Ast_Type_Unsigned_Integer__:
        case __Ast_Type_Reference__:
        case __Ast_Type_Box__:
        case __Ast_Type_Boolean__:
        case __Ast_Type_Character__:
            __Size__ = 8U;
            __Alignment__ = 8U;
            break;

        case __Ast_Type_String__:
        case __Ast_Type_Vector__:
            __Size__ = 24U;
            __Alignment__ = 8U;
            break;

        case __Ast_Type_Option__:
        case __Ast_Type_Result__:
        {
            __Ast_Type__ *__Payloads__[2] = {NULL, NULL};
            size_t __Payload_Count__ = 0U;
            size_t __Payload_Max__ = 0U;
            size_t __Payload_Alignment__ = 1U;
            size_t __Index__;

            if (__Base__->__Kind__ == __Ast_Type_Option__)
            {
                __Payloads__[0] = __Base__->__As__.__Inner__;
                __Payload_Count__ = 1U;
            }
            else
            {
                __Payloads__[0] = __Base__->__As__.__Result__.__Ok__;
                __Payloads__[1] = __Base__->__As__.__Result__.__Error__;
                __Payload_Count__ = 2U;
            }

            for (__Index__ = 0U; __Index__ < __Payload_Count__; ++__Index__)
            {
                size_t __Payload_Size__ = 0U;
                size_t __Current_Alignment__ = 1U;
                if (!__Layout_Type__(__Context__,
                                     __Payloads__[__Index__],
                                     &__Payload_Size__,
                                     &__Current_Alignment__))
                {
                    return 0;
                }
                if (__Payload_Size__ > __Payload_Max__)
                {
                    __Payload_Max__ = __Payload_Size__;
                }
                if (__Current_Alignment__ > __Payload_Alignment__)
                {
                    __Payload_Alignment__ = __Current_Alignment__;
                }
            }

            __Alignment__ = __Payload_Alignment__ > 8U ? __Payload_Alignment__ : 8U;
            __Size__ = __Align_Up__(8U, __Payload_Alignment__) + __Payload_Max__;
            __Size__ = __Align_Up__(__Size__, __Alignment__);
            break;
        }

        case __Ast_Type_Void__:
            __Size__ = 0U;
            __Alignment__ = 1U;
            break;

        case __Ast_Type_Machine__:
            switch (__Base__->__As__.__Machine__)
            {
                case __Machine_I8__:
                case __Machine_U8__:
                    __Size__ = 1U;
                    __Alignment__ = 1U;
                    break;

                case __Machine_I16__:
                case __Machine_U16__:
                    __Size__ = 2U;
                    __Alignment__ = 2U;
                    break;

                case __Machine_I32__:
                case __Machine_U32__:
                case __Machine_F32__:
                    __Size__ = 4U;
                    __Alignment__ = 4U;
                    break;

                case __Machine_I64__:
                case __Machine_U64__:
                case __Machine_F64__:
                    __Size__ = 8U;
                    __Alignment__ = 8U;
                    break;
            }
            break;

        case __Ast_Type_Named__:
        {
            __Semantic_Type_Entry__ *__Entry__ = NULL;

            if (!__Semantic_Resolve_Named_Entry__(__Context__, __Base__, &__Entry__))
            {
                return 0;
            }
            return __Named_Layout__(__Context__, __Entry__, __Out_Size__, __Out_Alignment__);
        }

        case __Ast_Type_Mutable__:
            return __Layout_Type__(
                __Context__, __Base__->__As__.__Inner__, __Out_Size__, __Out_Alignment__);
    }

    *__Out_Size__ = __Size__;
    *__Out_Alignment__ = __Alignment__;
    return 1;
}
