/* Parses module item lists, including top-level target conditions.
 *
 * Canonical owner: compiler/frontend/parser/declarations.sn
 * (حلل_محتوى_وحدة_مشروطة). Platform declarations are restricted before name
 * resolution sees them: both arms of a top-level target condition are parsed
 * for syntax, but only the selected arm contributes imports and declarations
 * to the module. No conditional AST and no second symbol table exist; module
 * loading, name collection and lowering consume the already-selected items. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/module_internal.h"
#include "support/text/equality.h"

#include <stdalign.h>

/* Parses an item introduced by "خارجي": an external function or external data. */
static int __Parser_Parse_External_Module_Item__(__Parser__ *__Parser_State__,
                                                 __Vector__ *__Items__,
                                                 int __Public__,
                                                 __Source_Position__ __Start__)
{
    /* A single external modifier applies to a function or a typed data symbol. */
    if (__Parser_State__->__Next__.__Kind__ == __Token_LET__)
    {
        if (!__Parser_Advance__(__Parser_State__))
        {
            return 0;
        }
        return __Parser_Parse_Global_Item__(__Parser_State__, __Items__, __Public__, __Start__);
    }
    if (!__Parser_Advance__(__Parser_State__))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_FUNCTION_DEF__)
    {
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Module_Item__);
    }
    return __Parser_Parse_Function_Item__(__Parser_State__, __Items__, __Public__, 1);
}

/* Parses the parser public module item. */
static int __Parser_Parse_Public_Module_Item__(__Parser__ *__Parser_State__, __Vector__ *__Items__)
{
    /* Tracks the public start state. */
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
    if (__Parser_State__->__Current__.__Kind__ == __Token_EXTERN__)
    {
        return __Parser_Parse_External_Module_Item__(
            __Parser_State__, __Items__, 1, __Public_Start__);
    }
    if (__Parser_State__->__Current__.__Kind__ == __Token_FUNCTION_DEF__)
    {
        return __Parser_Parse_Function_Item__(__Parser_State__, __Items__, 1, 0);
    }
    return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Module_Item__);
}

static int __Parser_Parse_Module_Content__(__Parser__ *__Parser_State__,
                                           __Vector__ *__Items__,
                                           __Vector__ *__Imports__,
                                           int __Active__,
                                           int __Inside_Block__);

/* Parses one top-level target condition and its optional else arm. */
static int __Parser_Parse_Module_Condition__(__Parser__ *__Parser_State__,
                                             __Vector__ *__Items__,
                                             __Vector__ *__Imports__,
                                             int __Active__)
{
    /* Stores the selected predicate value. */
    int __Value__ = 0;

    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__) ||
        !__Parser_Parse_Target_Condition__(__Parser_State__, &__Value__) ||
        !__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__) ||
        !__Parser_Expect__(__Parser_State__, __Token_LEFT_BRACE_OPERATOR__))
    {
        return 0;
    }
    if (!__Parser_Parse_Module_Content__(
            __Parser_State__, __Items__, __Imports__, __Active__ && __Value__, 1))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_ELSE__)
    {
        return 1;
    }
    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_LEFT_BRACE_OPERATOR__))
    {
        return 0;
    }
    return __Parser_Parse_Module_Content__(
        __Parser_State__, __Items__, __Imports__, __Active__ && !__Value__, 1);
}

/* Parses module content until EOF (top level) or the closing brace of a target
 * condition arm. Inactive content is parsed into scratch storage that is
 * discarded, so it never reaches imports, name collection or lowering. */
