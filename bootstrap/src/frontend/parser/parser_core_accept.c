/* Consumes optional parser tokens. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/internal.h"

/* Accepts the parser. */
int __Parser_Accept__(__Parser__ *__Parser_State__, __Token_Kind__ __Kind__)
{
    if (__Parser_State__ == NULL || __Parser_State__->__Current__.__Kind__ != __Kind__)
    {
        return 0;
    }
    return __Parser_Advance__(__Parser_State__);
}
