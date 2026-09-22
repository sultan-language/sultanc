#include "frontend/parser/diagnostic.h"
#include "frontend/parser/module_internal.h"

int __Parser_Push_Module_Item__(__Parser__ *__Parser_State__,
                                __Vector__ *__Items__,
                                __Ast_Module_Item__ *__Item__)
{
    if (__Item__->__Name__.__Length__ != 0U &&
        __Parser_Module_Name_Exists__(__Items__, __Item__->__Name__))
    {
        __Diagnostic_Begin__(__Parser_State__->__Diagnostic__,
                             __E0205_Duplicate_Declaration__,
                             __Item__->__Header__.__Span__);
        __Parser_State__->__Failed__ = 1;
        return 0;
    }
    if (__Vector_Push__(__Items__, &__Item__) != NULL)
    {
        return 1;
    }
    return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
}
