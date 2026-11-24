/* Parses generic type parameter lists on declarations. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/module_internal.h"
#include "frontend/parser/storage.h"

#include <stdalign.h>

/* Parses an optional declaration type-parameter list. */
int __Parser_Parse_Type_Parameters__(__Parser__ *__Parser_State__,
                                     __Text_Slice__ **__Out_Parameters__,
                                     size_t *__Out_Count__)
{
    __Vector__ __Parameters__;

    if (__Out_Parameters__ == NULL || __Out_Count__ == NULL)
    {
        return 0;
    }
    *__Out_Parameters__ = NULL;
    *__Out_Count__ = 0U;
    if (!__Parser_Accept__(__Parser_State__, __Token_LESS_THAN_OPERATOR__))
    {
        return 1;
    }

    __Vector_Init__(&__Parameters__, sizeof(__Text_Slice__));
    if (__Parser_State__->__Current__.__Kind__ == __Token_GREATER_THAN_OPERATOR__)
    {
        __Vector_Destroy__(&__Parameters__);
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Type_Name__);
    }

    for (;;)
    {
        __Text_Slice__ __Name__;
        if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
        {
            __Vector_Destroy__(&__Parameters__);
            return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Type_Name__);
        }
        __Name__ = __Parser_State__->__Current__.__Lexeme__;
        if (__Vector_Push__(&__Parameters__, &__Name__) == NULL)
        {
            __Vector_Destroy__(&__Parameters__);
            return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
        }
        if (!__Parser_Advance__(__Parser_State__))
        {
            __Vector_Destroy__(&__Parameters__);
            return 0;
        }
        if (!__Parser_Accept__(__Parser_State__, __Token_COMMA_OPERATOR__))
        {
            break;
        }
    }
    if (!__Parser_Expect__(__Parser_State__, __Token_GREATER_THAN_OPERATOR__))
    {
        __Vector_Destroy__(&__Parameters__);
        return 0;
    }
    *__Out_Count__ = __Parameters__.__Count__;
    *__Out_Parameters__ = (__Text_Slice__ *)__Parser_Freeze_Vector__(
        __Parser_State__, &__Parameters__, alignof(__Text_Slice__));
    __Vector_Destroy__(&__Parameters__);
    return *__Out_Count__ == 0U || *__Out_Parameters__ != NULL;
}
