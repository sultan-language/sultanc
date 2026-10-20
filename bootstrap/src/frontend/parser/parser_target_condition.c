/* Parses compile-time target predicates for top-level target conditions.
 *
 * This mirrors the canonical predicate grammar owned by
 * compiler/frontend/parser/target_conditions.sn:
 *
 *   condition   := conjunction ( "||" conjunction )*
 *   conjunction := atom ( "&&" atom )*
 *   atom        := "!" atom | "(" condition ")" | "الهدف" "." field ("==" | "!=") text
 *
 * It is not an expression evaluator. Only the selected target descriptor can
 * satisfy a predicate; unknown fields, unclassified target facts and malformed
 * predicates are invalid conditions. Every atom is parsed even after the
 * result is already determined, exactly like the canonical parser. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/module_internal.h"
#include "frontend/parser/token_payload.h"
#include "frontend/identifier_identity.h"

#include <string.h>

/* Maps one canonical predicate field spelling onto the Stage0 target model. */
typedef struct
{
    /* Stores the canonical Arabic field spelling. */
    const char *__Spelling__;
    /* Stores the target-model field. */
    __Bootstrap_Target_Condition_Field__ __Field__;
} __Parser_Target_Condition_Field_Spelling__;

/* Lists the canonical fields Stage0's target model can answer. The canonical
 * fields الثلاثي, الواجهة_الثنائية, الدعم and مزود_التشغيل depend on the
 * self-hosted target registry and are rejected as invalid here (fail closed). */
static const __Parser_Target_Condition_Field_Spelling__ __Parser_Target_Condition_Fields__[] = {
    {"المنصة", __Bootstrap_Target_Condition_Platform__},
    {"البيئة", __Bootstrap_Target_Condition_Environment__},
    {"المعمارية", __Bootstrap_Target_Condition_Architecture__},
    {"عائلة_المنصة", __Bootstrap_Target_Condition_Platform_Family__},
    {"صيغة_الكائن", __Bootstrap_Target_Condition_Object_Format__},
    {"ترتيب_البايت", __Bootstrap_Target_Condition_Byte_Order__},
    {"عرض_المؤشر", __Bootstrap_Target_Condition_Pointer_Width__},
};

/* Records an invalid target condition at the current token. */
static int __Parser_Target_Condition_Invalid__(__Parser__ *__Parser_State__)
{
    if (__Parser_State__->__Failed__)
    {
        return 0;
    }
    return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Target_Condition_Invalid__);
}

/* Returns the selected target's spelling for the current field token. */
static const char *__Parser_Target_Condition_Field_Value__(__Parser__ *__Parser_State__)
{
    /* Tracks the field index. */
    size_t __Index__;

    if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
    {
        return NULL;
    }
    for (__Index__ = 0U; __Index__ < sizeof(__Parser_Target_Condition_Fields__) /
                                         sizeof(__Parser_Target_Condition_Fields__[0]);
         ++__Index__)
    {
        if (__Identifier_Identity_Equals_Cstr__(
                __Parser_State__->__Current__.__Lexeme__,
                __Parser_Target_Condition_Fields__[__Index__].__Spelling__))
        {
            return __Bootstrap_Target_Condition_Value__(
                __Parser_State__->__Target__, __Parser_Target_Condition_Fields__[__Index__].__Field__);
        }
    }
    return NULL;
}

/* Compares decoded literal bytes with one canonical spelling. */
static int __Parser_Target_Condition_Text_Equals__(__Text_Slice__ __Text__, const char *__Spelling__)
{
    /* Stores the spelling length. */
    size_t __Length__ = strlen(__Spelling__);

    return __Text__.__Length__ == __Length__ &&
           (__Length__ == 0U || memcmp(__Text__.__Data__, __Spelling__, __Length__) == 0);
}

static int __Parser_Parse_Target_Condition_Or__(__Parser__ *__Parser_State__, int *__Out_Value__);

