/* Declares parser lifecycle operations. */

#ifndef SULTANC__FRONTEND_PARSER_LIFECYCLE_H__
#define SULTANC__FRONTEND_PARSER_LIFECYCLE_H__

#include "internal.h"

/* Initializes the parser. */
int __Parser_Init__(__Parser__ *__Parser_State__,
                    const __Source_File__ *__Source__,
                    __Ast_Context__ *__Ast__,
                    __Diagnostic__ *__Diagnostic_State__);

/* Releases the parser. */
void __Parser_Destroy__(__Parser__ *__Parser_State__);

#endif
