#ifndef SULTANC__FRONTEND_LEXER_TOKEN_H__
#define SULTANC__FRONTEND_LEXER_TOKEN_H__

#include "core/source.h"

#include <stdint.h>

typedef enum
{
#define SULTANC__TOKEN__(Name, Selfhost, Class, Lifecycle, Payload) __Token_##Name##__,
#define SULTANC__KEYWORD__(Name, English, Arabic)
#define SULTANC__BOOL_KEYWORD__(English, Arabic, Value)
#define SULTANC__OPERATOR__(Name, Spelling)
#include "token_registry.def"
#undef SULTANC__TOKEN__
#undef SULTANC__KEYWORD__
#undef SULTANC__BOOL_KEYWORD__
#undef SULTANC__OPERATOR__
    __Token_Kind_Count__
} __Token_Kind__;

typedef enum
{
    __Token_Class_Keyword__,
    __Token_Class_Operator__,
    __Token_Class_Delimiter__,
    __Token_Class_Value__,
    __Token_Class_Identifier__,
    __Token_Class_Eof__
} __Token_Class__;

typedef enum
{
    __Token_Lifecycle_Active__,
    __Token_Lifecycle_Future_Reserved__
} __Token_Lifecycle__;

typedef enum
{
    __Token_Payload_None__,
    __Token_Payload_Index__,
    __Token_Payload_Integer__,
    __Token_Payload_Text__,
    __Token_Payload_Character__,
    __Token_Payload_Boolean__
} __Token_Payload_Kind__;

typedef struct
{
    int64_t __Value__;
    __Text_Slice__ __Spelling__;
} __Integer_Literal__;

typedef union
{
    int64_t __Index__;
    __Text_Slice__ __Text__;
    __Integer_Literal__ __Integer__;
    uint32_t __Character__;
    int __Boolean__;
} __Token_Payload__;

typedef struct
{
    __Token_Kind__ __Kind__;
    __Source_Span__ __Span__;
    __Text_Slice__ __Lexeme__;
    __Token_Payload__ __Payload__;
} __Token__;

__Token_Lifecycle__ __Token_Lifecycle_Of__(__Token_Kind__ __Kind__);

__Token_Payload_Kind__ __Token_Payload_Kind_Of__(__Token_Kind__ __Kind__);

int __Token_Is_Keyword_Kind__(__Token_Kind__ __Kind__);

int __Token_Get_Index__(const __Token__ *__Token_State__, int64_t *__Out_Value__);

int __Token_Get_Integer__(const __Token__ *__Token_State__, __Integer_Literal__ *__Out_Value__);

int __Token_Get_Text__(const __Token__ *__Token_State__, __Text_Slice__ *__Out_Value__);

int __Token_Get_Character__(const __Token__ *__Token_State__, uint32_t *__Out_Value__);

int __Token_Get_Boolean__(const __Token__ *__Token_State__, int *__Out_Value__);

#endif
