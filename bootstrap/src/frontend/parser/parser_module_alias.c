#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/module_internal.h"
#include "frontend/parser/span.h"
#include "frontend/identifier_identity.h"
#include "kernel/name/name.h"

#include <stddef.h>

typedef struct
{
    __Ast_Alias_Path__ __Path__;
    __Source_Span__ __Parent_Span__;
    __Source_Span__ __Name_Span__;
    int __Qualified__;
} __Parser_Alias_Path__;

static int __Parser_Alias_Diagnostic__(__Parser__ *__Parser_State__,
                                       __Error_Id__ __Error__,
                                       __Source_Span__ __Span__)
{
    __Diagnostic_Begin__(__Parser_State__->__Diagnostic__, __Error__, __Span__);
    __Parser_State__->__Failed__ = 1;
    return 0;
}

static int __Parser_Parse_Alias_Path__(__Parser__ *__Parser_State__,
                                       __Parser_Alias_Path__ *__Out_Path__)
{
    __Text_Slice__ __First__;
    __Source_Span__ __First_Span__;

    if (__Parser_State__ == NULL || __Out_Path__ == NULL)
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
    {
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Module_Item__);
    }

    __First__ = __Parser_State__->__Current__.__Lexeme__;
    __First_Span__ = __Parser_State__->__Current__.__Span__;
    __Out_Path__->__Path__.__Parent__ = (__Text_Slice__){0};
    __Out_Path__->__Path__.__Name__ = __First__;
    __Out_Path__->__Parent_Span__ = (__Source_Span__){0};
    __Out_Path__->__Name_Span__ = __First_Span__;
    __Out_Path__->__Qualified__ = 0;

    if (!__Parser_Advance__(__Parser_State__))
    {
        return 0;
    }
    if (!__Parser_Accept__(__Parser_State__, __Token_DOT_OPERATOR__))
    {
        return 1;
    }

    __Out_Path__->__Path__.__Parent__ = __First__;
    __Out_Path__->__Parent_Span__ = __First_Span__;
    __Out_Path__->__Qualified__ = 1;
    if (!__Parser_Token_Is_Contextual_Name__(&__Parser_State__->__Current__))
    {
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Module_Item__);
    }
    __Out_Path__->__Name_Span__ = __Parser_State__->__Current__.__Span__;
    return __Parser_Take_Contextual_Name__(__Parser_State__, &__Out_Path__->__Path__.__Name__);
}

/* Parser-time canonicalization mirrors the module-root alias chain. It only
 * inspects already parsed bindings, so source declaration ordering is kept. */
static const __Ast_Module_Item__ *__Parser_Resolve_Root_Binding__(const __Vector__ *__Items__,
                                                                  __Text_Slice__ __Name__)
{
    size_t __Depth__;
    __Text_Slice__ __Resolved__ = __Name__;

    if (__Items__ == NULL)
    {
        return NULL;
    }
    for (__Depth__ = 0U; __Depth__ <= __Items__->__Count__; ++__Depth__)
    {
        const __Ast_Module_Item__ *__Item__ =
            __Parser_Module_Find_Binding__(__Items__, __Resolved__);

        if (__Item__ == NULL)
        {
            return NULL;
        }
        if (__Item__->__Kind__ != __Ast_Module_Item_Alias__ ||
            __Item__->__As__.__Alias__.__Destination__.__Parent__.__Length__ != 0U)
        {
            return __Item__;
        }
        if (__Item__->__As__.__Alias__.__Target__.__Parent__.__Length__ != 0U)
        {
            return NULL;
        }
        __Resolved__ = __Item__->__As__.__Alias__.__Target__.__Name__;
    }
    return NULL;
}

static int __Parser_Find_Direct_Enum_Constructor__(const __Ast_Module_Item__ *__Parent__,
                                                   __Text_Slice__ __Name__,
                                                   size_t *__Out_Index__)
{
    const __Ast_Type_Declaration__ *__Type__;
    size_t __Index__;

    if (__Parent__ == NULL || __Parent__->__Kind__ != __Ast_Module_Item_Type__)
    {
        return 0;
    }
    __Type__ = __Parent__->__As__.__Type__;
    if (__Type__ == NULL || __Type__->__Kind__ != __Ast_Type_Decl_Enum__)
    {
        return 0;
    }
    for (__Index__ = 0U; __Index__ < __Type__->__As__.__Enum__.__Count__; ++__Index__)
    {
        if (__Identifier_Identity_Equals__(
                __Type__->__As__.__Enum__.__Constructors__[__Index__].__Name__, __Name__))
        {
            if (__Out_Index__ != NULL)
            {
                *__Out_Index__ = __Index__;
            }
            return 1;
        }
    }
    return 0;
}

