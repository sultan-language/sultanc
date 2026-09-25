/* Stores validated token payloads. */

#include "frontend/lexer/cursor.h"

#include <string.h>

/* Sets the lexer token header. */
static void __Lexer_Set_Token_Header__(__Lexer__ *__Lexer_State__,
                                       __Token__ *__Token_State__,
                                       __Token_Kind__ __Kind__,
                                       __Source_Position__ __Start__,
                                       size_t __Start_Offset__)
{
    memset(__Token_State__, 0, sizeof(*__Token_State__));
    __Token_State__->__Kind__ = __Kind__;
    __Token_State__->__Span__.__Start__ = __Start__;
    __Token_State__->__Span__.__End__ = __Lexer_Position__(__Lexer_State__);
    __Token_State__->__Lexeme__ = __Source_Slice__(
        __Lexer_State__->__Source__, __Start_Offset__, __Lexer_State__->__Offset__);
}

/* Requires the lexer payload kind. */
static int __Lexer_Require_Payload_Kind__(__Lexer__ *__Lexer_State__,
                                          __Token_Kind__ __Kind__,
                                          __Token_Payload_Kind__ __Expected__,
                                          __Source_Position__ __Start__)
{
    if (__Token_Payload_Kind_Of__(__Kind__) == __Expected__)
    {
        return 1;
    }
    return __Lexer_Fail__(
        __Lexer_State__, __E1100_Internal_Context_Error__, __Start__, __Diag_Word_E1100__);
}

/* Sets the lexer token. */
int __Lexer_Set_Token__(__Lexer__ *__Lexer_State__,
                        __Token__ *__Token_State__,
                        __Token_Kind__ __Kind__,
                        __Source_Position__ __Start__,
                        size_t __Start_Offset__)
{
    if (!__Lexer_Require_Payload_Kind__(
            __Lexer_State__, __Kind__, __Token_Payload_None__, __Start__))
    {
        return 0;
    }
    __Lexer_Set_Token_Header__(
        __Lexer_State__, __Token_State__, __Kind__, __Start__, __Start_Offset__);
    return 1;
}

/* Sets the lexer token index. */
int __Lexer_Set_Token_Index__(__Lexer__ *__Lexer_State__,
                              __Token__ *__Token_State__,
                              __Token_Kind__ __Kind__,
                              __Source_Position__ __Start__,
                              size_t __Start_Offset__,
                              int64_t __Value__)
{
    if (!__Lexer_Require_Payload_Kind__(
            __Lexer_State__, __Kind__, __Token_Payload_Index__, __Start__))
    {
        return 0;
    }
    __Lexer_Set_Token_Header__(
        __Lexer_State__, __Token_State__, __Kind__, __Start__, __Start_Offset__);
    __Token_State__->__Payload__.__Index__ = __Value__;
    return 1;
}

/* Sets the lexer token integer. */
int __Lexer_Set_Token_Integer__(__Lexer__ *__Lexer_State__,
                                __Token__ *__Token_State__,
                                __Token_Kind__ __Kind__,
                                __Source_Position__ __Start__,
                                size_t __Start_Offset__,
                                __Integer_Literal__ __Value__)
{
    if (!__Lexer_Require_Payload_Kind__(
            __Lexer_State__, __Kind__, __Token_Payload_Integer__, __Start__))
    {
        return 0;
    }
    __Lexer_Set_Token_Header__(
        __Lexer_State__, __Token_State__, __Kind__, __Start__, __Start_Offset__);
    __Token_State__->__Payload__.__Integer__ = __Value__;
    return 1;
}

/* Sets the lexer token text. */
int __Lexer_Set_Token_Text__(__Lexer__ *__Lexer_State__,
                             __Token__ *__Token_State__,
                             __Token_Kind__ __Kind__,
                             __Source_Position__ __Start__,
                             size_t __Start_Offset__,
                             __Text_Slice__ __Value__)
{
    if (!__Lexer_Require_Payload_Kind__(
            __Lexer_State__, __Kind__, __Token_Payload_Text__, __Start__))
    {
        return 0;
    }
    __Lexer_Set_Token_Header__(
        __Lexer_State__, __Token_State__, __Kind__, __Start__, __Start_Offset__);
    __Token_State__->__Payload__.__Text__ = __Value__;
    return 1;
}

/* Sets the lexer token character. */
int __Lexer_Set_Token_Character__(__Lexer__ *__Lexer_State__,
                                  __Token__ *__Token_State__,
                                  __Token_Kind__ __Kind__,
                                  __Source_Position__ __Start__,
                                  size_t __Start_Offset__,
                                  uint32_t __Value__)
{
    if (!__Lexer_Require_Payload_Kind__(
            __Lexer_State__, __Kind__, __Token_Payload_Character__, __Start__))
    {
        return 0;
    }
    __Lexer_Set_Token_Header__(
        __Lexer_State__, __Token_State__, __Kind__, __Start__, __Start_Offset__);
    __Token_State__->__Payload__.__Character__ = __Value__;
    return 1;
}

/* Sets the lexer token boolean. */
int __Lexer_Set_Token_Boolean__(__Lexer__ *__Lexer_State__,
                                __Token__ *__Token_State__,
                                __Token_Kind__ __Kind__,
                                __Source_Position__ __Start__,
                                size_t __Start_Offset__,
                                int __Value__)
{
    if (!__Lexer_Require_Payload_Kind__(
            __Lexer_State__, __Kind__, __Token_Payload_Boolean__, __Start__))
    {
        return 0;
    }
    __Lexer_Set_Token_Header__(
        __Lexer_State__, __Token_State__, __Kind__, __Start__, __Start_Offset__);
    __Token_State__->__Payload__.__Boolean__ = __Value__;
    return 1;
}
