/* Declares escape-sequence scanning. */

#ifndef SULTANC__FRONTEND_LEXER_ESCAPE_H__
#define SULTANC__FRONTEND_LEXER_ESCAPE_H__

#include "cursor.h"
#include "support/text/builder.h"

/* Parses the lexer hex escape. */
int __Lexer_Parse_Hex_Escape__(__Lexer__ *__Lexer_State__,
                               size_t __Digits__,
                               uint32_t *__Out_Codepoint__);

/* Scans the lexer escape. */
int __Lexer_Scan_Escape__(__Lexer__ *__Lexer_State__,
                          __Text_Builder__ *__Builder__,
                          __Source_Position__ __Start__);

#endif
