/* Looks up bilingual keyword spellings. */

#include "frontend/lexer/keyword.h"
#include "frontend/identifier_identity.h"
#include "core/source.h"

/* Defines the keyword entry structure. */
typedef struct
{
    /* References the alias. */
    const char *__Alias__;
    /* Stores the kind. */
    __Token_Kind__ __Kind__;
    /* Stores the boolean value. */
    int __Boolean_Value__;
} __Keyword_Entry__;

/* Expands the keyword entry macro. */
#define SULTANC__KEYWORD_ENTRY__(Alias, Kind) {Alias, __Token_##Kind##__, 0}
/* Expands the bool keyword entry macro. */
#define SULTANC__BOOL_KEYWORD_ENTRY__(Alias, Value) {Alias, __Token_LIT_BOOL__, Value}

/* Stores the keyword entries. */
static const __Keyword_Entry__ __Keyword_Entries__[] = {
/* Expands the token macro. */
#define SULTANC__TOKEN__(Name, Selfhost, Class, Lifecycle, Payload)
/* Expands the keyword macro. */
#define SULTANC__KEYWORD__(Name, English, Arabic)                                                  \
    SULTANC__KEYWORD_ENTRY__(English, Name), SULTANC__KEYWORD_ENTRY__(Arabic, Name),
/* Expands the bool keyword macro. */
#define SULTANC__BOOL_KEYWORD__(English, Arabic, Value)                                            \
    SULTANC__BOOL_KEYWORD_ENTRY__(English, Value), SULTANC__BOOL_KEYWORD_ENTRY__(Arabic, Value),
/* Expands the operator macro. */
#define SULTANC__OPERATOR__(Name, Spelling)
#include "frontend/lexer/token_registry.def"
#undef SULTANC__TOKEN__
#undef SULTANC__KEYWORD__
#undef SULTANC__BOOL_KEYWORD__
#undef SULTANC__OPERATOR__
};

#undef SULTANC__KEYWORD_ENTRY__
#undef SULTANC__BOOL_KEYWORD_ENTRY__

/* Returns the keyword alias count. */
static size_t __Keyword_Alias_Count__(void)
{
    return sizeof(__Keyword_Entries__) / sizeof(__Keyword_Entries__[0]);
}

/* Finds the keyword. */
__Keyword_Result__ __Keyword_Lookup__(__Text_Slice__ __Text__)
{
    /* Stores the operation result. */
    __Keyword_Result__ __Result__ = {0};
    /* Tracks the index. */
    size_t __Index__ = 0U;
    for (__Index__ = 0U; __Index__ < __Keyword_Alias_Count__(); ++__Index__)
    {
        /* References the entry. */
        const __Keyword_Entry__ *__Entry__ = &__Keyword_Entries__[__Index__];
        if (__Identifier_Identity_Equals_Cstr__(__Text__, __Entry__->__Alias__))
        {
            __Result__.__Found__ = 1;
            __Result__.__Kind__ = __Entry__->__Kind__;
            __Result__.__Boolean_Value__ = __Entry__->__Boolean_Value__;
            return __Result__;
        }
    }
    return __Result__;
}
