#include "frontend/lexer/keyword.h"
#include "frontend/identifier_identity.h"
#include "core/source.h"

typedef struct
{
    const char *__Alias__;
    __Token_Kind__ __Kind__;
    int __Boolean_Value__;
} __Keyword_Entry__;

#define SULTANC__KEYWORD_ENTRY__(Alias, Kind) {Alias, __Token_##Kind##__, 0}
#define SULTANC__BOOL_KEYWORD_ENTRY__(Alias, Value) {Alias, __Token_LIT_BOOL__, Value}

static const __Keyword_Entry__ __Keyword_Entries__[] = {
#define SULTANC__TOKEN__(Name, Selfhost, Class, Lifecycle, Payload)
#define SULTANC__KEYWORD__(Name, English, Arabic)                                                  \
    SULTANC__KEYWORD_ENTRY__(English, Name), SULTANC__KEYWORD_ENTRY__(Arabic, Name),
#define SULTANC__BOOL_KEYWORD__(English, Arabic, Value)                                            \
    SULTANC__BOOL_KEYWORD_ENTRY__(English, Value), SULTANC__BOOL_KEYWORD_ENTRY__(Arabic, Value),
#define SULTANC__OPERATOR__(Name, Spelling)
#include "frontend/lexer/token_registry.def"
#undef SULTANC__TOKEN__
#undef SULTANC__KEYWORD__
#undef SULTANC__BOOL_KEYWORD__
#undef SULTANC__OPERATOR__
};

#undef SULTANC__KEYWORD_ENTRY__
#undef SULTANC__BOOL_KEYWORD_ENTRY__

static size_t __Keyword_Alias_Count__(void)
{
    return sizeof(__Keyword_Entries__) / sizeof(__Keyword_Entries__[0]);
}

__Keyword_Result__ __Keyword_Lookup__(__Text_Slice__ __Text__)
{
    __Keyword_Result__ __Result__ = {0};
    size_t __Index__ = 0U;
    for (__Index__ = 0U; __Index__ < __Keyword_Alias_Count__(); ++__Index__)
    {
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
