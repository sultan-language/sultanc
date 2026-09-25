/* Scans operators and delimiters. */

#include "frontend/lexer/operator.h"

#include <string.h>

/* Defines the operator entry structure. */
typedef struct
{
    /* References the spelling. */
    const char *__Spelling__;
    /* Stores the kind. */
    __Token_Kind__ __Kind__;
} __Operator_Entry__;

/* Stores the operators. */
static const __Operator_Entry__ __Operators__[] = {
/* Expands the token macro. */
#define SULTANC__TOKEN__(Name, Selfhost, Class, Lifecycle, Payload)
/* Expands the keyword macro. */
#define SULTANC__KEYWORD__(Name, English, Arabic)
/* Expands the bool keyword macro. */
#define SULTANC__BOOL_KEYWORD__(English, Arabic, Value)
/* Expands the operator macro. */
#define SULTANC__OPERATOR__(Name, Spelling) {Spelling, __Token_##Name##__},
#include "frontend/lexer/token_registry.def"
#undef SULTANC__TOKEN__
#undef SULTANC__KEYWORD__
#undef SULTANC__BOOL_KEYWORD__
#undef SULTANC__OPERATOR__
};

/* Scans the lexer operator. */
int __Lexer_Scan_Operator__(__Lexer__ *__Lexer_State__,
                            __Token__ *__Out_Token__,
                            __Source_Position__ __Start__,
                            size_t __Start_Offset__)
{
    /* Tracks the index. */
    size_t __Index__;

    for (__Index__ = 0U; __Index__ < sizeof(__Operators__) / sizeof(__Operators__[0]); ++__Index__)
    {
        if (__Lexer_Matches__(__Lexer_State__, __Operators__[__Index__].__Spelling__))
        {
            /* Stores the length. */
            size_t __Length__ = strlen(__Operators__[__Index__].__Spelling__);

            __Lexer_Advance_Bytes__(__Lexer_State__, __Length__);
            return __Lexer_Set_Token__(__Lexer_State__,
                                       __Out_Token__,
                                       __Operators__[__Index__].__Kind__,
                                       __Start__,
                                       __Start_Offset__);
        }
    }
    return 0;
}