/* Parses one predicate atom. */
static int __Parser_Parse_Target_Condition_Atom__(__Parser__ *__Parser_State__, int *__Out_Value__)
{
    /* Stores the selected target spelling. */
    const char *__Expected__;
    /* Tracks whether the comparison is negated. */
    int __Negated__;
    /* Stores the decoded comparison text. */
    __Text_Slice__ __Text__;
    /* Stores a nested value. */
    int __Inner__ = 0;

    __Text__.__Data__ = NULL;
    __Text__.__Length__ = 0U;
    if (__Parser_State__->__Current__.__Kind__ == __Token_NOT_OPERATOR__)
    {
        if (!__Parser_Advance__(__Parser_State__) ||
            !__Parser_Parse_Target_Condition_Atom__(__Parser_State__, &__Inner__))
        {
            return 0;
        }
        *__Out_Value__ = !__Inner__;
        return 1;
    }
    if (__Parser_State__->__Current__.__Kind__ == __Token_LEFT_PARENTHESIS_OPERATOR__)
    {
        if (!__Parser_Advance__(__Parser_State__) ||
            !__Parser_Parse_Target_Condition_Or__(__Parser_State__, &__Inner__) ||
            !__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__))
        {
            return 0;
        }
        *__Out_Value__ = __Inner__;
        return 1;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__ ||
        !__Identifier_Identity_Equals_Cstr__(__Parser_State__->__Current__.__Lexeme__, "الهدف"))
    {
        return __Parser_Target_Condition_Invalid__(__Parser_State__);
    }
    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_DOT_OPERATOR__))
    {
        return 0;
    }
    __Expected__ = __Parser_Target_Condition_Field_Value__(__Parser_State__);
    if (__Expected__ == NULL)
    {
        return __Parser_Target_Condition_Invalid__(__Parser_State__);
    }
    if (!__Parser_Advance__(__Parser_State__))
    {
        return 0;
    }
    __Negated__ = __Parser_State__->__Current__.__Kind__ == __Token_NOT_EQUAL_OPERATOR__;
    if (!__Negated__ && __Parser_State__->__Current__.__Kind__ != __Token_EQUALS_OPERATOR__)
    {
        return __Parser_Target_Condition_Invalid__(__Parser_State__);
    }
    if (!__Parser_Advance__(__Parser_State__))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_LIT_STR__)
    {
        return __Parser_Target_Condition_Invalid__(__Parser_State__);
    }
    if (!__Parser_Current_Text__(__Parser_State__, &__Text__))
    {
        return 0;
    }
    *__Out_Value__ = __Parser_Target_Condition_Text_Equals__(__Text__, __Expected__);
    if (__Negated__)
    {
        *__Out_Value__ = !*__Out_Value__;
    }
    return __Parser_Advance__(__Parser_State__);
}

/* Parses one predicate conjunction. */
static int __Parser_Parse_Target_Condition_And__(__Parser__ *__Parser_State__, int *__Out_Value__)
{
    /* Stores the accumulated value. */
    int __Value__ = 0;
    /* Stores the next atom value. */
    int __Next__ = 0;

    if (!__Parser_Parse_Target_Condition_Atom__(__Parser_State__, &__Value__))
    {
        return 0;
    }
    while (__Parser_State__->__Current__.__Kind__ == __Token_AND_AND_OPERATOR__)
    {
        if (!__Parser_Advance__(__Parser_State__) ||
            !__Parser_Parse_Target_Condition_Atom__(__Parser_State__, &__Next__))
        {
            return 0;
        }
        __Value__ = __Value__ && __Next__;
    }
    *__Out_Value__ = __Value__;
    return 1;
}

/* Parses one predicate disjunction. */
static int __Parser_Parse_Target_Condition_Or__(__Parser__ *__Parser_State__, int *__Out_Value__)
{
    /* Stores the accumulated value. */
    int __Value__ = 0;
    /* Stores the next conjunction value. */
    int __Next__ = 0;

    if (!__Parser_Parse_Target_Condition_And__(__Parser_State__, &__Value__))
    {
        return 0;
    }
    while (__Parser_State__->__Current__.__Kind__ == __Token_OR_OR_OPERATOR__)
    {
        if (!__Parser_Advance__(__Parser_State__) ||
            !__Parser_Parse_Target_Condition_And__(__Parser_State__, &__Next__))
        {
            return 0;
        }
        __Value__ = __Value__ || __Next__;
    }
    *__Out_Value__ = __Value__;
    return 1;
}

/* Parses one target predicate after the opening parenthesis of a top-level
 * target condition. The caller owns the surrounding parentheses. */
int __Parser_Parse_Target_Condition__(__Parser__ *__Parser_State__, int *__Out_Value__)
{
    if (__Parser_State__ == NULL || __Out_Value__ == NULL)
    {
        return 0;
    }
    if (__Parser_State__->__Target__ == NULL)
    {
        /* A target conditional without a selected target cannot silently select
         * either implementation (canonical شرط_هدف_بلا_هدف). */
        return __Parser_Fail__(__Parser_State__,
                               __Diag_Word_Syntax_Target_Condition_Without_Target__);
    }
    return __Parser_Parse_Target_Condition_Or__(__Parser_State__, __Out_Value__);
}
