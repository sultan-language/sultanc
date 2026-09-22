#ifndef SULTANC__FRONTEND_PARSER_TOKEN_PAYLOAD_H__
#define SULTANC__FRONTEND_PARSER_TOKEN_PAYLOAD_H__

#include "internal.h"

int __Parser_Current_Integer__(__Parser__ *__Parser_State__, __Integer_Literal__ *__Out_Value__);

int __Parser_Current_Text__(__Parser__ *__Parser_State__, __Text_Slice__ *__Out_Value__);

int __Parser_Current_Character__(__Parser__ *__Parser_State__, uint32_t *__Out_Value__);

int __Parser_Current_Boolean__(__Parser__ *__Parser_State__, int *__Out_Value__);

#endif
