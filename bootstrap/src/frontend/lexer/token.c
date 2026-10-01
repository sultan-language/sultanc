/* Queries token lifecycle and typed payloads. */

#include "frontend/lexer/token.h"

#include <stddef.h>

/* Stores the token classes. */
static const __Token_Class__ __Token_Classes__[__Token_Kind_Count__] = {
/* Expands the token macro. */
#define SULTANC__TOKEN__(Name, Selfhost, Class, Lifecycle, Payload)                                \
    [__Token_##Name##__] = __Token_Class_##Class##__,
/* Expands the keyword macro. */
#define SULTANC__KEYWORD__(Name, English, Arabic)
/* Expands the bool keyword macro. */
#define SULTANC__BOOL_KEYWORD__(English, Arabic, Value)
/* Expands the operator macro. */
#define SULTANC__OPERATOR__(Name, Spelling)
#include "frontend/lexer/token_registry.def"
#undef SULTANC__TOKEN__
#undef SULTANC__KEYWORD__
#undef SULTANC__BOOL_KEYWORD__
#undef SULTANC__OPERATOR__
};

/* Stores the token lifecycles. */
static const __Token_Lifecycle__ __Token_Lifecycles__[__Token_Kind_Count__] = {
/* Expands the token macro. */
#define SULTANC__TOKEN__(Name, Selfhost, Class, Lifecycle, Payload)                                \
    [__Token_##Name##__] = __Token_Lifecycle_##Lifecycle##__,
/* Expands the keyword macro. */
#define SULTANC__KEYWORD__(Name, English, Arabic)
/* Expands the bool keyword macro. */
#define SULTANC__BOOL_KEYWORD__(English, Arabic, Value)
/* Expands the operator macro. */
#define SULTANC__OPERATOR__(Name, Spelling)
#include "frontend/lexer/token_registry.def"
#undef SULTANC__TOKEN__
#undef SULTANC__KEYWORD__
#undef SULTANC__BOOL_KEYWORD__
#undef SULTANC__OPERATOR__
};

/* Stores the token payload kinds. */
static const __Token_Payload_Kind__ __Token_Payload_Kinds__[__Token_Kind_Count__] = {
/* Expands the token macro. */
#define SULTANC__TOKEN__(Name, Selfhost, Class, Lifecycle, Payload)                                \
    [__Token_##Name##__] = __Token_Payload_##Payload##__,
/* Expands the keyword macro. */
#define SULTANC__KEYWORD__(Name, English, Arabic)
/* Expands the bool keyword macro. */
#define SULTANC__BOOL_KEYWORD__(English, Arabic, Value)
/* Expands the operator macro. */
#define SULTANC__OPERATOR__(Name, Spelling)
#include "frontend/lexer/token_registry.def"
#undef SULTANC__TOKEN__
#undef SULTANC__KEYWORD__
#undef SULTANC__BOOL_KEYWORD__
#undef SULTANC__OPERATOR__
};

/* Returns the token lifecycle of. */
__Token_Lifecycle__ __Token_Lifecycle_Of__(__Token_Kind__ __Kind__)
{
    if ((size_t)__Kind__ >= (size_t)__Token_Kind_Count__)
    {
        return __Token_Lifecycle_Future_Reserved__;
    }
    return __Token_Lifecycles__[__Kind__];
}

/* Checks whether the token is keyword kind. */
int __Token_Is_Keyword_Kind__(__Token_Kind__ __Kind__)
{
    return (size_t)__Kind__ < (size_t)__Token_Kind_Count__ &&
           __Token_Classes__[__Kind__] == __Token_Class_Keyword__;
}

/* Returns the token payload kind of. */
__Token_Payload_Kind__ __Token_Payload_Kind_Of__(__Token_Kind__ __Kind__)
{
    if ((size_t)__Kind__ >= (size_t)__Token_Kind_Count__)
    {
        return __Token_Payload_None__;
    }
    return __Token_Payload_Kinds__[__Kind__];
}

/* Checks whether the token payload is valid. */
static int __Token_Payload_Is__(const __Token__ *__Token_State__,
                                __Token_Payload_Kind__ __Expected__)
{
    return __Token_State__ != NULL &&
           (size_t)__Token_State__->__Kind__ < (size_t)__Token_Kind_Count__ &&
           __Token_Payload_Kinds__[__Token_State__->__Kind__] == __Expected__;
}

/* Returns the token index. */
int __Token_Get_Index__(const __Token__ *__Token_State__, int64_t *__Out_Value__)
{
    if (__Out_Value__ == NULL || !__Token_Payload_Is__(__Token_State__, __Token_Payload_Index__))
    {
        return 0;
    }
    *__Out_Value__ = __Token_State__->__Payload__.__Index__;
    return 1;
}

/* Returns the token integer. */
int __Token_Get_Integer__(const __Token__ *__Token_State__, __Integer_Literal__ *__Out_Value__)
{
    if (__Out_Value__ == NULL || !__Token_Payload_Is__(__Token_State__, __Token_Payload_Integer__))
    {
        return 0;
    }
    *__Out_Value__ = __Token_State__->__Payload__.__Integer__;
    return 1;
}

/* Returns the token text. */
int __Token_Get_Text__(const __Token__ *__Token_State__, __Text_Slice__ *__Out_Value__)
{
    if (__Out_Value__ == NULL || !__Token_Payload_Is__(__Token_State__, __Token_Payload_Text__))
    {
        return 0;
    }
    *__Out_Value__ = __Token_State__->__Payload__.__Text__;
    return 1;
}

/* Returns the token character. */
int __Token_Get_Character__(const __Token__ *__Token_State__, uint32_t *__Out_Value__)
{
    if (__Out_Value__ == NULL ||
        !__Token_Payload_Is__(__Token_State__, __Token_Payload_Character__))
    {
        return 0;
    }
    *__Out_Value__ = __Token_State__->__Payload__.__Character__;
    return 1;
}

/* Returns the token boolean. */
int __Token_Get_Boolean__(const __Token__ *__Token_State__, int *__Out_Value__)
{
    if (__Out_Value__ == NULL || !__Token_Payload_Is__(__Token_State__, __Token_Payload_Boolean__))
    {
        return 0;
    }
    *__Out_Value__ = __Token_State__->__Payload__.__Boolean__;
    return 1;
}