/* Resolve a child spelling through prior child aliases to the one constructor
 * declaration owned by Parent. No constructor declaration or index is copied. */
static int __Parser_Resolve_Enum_Child__(const __Vector__ *__Items__,
                                         const __Ast_Module_Item__ *__Parent__,
                                         __Text_Slice__ __Name__,
                                         size_t *__Out_Index__)
{
    size_t __Depth__;
    __Text_Slice__ __Resolved__ = __Name__;

    if (__Items__ == NULL || __Parent__ == NULL)
    {
        return 0;
    }
    for (__Depth__ = 0U; __Depth__ <= __Items__->__Count__; ++__Depth__)
    {
        size_t __Index__;
        int __Found_Alias__ = 0;

        if (__Parser_Find_Direct_Enum_Constructor__(__Parent__, __Resolved__, __Out_Index__))
        {
            return 1;
        }
        for (__Index__ = 0U; __Index__ < __Items__->__Count__; ++__Index__)
        {
            __Ast_Module_Item__ *const *__Slot__ =
                (__Ast_Module_Item__ *const *)(__Items__->__Data__ +
                                               __Index__ * __Items__->__Element_Size__);
            const __Ast_Module_Item__ *__Item__;
            const __Ast_Module_Item__ *__Destination_Parent__;
            const __Ast_Module_Item__ *__Target_Parent__;

            if (__Slot__ == NULL || *__Slot__ == NULL)
            {
                continue;
            }
            __Item__ = *__Slot__;
            if (__Item__->__Kind__ != __Ast_Module_Item_Alias__ ||
                __Item__->__As__.__Alias__.__Destination__.__Parent__.__Length__ == 0U ||
                !__Identifier_Identity_Equals__(__Item__->__As__.__Alias__.__Destination__.__Name__,
                                                __Resolved__))
            {
                continue;
            }
            __Destination_Parent__ = __Parser_Resolve_Root_Binding__(
                __Items__, __Item__->__As__.__Alias__.__Destination__.__Parent__);
            if (__Destination_Parent__ != __Parent__)
            {
                continue;
            }
            __Target_Parent__ = __Parser_Resolve_Root_Binding__(
                __Items__, __Item__->__As__.__Alias__.__Target__.__Parent__);
            if (__Target_Parent__ != __Parent__)
            {
                return 0;
            }
            __Resolved__ = __Item__->__As__.__Alias__.__Target__.__Name__;
            __Found_Alias__ = 1;
            break;
        }
        if (!__Found_Alias__)
        {
            return 0;
        }
    }
    return 0;
}

