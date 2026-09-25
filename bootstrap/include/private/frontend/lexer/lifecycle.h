/* Declares lexer lifecycle operations. */

#ifndef SULTANC__FRONTEND_LEXER_LIFECYCLE_H__
#define SULTANC__FRONTEND_LEXER_LIFECYCLE_H__

#include "lexer.h"

/* Initializes the lexer. */
void __Lexer_Init__(__Lexer__ *__Lexer__, const __Source_File__ *__Source__);

/* Releases the lexer. */
void __Lexer_Destroy__(__Lexer__ *__Lexer__);

/* Returns the lexer diagnostic. */
const __Diagnostic__ *__Lexer_Diagnostic__(const __Lexer__ *__Lexer__);

#endif
