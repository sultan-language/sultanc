/* Defines lexical token kinds and payloads. */

#ifndef SULTANC__FRONTEND_LEXER_TOKEN_H__
#define SULTANC__FRONTEND_LEXER_TOKEN_H__

#include "core/source.h"

#include <stdint.h>

/* Defines the token kind values. */
typedef enum
{
/* Expands the token macro. */
#define SULTANC__TOKEN__(Name, Selfhost, Class, Lifecycle, Payload) __Token_##Name##__,
/* Expands the keyword macro. */
#define SULTANC__KEYWORD__(Name, English, Arabic)
/* Expands the bool keyword macro. */
#define SULTANC__BOOL_KEYWORD__(English, Arabic, Value)
/* Expands the operator macro. */
#define SULTANC__OPERATOR__(Name, Spelling)
#include "token_registry.def"
#undef SULTANC__TOKEN__
#undef SULTANC__KEYWORD__
#undef SULTANC__BOOL_KEYWORD__
#undef SULTANC__OPERATOR__
    /* Represents the token kind count value. */
    __Token_Kind_Count__
} __Token_Kind__;

/* Defines the token class values. */
typedef enum
{
    /* Represents the token class keyword value. */
    __Token_Class_Keyword__,
    /* Represents the token class operator value. */
    __Token_Class_Operator__,
    /* Represents the token class delimiter value. */
    __Token_Class_Delimiter__,
    /* Represents the token class value value. */
    __Token_Class_Value__,
    /* Represents the token class identifier value. */
    __Token_Class_Identifier__,
    /* Represents the token class EOF value. */
    __Token_Class_Eof__
} __Token_Class__;

/* Defines the token lifecycle values. */
typedef enum
{
    /* Represents the token lifecycle active value. */
    __Token_Lifecycle_Active__,
    /* Represents the token lifecycle future reserved value. */
    __Token_Lifecycle_Future_Reserved__
} __Token_Lifecycle__;

/* Defines the token payload kind values. */
typedef enum
{
    /* Represents the token payload none value. */
    __Token_Payload_None__,
    /* Represents the token payload index value. */
    __Token_Payload_Index__,
    /* Represents the token payload integer value. */
    __Token_Payload_Integer__,
    /* Represents the token payload text value. */
    __Token_Payload_Text__,
    /* Represents the token payload character value. */
    __Token_Payload_Character__,
    /* Represents the token payload boolean value. */
    __Token_Payload_Boolean__
} __Token_Payload_Kind__;

/* Defines the integer literal structure. */
typedef struct
{
    /* Stores the value. */
    int64_t __Value__;
    /* Stores the spelling. */
    __Text_Slice__ __Spelling__;
} __Integer_Literal__;

/* Defines the token payload union. */
typedef union
{
    /* Tracks the index. */
    int64_t __Index__;
    /* Stores the text. */
    __Text_Slice__ __Text__;
    /* Stores the integer. */
    __Integer_Literal__ __Integer__;
    /* Stores the character. */
    uint32_t __Character__;
    /* Stores the boolean. */
    int __Boolean__;
} __Token_Payload__;

/* Defines the token structure. */
typedef struct
{
    /* Stores the kind. */
    __Token_Kind__ __Kind__;
    /* Stores the span. */
    __Source_Span__ __Span__;
    /* Stores the lexeme. */
    __Text_Slice__ __Lexeme__;
    /* Stores the payload. */
    __Token_Payload__ __Payload__;
} __Token__;

/* Returns the token lifecycle of. */
__Token_Lifecycle__ __Token_Lifecycle_Of__(__Token_Kind__ __Kind__);

/* Returns the token payload kind of. */
__Token_Payload_Kind__ __Token_Payload_Kind_Of__(__Token_Kind__ __Kind__);

/* Checks whether the token is keyword kind. */
int __Token_Is_Keyword_Kind__(__Token_Kind__ __Kind__);

/* Returns the token index. */
int __Token_Get_Index__(const __Token__ *__Token_State__, int64_t *__Out_Value__);

/* Returns the token integer. */
int __Token_Get_Integer__(const __Token__ *__Token_State__, __Integer_Literal__ *__Out_Value__);

/* Returns the token text. */
int __Token_Get_Text__(const __Token__ *__Token_State__, __Text_Slice__ *__Out_Value__);

/* Returns the token character. */
int __Token_Get_Character__(const __Token__ *__Token_State__, uint32_t *__Out_Value__);

/* Returns the token boolean. */
int __Token_Get_Boolean__(const __Token__ *__Token_State__, int *__Out_Value__);

#endif
