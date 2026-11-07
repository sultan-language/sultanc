/* Identifies calls of the never-returning stop primitive (إنهيار). */

#include "semantic/never_return.h"
#include "frontend/identifier_identity.h"
#include "kernel/name/name.h"
#include "support/path/source_path.h"

#include <stdlib.h>
#include <string.h>

/* Returns the canonical path of std's process/panic.sn, or NULL. */
static const char *__Never_Return_Module_Path__(__Semantic_Context__ *__Context__)
{
    static const char __Relative__[] = "library/std/process/panic.sn";
    if (!__Context__->__Never_Return_Module_Resolved__)
    {
        __Text_Slice__ __Relative_Text__ = {__Relative__, sizeof(__Relative__) - 1U};
        char *__Joined__ = NULL;
        __Context__->__Never_Return_Module_Resolved__ = 1;
        if (__Context__->__Program__ == NULL || __Context__->__Program__->__Project_Root__ == NULL)
        {
            return NULL;
        }
        __Joined__ = __Path_Join_Text__(__Context__->__Program__->__Project_Root__,
                                        __Relative_Text__);
        if (__Joined__ == NULL)
        {
            return NULL;
        }
        __Context__->__Never_Return_Module_Path__ = __Source_Path_Canonical__(__Joined__);
        free(__Joined__);
    }
    return __Context__->__Never_Return_Module_Path__;
}

int __Semantic_Call_Never_Returns__(__Semantic_Context__ *__Context__,
                                    const __Program_Unit__ *__From_Unit__,
                                    const __Ast_Expression__ *__Expression__)
{
    const __Ast_Lvalue__ *__Function__;
    __Semantic_Function_Entry__ *__Entry__ = NULL;
    const char *__Module_Path__;

    if (__Context__ == NULL || __From_Unit__ == NULL || __Expression__ == NULL ||
        __Expression__->__Kind__ != __Ast_Expression_Call__)
    {
        return 0;
    }
    __Function__ = __Expression__->__As__.__Call__.__Function__;
    if (__Function__ == NULL || __Function__->__Kind__ != __Ast_Lvalue_Base__ ||
        __Function__->__As__.__Base__.__Kind__ != __Ast_Lvalue_Base_Identifier__)
    {
        return 0;
    }
    if (__Name_Resolve_Function__(__Context__, __From_Unit__,
                                  __Function__->__As__.__Base__.__As__.__Identifier__,
                                  &__Entry__) != __Name_Lookup_Found__ ||
        __Entry__ == NULL || __Entry__->__Function__ == NULL || __Entry__->__Unit__ == NULL ||
        __Entry__->__Unit__->__Path__ == NULL ||
        !__Identifier_Identity_Equals_Cstr__(__Entry__->__Function__->__Name__, "إنهيار"))
    {
        return 0;
    }
    __Module_Path__ = __Never_Return_Module_Path__(__Context__);
    return __Module_Path__ != NULL && strcmp(__Module_Path__, __Entry__->__Unit__->__Path__) == 0;
}
