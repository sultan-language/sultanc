/* Parses wrapped and composite types. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/type_internal.h"

/* Parses the parser wrapped type. */
__Ast_Type__ *__Parser_Parse_Wrapped_Type__(__Parser__ *__Parser_State__,
                                            __Ast_Type_Kind__ __Kind__)
{
    /* References the outer. */
    __Ast_Type__ *__Outer__ = NULL;
    if (!__Parser_Advance__(__Parser_State__))
    {
        return NULL;
    }
    __Outer__ = __Parser_New_Type__(__Parser_State__, __Kind__);
    if (__Outer__ == NULL)
    {
        return NULL;
    }
    __Outer__->__As__.__Inner__ = __Parser_Parse_Type__(__Parser_State__);
    if (__Outer__->__As__.__Inner__ == NULL)
    {
        return NULL;
    }
    return __Outer__;
}
