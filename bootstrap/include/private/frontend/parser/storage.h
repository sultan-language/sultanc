/* Declares parser storage helpers. */

#ifndef SULTANC__FRONTEND_PARSER_STORAGE_H__
#define SULTANC__FRONTEND_PARSER_STORAGE_H__

#include "internal.h"

/* Returns the parser freeze vector. */
void *__Parser_Freeze_Vector__(__Parser__ *__Parser_State__,
                               const __Vector__ *__Vector_State__,
                               size_t __Alignment__);

/* Copies the parser literal text. */
__Text_Slice__ __Parser_Copy_Literal_Text__(__Parser__ *__Parser_State__, __Text_Slice__ __Text__);

#endif
