/* Reads typed payloads from parser tokens. */

#include "frontend/parser/token_payload.h"
#include "frontend/parser/diagnostic.h"

/* Returns the parser payload invariant failed. */
static int __Parser_Payload_Invariant_Failed__(__Parser__ *__Parser_State__)
{
    return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_E1100__);
}

/* Returns the parser current integer. */
int __Parser_Current_Integer__(__Parser__ *__Parser_State__, __Integer_Literal__ *__Out_Value__)
{
    if (__Parser_State__ == NULL ||
        !__Token_Get_Integer__(&__Parser_State__->__Current__, __Out_Value__))
    {
        return __Parser_Payload_Invariant_Failed__(__Parser_State__);
    }
    return 1;
}

/* Returns the parser current text. */
int __Parser_Current_Text__(__Parser__ *__Parser_State__, __Text_Slice__ *__Out_Value__)
{
    if (__Parser_State__ == NULL ||
        !__Token_Get_Text__(&__Parser_State__->__Current__, __Out_Value__))
    {
        return __Parser_Payload_Invariant_Failed__(__Parser_State__);
    }
    return 1;
}

/* Returns the parser current character. */
int __Parser_Current_Character__(__Parser__ *__Parser_State__, uint32_t *__Out_Value__)
{
    if (__Parser_State__ == NULL ||
        !__Token_Get_Character__(&__Parser_State__->__Current__, __Out_Value__))
    {
        return __Parser_Payload_Invariant_Failed__(__Parser_State__);
    }
    return 1;
}

/* Returns the parser current boolean. */
int __Parser_Current_Boolean__(__Parser__ *__Parser_State__, int *__Out_Value__)
{
    if (__Parser_State__ == NULL ||
        !__Token_Get_Boolean__(&__Parser_State__->__Current__, __Out_Value__))
    {
        return __Parser_Payload_Invariant_Failed__(__Parser_State__);
    }
    return 1;
}