int __Parser_Parse_Alias_Item__(__Parser__ *__Parser_State__,
                                __Vector__ *__Items__,
                                __Source_Position__ __Start__,
                                int __Public__)
{
    __Parser_Alias_Path__ __Destination__;
    __Parser_Alias_Path__ __Target__;
    const __Ast_Module_Item__ *__Target_Item__;
    __Ast_Module_Item__ *__Item__;

    if (!__Parser_Expect__(__Parser_State__, __Token_ALIAS__) ||
        !__Parser_Parse_Alias_Path__(__Parser_State__, &__Destination__) ||
        !__Parser_Expect__(__Parser_State__, __Token_DEFINITION_SEPARATOR__) ||
        !__Parser_Parse_Alias_Path__(__Parser_State__, &__Target__))
    {
        return 0;
    }

    if (__Destination__.__Qualified__ != __Target__.__Qualified__)
    {
        return __Parser_Alias_Diagnostic__(
            __Parser_State__, __E0304_Unsatisfied_Use__, __Target__.__Name_Span__);
    }

    if (!__Public__)
    {
        __Name_Builtin_Function__ __Builtin__;
        if (__Destination__.__Qualified__ ||
            __Identifier_Identity_Equals__(__Destination__.__Path__.__Name__,
                                           __Target__.__Path__.__Name__))
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0304_Unsatisfied_Use__, __Target__.__Name_Span__);
        }
        __Builtin__ = __Name_Find_Builtin_Function__(__Target__.__Path__.__Name__);
        if (__Builtin__ == __Name_Builtin_None__ || __Name_Builtin_Is_Direct_Source__(__Builtin__))
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0300_Unknown_Name__, __Target__.__Name_Span__);
        }
        if (!__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
        {
            return 0;
        }
        __Item__ = __Parser_New_Module_Item__(
            __Parser_State__,
            __Ast_Module_Item_Alias__,
            __Destination__.__Path__.__Name__,
            __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
        if (__Item__ == NULL)
        {
            return 0;
        }
        __Item__->__Public__ = 0;
        __Item__->__As__.__Alias__.__Destination__ = __Destination__.__Path__;
        __Item__->__As__.__Alias__.__Target__ = __Target__.__Path__;
        return __Parser_Push_Module_Item__(__Parser_State__, __Items__, __Item__);
    }

    if (!__Destination__.__Qualified__)
    {
        if (__Identifier_Identity_Equals__(__Destination__.__Path__.__Name__,
                                           __Target__.__Path__.__Name__))
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0205_Duplicate_Declaration__, __Destination__.__Name_Span__);
        }
        __Target_Item__ = __Parser_Module_Find_Binding__(__Items__, __Target__.__Path__.__Name__);
        if (__Target_Item__ == NULL)
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0300_Unknown_Name__, __Target__.__Name_Span__);
        }
        if (!__Target_Item__->__Public__)
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0309_Private_Declaration__, __Target__.__Name_Span__);
        }
        if (!__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
        {
            return 0;
        }
        __Item__ = __Parser_New_Module_Item__(
            __Parser_State__,
            __Ast_Module_Item_Alias__,
            __Destination__.__Path__.__Name__,
            __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
        if (__Item__ == NULL)
        {
            return 0;
        }
        __Item__->__Public__ = 1;
        __Item__->__As__.__Alias__.__Destination__ = __Destination__.__Path__;
        __Item__->__As__.__Alias__.__Target__ = __Target__.__Path__;
        return __Parser_Push_Module_Item__(__Parser_State__, __Items__, __Item__);
    }
    else
    {
        const __Ast_Module_Item__ *__Destination_Parent__ =
            __Parser_Resolve_Root_Binding__(__Items__, __Destination__.__Path__.__Parent__);
        const __Ast_Module_Item__ *__Target_Parent__ =
            __Parser_Resolve_Root_Binding__(__Items__, __Target__.__Path__.__Parent__);
        size_t __Target_Index__;

        if (__Destination_Parent__ == NULL)
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0300_Unknown_Name__, __Destination__.__Parent_Span__);
        }
        if (__Target_Parent__ == NULL)
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0300_Unknown_Name__, __Target__.__Parent_Span__);
        }
        if (!__Destination_Parent__->__Public__ || !__Target_Parent__->__Public__)
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0309_Private_Declaration__, __Target__.__Parent_Span__);
        }
        if (__Destination_Parent__ != __Target_Parent__)
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0304_Unsatisfied_Use__, __Target__.__Parent_Span__);
        }
        if (__Identifier_Identity_Equals__(__Destination__.__Path__.__Name__,
                                           __Target__.__Path__.__Name__))
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0205_Duplicate_Declaration__, __Destination__.__Name_Span__);
        }
        if (__Parser_Resolve_Enum_Child__(
                __Items__, __Destination_Parent__, __Destination__.__Path__.__Name__, NULL))
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0205_Duplicate_Declaration__, __Destination__.__Name_Span__);
        }
        if (!__Parser_Resolve_Enum_Child__(
                __Items__, __Target_Parent__, __Target__.__Path__.__Name__, &__Target_Index__))
        {
            return __Parser_Alias_Diagnostic__(
                __Parser_State__, __E0300_Unknown_Name__, __Target__.__Name_Span__);
        }
        (void)__Target_Index__;
        if (!__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
        {
            return 0;
        }
        __Item__ = __Parser_New_Module_Item__(
            __Parser_State__,
            __Ast_Module_Item_Alias__,
            (__Text_Slice__){0},
            __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__));
        if (__Item__ == NULL)
        {
            return 0;
        }
        __Item__->__Public__ = 1;
        __Item__->__As__.__Alias__.__Destination__ = __Destination__.__Path__;
        __Item__->__As__.__Alias__.__Target__ = __Target__.__Path__;
        return __Parser_Push_Module_Item__(__Parser_State__, __Items__, __Item__);
    }
}
