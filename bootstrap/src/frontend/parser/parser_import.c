/* Parses source and module imports. */

#include "frontend/parser/cursor.h"
#include "frontend/parser/token_payload.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/module_internal.h"
#include "support/text/equality.h"

#include <stdalign.h>

/* Appends the parser import bytes. */
static int __Parser_Append_Import_Bytes__(__Vector__ *__Bytes__, __Text_Slice__ __Text__)
{
    /* Tracks the index. */
    size_t __Index__;

    for (__Index__ = 0U; __Index__ < __Text__.__Length__; ++__Index__)
    {
        /* Stores the byte. */
        char __Byte__ = __Text__.__Data__[__Index__];
        if (__Vector_Push__(__Bytes__, &__Byte__) == NULL)
        {
            return 0;
        }
    }
    return 1;
}

/* Appends the parser import byte. */
static int __Parser_Append_Import_Byte__(__Vector__ *__Bytes__, char __Byte__)
{
    return __Vector_Push__(__Bytes__, &__Byte__) != NULL;
}

/* Parses the parser logical import. */
static int __Parser_Parse_Logical_Import__(__Parser__ *__Parser_State__, __Ast_Import__ *__Import__)
{
    /* Stores the bytes. */
    __Vector__ __Bytes__;
    /* References the frozen. */
    char *__Frozen__;

    __Vector_Init__(&__Bytes__, sizeof(char));
    while (1)
    {
        if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__ ||
            !__Parser_Append_Import_Bytes__(&__Bytes__, __Parser_State__->__Current__.__Lexeme__))
        {
            __Vector_Destroy__(&__Bytes__);
            return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Source_Or_Module__);
        }
        if (!__Parser_Advance__(__Parser_State__))
        {
            __Vector_Destroy__(&__Bytes__);
            return 0;
        }
        if (__Parser_State__->__Current__.__Kind__ != __Token_DOT_OPERATOR__)
        {
            break;
        }
        if (!__Parser_Append_Import_Byte__(&__Bytes__, '/') ||
            !__Parser_Advance__(__Parser_State__))
        {
            __Vector_Destroy__(&__Bytes__);
            return 0;
        }
        if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
        {
            __Vector_Destroy__(&__Bytes__);
            return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Source_Or_Module__);
        }
    }

    if (!__Parser_Append_Import_Byte__(&__Bytes__, '.') ||
        !__Parser_Append_Import_Byte__(&__Bytes__, 's') ||
        !__Parser_Append_Import_Byte__(&__Bytes__, 'n'))
    {
        __Vector_Destroy__(&__Bytes__);
        return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
    }

    __Frozen__ = (char *)__Parser_Freeze_Vector__(__Parser_State__, &__Bytes__, alignof(char));
    if (__Frozen__ == NULL)
    {
        __Vector_Destroy__(&__Bytes__);
        return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
    }
    __Import__->__Kind__ = __Ast_Import_Logical__;
    __Import__->__Path__.__Data__ = __Frozen__;
    __Import__->__Path__.__Length__ = __Bytes__.__Count__;
    __Vector_Destroy__(&__Bytes__);
    return 1;
}

/* Parses the parser import. */
int __Parser_Parse_Import__(__Parser__ *__Parser_State__, __Vector__ *__Imports__)
{
    /* Stores the import. */
    __Ast_Import__ __Import__ = {0};

    if (!__Parser_Advance__(__Parser_State__))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ == __Token_LIT_STR__)
    {
        /* Stores the literal text. */
        __Text_Slice__ __Literal_Text__;
        if (!__Parser_Current_Text__(__Parser_State__, &__Literal_Text__))
        {
            return 0;
        }
        __Import__.__Kind__ = __Ast_Import_Relative__;
        __Import__.__Path__ = __Parser_Copy_Literal_Text__(__Parser_State__, __Literal_Text__);
        if (!__Parser_Advance__(__Parser_State__))
        {
            return 0;
        }
    }
    else if (__Parser_State__->__Current__.__Kind__ == __Token_IDENTIFIER__)
    {
        if (!__Parser_Parse_Logical_Import__(__Parser_State__, &__Import__))
        {
            return 0;
        }
    }
    else
    {
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Source_Or_Module__);
    }
    if (!__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
    {
        return 0;
    }
    if (__Vector_Push__(__Imports__, &__Import__) == NULL)
    {
        return __Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
    }
    return 1;
}
