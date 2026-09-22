#ifndef SULTANC__FRONTEND_LEXER_CURSOR_H__
#define SULTANC__FRONTEND_LEXER_CURSOR_H__

#include "frontend/lexer/token.h"
#include "lexer.h"

__Source_Position__ __Lexer_Position__(const __Lexer__ *__Lexer_State__);

int __Lexer_Has__(const __Lexer__ *__Lexer_State__, size_t __Count__);

unsigned char __Lexer_Peek__(const __Lexer__ *__Lexer_State__, size_t __Ahead__);

void __Lexer_Advance_Bytes__(__Lexer__ *__Lexer_State__, size_t __Count__);

int __Lexer_Matches__(const __Lexer__ *__Lexer_State__, const char *__Text__);

int __Lexer_Fail__(__Lexer__ *__Lexer_State__,
                   __Error_Id__ __Id__,
                   __Source_Position__ __Start__,
                   __Diagnostic_Wording_Key__ __Message_Key__);

int __Lexer_Set_Token__(__Lexer__ *__Lexer_State__,
                        __Token__ *__Token_State__,
                        __Token_Kind__ __Kind__,
                        __Source_Position__ __Start__,
                        size_t __Start_Offset__);

int __Lexer_Set_Token_Index__(__Lexer__ *__Lexer_State__,
                              __Token__ *__Token_State__,
                              __Token_Kind__ __Kind__,
                              __Source_Position__ __Start__,
                              size_t __Start_Offset__,
                              int64_t __Value__);

int __Lexer_Set_Token_Integer__(__Lexer__ *__Lexer_State__,
                                __Token__ *__Token_State__,
                                __Token_Kind__ __Kind__,
                                __Source_Position__ __Start__,
                                size_t __Start_Offset__,
                                __Integer_Literal__ __Value__);

int __Lexer_Set_Token_Text__(__Lexer__ *__Lexer_State__,
                             __Token__ *__Token_State__,
                             __Token_Kind__ __Kind__,
                             __Source_Position__ __Start__,
                             size_t __Start_Offset__,
                             __Text_Slice__ __Value__);

int __Lexer_Set_Token_Character__(__Lexer__ *__Lexer_State__,
                                  __Token__ *__Token_State__,
                                  __Token_Kind__ __Kind__,
                                  __Source_Position__ __Start__,
                                  size_t __Start_Offset__,
                                  uint32_t __Value__);

int __Lexer_Set_Token_Boolean__(__Lexer__ *__Lexer_State__,
                                __Token__ *__Token_State__,
                                __Token_Kind__ __Kind__,
                                __Source_Position__ __Start__,
                                size_t __Start_Offset__,
                                int __Value__);

#endif
