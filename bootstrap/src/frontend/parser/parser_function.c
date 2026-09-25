/* Parses function declarations. */

#include "frontend/ast/storage.h"
#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/type_internal.h"

#include <stdalign.h>
#include <string.h>

/* Parses the parser function. */
__Ast_Function__ *__Parser_Parse_Function__(__Parser__ *__Parser_State__,
                                            __Text_Slice__ __Name__,
                                            int __Public__,
                                            __Source_Position__ __Start__)
{
    /* Stores the parameters. */
    __Vector__ __Parameters__;
    /* References the output type. */
    __Ast_Type__ *__Output_Type__ = NULL;
    /* References the body. */
    __Ast_Block__ *__Body__ = NULL;
    /* References the function. */
    __Ast_Function__ *__Function__ = NULL;

    __Vector_Init__(&__Parameters__, sizeof(__Ast_Function_Parameter__));
    if (!__Parser_Expect__(__Parser_State__, __Token_LEFT_PARENTHESIS_OPERATOR__))
    {
        __Vector_Destroy__(&__Parameters__);
        return NULL;
    }
    while (__Parser_State__->__Current__.__Kind__ != __Token_RIGHT_PARENTHESIS_OPERATOR__)
    {
        /* Stores the parameter. */
        __Ast_Function_Parameter__ __Parameter__;
        /* Stores the parameter start. */
        __Source_Position__ __Parameter_Start__;
        /* References the parameter type. */
        __Ast_Type__ *__Parameter_Type__ = NULL;

        if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
        {
            __Vector_Destroy__(&__Parameters__);
            (void)__Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Parameter_Name__);
            return NULL;
        }
        __Parameter_Start__ = __Parser_State__->__Current__.__Span__.__Start__;
        __Parameter__.__Name__ = __Parser_State__->__Current__.__Lexeme__;
        if (!__Parser_Advance__(__Parser_State__) ||
            !__Parser_Expect__(__Parser_State__, __Token_COLON_OPERATOR__))
        {
            __Vector_Destroy__(&__Parameters__);
            return NULL;
        }
        __Parameter_Type__ = __Parser_Parse_Type__(__Parser_State__);
        if (__Parameter_Type__ == NULL)
        {
            __Vector_Destroy__(&__Parameters__);
            return NULL;
        }
        __Parameter__.__Slot__ = __Parser_Exterior_Slot__(
            __Parser_State__,
            __Parameter_Type__,
            __Parser_Span__(__Parameter_Start__, __Parser_State__->__Previous__.__Span__.__End__));
        if (__Vector_Push__(&__Parameters__, &__Parameter__) == NULL)
        {
            __Vector_Destroy__(&__Parameters__);
            (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
            return NULL;
        }
        if (!__Parser_Accept__(__Parser_State__, __Token_COMMA_OPERATOR__))
        {
            break;
        }
    }
    if (!__Parser_Expect__(__Parser_State__, __Token_RIGHT_PARENTHESIS_OPERATOR__) ||
        !__Parser_Expect__(__Parser_State__, __Token_COLON_OPERATOR__))
    {
        __Vector_Destroy__(&__Parameters__);
        return NULL;
    }

    {
        /* Stores the output start. */
        __Source_Position__ __Output_Start__ = __Parser_State__->__Current__.__Span__.__Start__;
        /* Stores the output end. */
        __Source_Position__ __Output_End__;

        __Output_Type__ = __Parser_Parse_Type__(__Parser_State__);
        if (__Output_Type__ == NULL)
        {
            __Vector_Destroy__(&__Parameters__);
            return NULL;
        }
        __Output_End__ = __Parser_State__->__Previous__.__Span__.__End__;
        __Body__ = __Parser_Parse_Block__(__Parser_State__);
        if (__Body__ == NULL)
        {
            __Vector_Destroy__(&__Parameters__);
            return NULL;
        }
        __Function__ = (__Ast_Function__ *)__Ast_Allocate__(
            __Parser_State__->__Ast__, sizeof(*__Function__), alignof(__Ast_Function__));
        if (__Function__ == NULL)
        {
            __Vector_Destroy__(&__Parameters__);
            (void)__Parser_Fail_Internal__(__Parser_State__, __Diag_Word_Syntax_Internal_Oom__);
            return NULL;
        }
        memset(__Function__, 0, sizeof(*__Function__));
        __Function__->__Header__ =
            __Ast_New_Header__(__Parser_State__->__Ast__,
                               __Parser_Span__(__Start__, __Body__->__Header__.__Span__.__End__));
        __Function__->__Name__ = __Name__;
        __Function__->__Public__ = __Public__;
        __Function__->__Parameter_Count__ = __Parameters__.__Count__;
        __Function__->__Parameters__ = (__Ast_Function_Parameter__ *)__Parser_Freeze_Vector__(
            __Parser_State__, &__Parameters__, alignof(__Ast_Function_Parameter__));
        __Function__->__Output__ = __Parser_Exterior_Slot__(
            __Parser_State__, __Output_Type__, __Parser_Span__(__Output_Start__, __Output_End__));
        __Function__->__Body__ = __Body__;
    }

    __Vector_Destroy__(&__Parameters__);
    return __Function__;
}
