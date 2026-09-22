#include "frontend/ast/context_lifecycle.h"
#include "frontend/parser/parser.h"

#include <string.h>

void __Parse_Result_Destroy__(__Parse_Result__ *__Result__)
{
    if (__Result__ == NULL)
    {
        return;
    }
    __Ast_Context_Destroy__(&__Result__->__Ast__);
    memset(&__Result__->__Module__, 0, sizeof(__Result__->__Module__));
    __Result__->__Failed__ = 0;
}