static int __Parser_Parse_Module_Content__(__Parser__ *__Parser_State__,
                                           __Vector__ *__Items__,
                                           __Vector__ *__Imports__,
                                           int __Active__,
                                           int __Inside_Block__)
{
    /* Stores discarded items of an inactive arm. */
    __Vector__ __Inactive_Items__;
    /* Stores discarded imports of an inactive arm. */
    __Vector__ __Inactive_Imports__;
    /* References the item destination. */
    __Vector__ *__Item_Target__ = __Items__;
    /* References the import destination. */
    __Vector__ *__Import_Target__ = __Imports__;
    /* Tracks whether the operation succeeded. */
    int __Ok__ = 1;
    /* Tracks whether the closing brace was consumed. */
    int __Closed__ = 0;

    __Vector_Init__(&__Inactive_Items__, sizeof(__Ast_Module_Item__ *));
    __Vector_Init__(&__Inactive_Imports__, sizeof(__Ast_Import__));
    if (!__Active__)
    {
        __Item_Target__ = &__Inactive_Items__;
        __Import_Target__ = &__Inactive_Imports__;
    }

    while (__Ok__ && __Parser_State__->__Current__.__Kind__ != __Token_EOF__)
    {
        if (__Inside_Block__ &&
            __Parser_State__->__Current__.__Kind__ == __Token_RIGHT_BRACE_OPERATOR__)
        {
            __Ok__ = __Parser_Advance__(__Parser_State__);
            __Closed__ = 1;
            break;
        }
        switch (__Parser_State__->__Current__.__Kind__)
        {
            case __Token_IF__:
                /* Nested conditions keep the real destinations; the nested arm is
                 * active only when every enclosing arm is active. */
                __Ok__ = __Parser_Parse_Module_Condition__(
                    __Parser_State__, __Items__, __Imports__, __Active__);
                break;
            case __Token_IMPORT__:
                __Ok__ = __Parser_Parse_Import__(__Parser_State__, __Import_Target__);
                break;
            case __Token_ALIAS__:
                __Ok__ =
                    __Parser_Parse_Alias_Item__(__Parser_State__,
                                                __Item_Target__,
                                                __Parser_State__->__Current__.__Span__.__Start__,
                                                0);
                break;
            case __Token_TYPE__:
                __Ok__ = __Parser_Parse_Type_Item__(__Parser_State__, __Item_Target__, 0);
                break;
            case __Token_PUBLIC__:
                __Ok__ = __Parser_Parse_Public_Module_Item__(__Parser_State__, __Item_Target__);
                break;
            case __Token_EXTERN__:
                __Ok__ = __Parser_Parse_External_Module_Item__(
                    __Parser_State__,
                    __Item_Target__,
                    0,
                    __Parser_State__->__Current__.__Span__.__Start__);
                break;
            case __Token_FUNCTION_DEF__:
                __Ok__ = __Parser_Parse_Function_Item__(__Parser_State__, __Item_Target__, 0, 0);
                break;
            default:
                __Ok__ =
                    __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Module_Item__);
                break;
        }
    }
    if (__Ok__ && __Inside_Block__ && !__Closed__)
    {
        /* EOF inside a target condition arm: report the missing closing brace. */
        __Ok__ = __Parser_Expect__(__Parser_State__, __Token_RIGHT_BRACE_OPERATOR__);
    }
    __Vector_Destroy__(&__Inactive_Items__);
    __Vector_Destroy__(&__Inactive_Imports__);
    return __Ok__;
}

/* Parses the parser module items. */
int __Parser_Parse_Module_Items__(__Parser__ *__Parser_State__, __Ast_Module__ *__Out_Module__)
{
    /* Stores the items. */
    __Vector__ __Items__;
    /* Stores the imports. */
    __Vector__ __Imports__;

    __Vector_Init__(&__Items__, sizeof(__Ast_Module_Item__ *));
    __Vector_Init__(&__Imports__, sizeof(__Ast_Import__));
    if (!__Parser_Parse_Module_Content__(__Parser_State__, &__Items__, &__Imports__, 1, 0))
    {
        __Vector_Destroy__(&__Items__);
        __Vector_Destroy__(&__Imports__);
        return 0;
    }
    __Out_Module__->__Item_Count__ = __Items__.__Count__;
    __Out_Module__->__Items__ = (__Ast_Module_Item__ **)__Parser_Freeze_Vector__(
        __Parser_State__, &__Items__, alignof(__Ast_Module_Item__ *));
    __Out_Module__->__Import_Count__ = __Imports__.__Count__;
    __Out_Module__->__Imports__ = (__Ast_Import__ *)__Parser_Freeze_Vector__(
        __Parser_State__, &__Imports__, alignof(__Ast_Import__));
    __Vector_Destroy__(&__Items__);
    __Vector_Destroy__(&__Imports__);
    return 1;
}
