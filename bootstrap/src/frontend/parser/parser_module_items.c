#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/module_internal.h"
#include "support/text/equality.h"

#include <stdalign.h>

static int __Parser_Parse_Public_Module_Item__(__Parser__ *__Parser_State__, __Vector__ *__Items__)
{
    __Source_Position__ __Public_Start__ = __Parser_State__->__Current__.__Span__.__Start__;

    if (!__Parser_Advance__(__Parser_State__))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ == __Token_ALIAS__)
    {
        return __Parser_Parse_Alias_Item__(__Parser_State__, __Items__, __Public_Start__, 1);
    }
    if (__Parser_State__->__Current__.__Kind__ == __Token_TYPE__)
    {
        return __Parser_Parse_Type_Item__(__Parser_State__, __Items__, 1);
    }
    if (__Parser_State__->__Current__.__Kind__ == __Token_FUNCTION_DEF__)
    {
        return __Parser_Parse_Function_Item__(__Parser_State__, __Items__, 1);
    }
    return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Module_Item__);
}

int __Parser_Parse_Module_Items__(__Parser__ *__Parser_State__, __Ast_Module__ *__Out_Module__)
{
    __Vector__ __Items__;
    __Vector__ __Imports__;

    __Vector_Init__(&__Items__, sizeof(__Ast_Module_Item__ *));
    __Vector_Init__(&__Imports__, sizeof(__Text_Slice__));
    while (__Parser_State__->__Current__.__Kind__ != __Token_EOF__)
    {
        int __Ok__ = 0;
        switch (__Parser_State__->__Current__.__Kind__)
        {
            case __Token_IMPORT__:
                __Ok__ = __Parser_Parse_Import__(__Parser_State__, &__Imports__);
                break;
            case __Token_ALIAS__:
                __Ok__ =
                    __Parser_Parse_Alias_Item__(__Parser_State__,
                                                &__Items__,
                                                __Parser_State__->__Current__.__Span__.__Start__,
                                                0);
                break;
            case __Token_TYPE__:
                __Ok__ = __Parser_Parse_Type_Item__(__Parser_State__, &__Items__, 0);
                break;
            case __Token_PUBLIC__:
                __Ok__ = __Parser_Parse_Public_Module_Item__(__Parser_State__, &__Items__);
                break;
            case __Token_FUNCTION_DEF__:
                __Ok__ = __Parser_Parse_Function_Item__(__Parser_State__, &__Items__, 0);
                break;
            default:
                __Ok__ =
                    __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Module_Item__);
                break;
        }
        if (!__Ok__)
        {
            __Vector_Destroy__(&__Items__);
            __Vector_Destroy__(&__Imports__);
            return 0;
        }
    }
    __Out_Module__->__Item_Count__ = __Items__.__Count__;
    __Out_Module__->__Items__ = (__Ast_Module_Item__ **)__Parser_Freeze_Vector__(
        __Parser_State__, &__Items__, alignof(__Ast_Module_Item__ *));
    __Out_Module__->__Import_Count__ = __Imports__.__Count__;
    __Out_Module__->__Imports__ = (__Text_Slice__ *)__Parser_Freeze_Vector__(
        __Parser_State__, &__Imports__, alignof(__Text_Slice__));
    __Vector_Destroy__(&__Items__);
    __Vector_Destroy__(&__Imports__);
    return 1;
}
