/* Tracks names declared in a module. */

#include "frontend/parser/module_internal.h"
#include "frontend/identifier_identity.h"

/* Finds the parser module binding. */
const __Ast_Module_Item__ *__Parser_Module_Find_Binding__(const __Vector__ *__Items__,
                                                          __Text_Slice__ __Name__)
{
    /* Tracks the index. */
    size_t __Index__;

    for (__Index__ = 0U; __Index__ < __Items__->__Count__; ++__Index__)
    {
        /* References the item. */
        __Ast_Module_Item__ *const *__Item__ =
            (__Ast_Module_Item__ *const *)(__Items__->__Data__ +
                                           __Index__ * __Items__->__Element_Size__);
        if (__Item__ != NULL && *__Item__ != NULL &&
            __Identifier_Identity_Equals__((*__Item__)->__Name__, __Name__))
        {
            return *__Item__;
        }
    }
    return NULL;
}

/* Returns the parser module name exists. */
int __Parser_Module_Name_Exists__(const __Vector__ *__Items__, __Text_Slice__ __Name__)
{
    return __Parser_Module_Find_Binding__(__Items__, __Name__) != NULL;
}
