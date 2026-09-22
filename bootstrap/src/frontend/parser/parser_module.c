#include "frontend/ast/context_lifecycle.h"
#include "frontend/parser/lifecycle.h"
#include "frontend/parser/module_internal.h"

#include <string.h>

int __Parser_Parse_Module__(const __Source_File__ *__Source__, __Parse_Result__ *__Out_Result__)
{
    __Parser__ __Parser_State__;
    if (__Source__ == NULL || __Out_Result__ == NULL)
    {
        return 0;
    }
    memset(__Out_Result__, 0, sizeof(*__Out_Result__));
    __Ast_Context_Init__(&__Out_Result__->__Ast__);
    if (!__Parser_Init__(&__Parser_State__,
                         __Source__,
                         &__Out_Result__->__Ast__,
                         &__Out_Result__->__Diagnostic__))
    {
        __Out_Result__->__Failed__ = 1;
        __Parser_Destroy__(&__Parser_State__);
        return 0;
    }
    if (!__Parser_Parse_Module_Items__(&__Parser_State__, &__Out_Result__->__Module__))
    {
        __Out_Result__->__Failed__ = 1;
        __Parser_Destroy__(&__Parser_State__);
        return 0;
    }
    __Parser_Destroy__(&__Parser_State__);
    return 1;
}
