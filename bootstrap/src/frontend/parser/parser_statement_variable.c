/* Parses local variable declarations. */

#include "frontend/ast/storage.h"
#include "frontend/parser/cursor.h"
#include "frontend/parser/diagnostic.h"
#include "frontend/parser/span.h"
#include "frontend/parser/statement_internal.h"

/* Creates the parser variable declaration. */
static __Ast_Statement__ *__Parser_Create_Variable_Declaration__(__Parser__ *__Parser_State__,
                                                                 __Text_Slice__ __Name__,
                                                                 __Ast_Type__ *__Type__,
                                                                 __Source_Span__ __Span__)
{
    /* References the declaration. */
    __Ast_Statement__ *__Declaration__ = __Parser_New_Statement__(
        __Parser_State__, __Ast_Statement_Variable_Declaration__, __Span__);

    if (__Declaration__ == NULL)
    {
        return NULL;
    }

    __Declaration__->__As__.__Variable__.__Name_Kind__ = __Ast_Lvalue_Base_Identifier__;
    __Declaration__->__As__.__Variable__.__Name__.__Identifier__ = __Name__;
    __Declaration__->__As__.__Variable__.__Slot__.__Header__ =
        __Ast_New_Header__(__Parser_State__->__Ast__, __Span__);
    __Declaration__->__As__.__Variable__.__Slot__.__Type__ = __Type__;
    return __Declaration__;
}

/* Parses the parser variable. */
int __Parser_Parse_Variable__(__Parser__ *__Parser_State__, __Vector__ *__Statements__)
{
    /* Stores the start. */
    __Source_Position__ __Start__ = __Parser_State__->__Current__.__Span__.__Start__;
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* References the type. */
    __Ast_Type__ *__Type__ = NULL;
    /* Stores the initializer value. */
    __Initializer__ __Initializer_Value__;
    /* Stores the initializer ok. */
    int __Initializer_Ok__ = 0;
    /* References the declaration. */
    __Ast_Statement__ *__Declaration__ = NULL;
    /* References the initialization. */
    __Ast_Statement__ *__Initialization__ = NULL;
    /* References the destination. */
    __Ast_Lvalue__ *__Destination__ = NULL;
    /* Stores the span. */
    __Source_Span__ __Span__;

    if (!__Parser_Advance__(__Parser_State__))
    {
        return 0;
    }
    if (__Parser_State__->__Current__.__Kind__ != __Token_IDENTIFIER__)
    {
        return __Parser_Fail__(__Parser_State__, __Diag_Word_Syntax_Expected_Variable_Name__);
    }

    __Name__ = __Parser_State__->__Current__.__Lexeme__;
    if (!__Parser_Advance__(__Parser_State__) ||
        !__Parser_Expect__(__Parser_State__, __Token_COLON_OPERATOR__))
    {
        return 0;
    }

    __Type__ = __Parser_Parse_Type__(__Parser_State__);
    if (__Type__ == NULL)
    {
        return 0;
    }

    if (__Parser_State__->__Current__.__Kind__ == __Token_SEMICOLON_OPERATOR__)
    {
        if (!__Parser_Advance__(__Parser_State__))
        {
            return 0;
        }
        __Span__ = __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__);
        __Declaration__ =
            __Parser_Create_Variable_Declaration__(__Parser_State__, __Name__, __Type__, __Span__);
        return __Declaration__ != NULL &&
               __Parser_Push_Statement__(__Parser_State__, __Statements__, __Declaration__);
    }

    if (!__Parser_Expect__(__Parser_State__, __Token_DEFINITION_SEPARATOR__))
    {
        return 0;
    }

    __Initializer_Value__ = __Parser_Parse_Initializer__(__Parser_State__, &__Initializer_Ok__);
    if (!__Initializer_Ok__ || !__Parser_Expect__(__Parser_State__, __Token_SEMICOLON_OPERATOR__))
    {
        return 0;
    }

    __Span__ = __Parser_Span__(__Start__, __Parser_State__->__Previous__.__Span__.__End__);
    __Declaration__ =
        __Parser_Create_Variable_Declaration__(__Parser_State__, __Name__, __Type__, __Span__);
    if (__Declaration__ == NULL ||
        !__Parser_Push_Statement__(__Parser_State__, __Statements__, __Declaration__))
    {
        return 0;
    }

    __Destination__ = __Parser_New_Base_Lvalue__(
        __Parser_State__, __Ast_Lvalue_Base_Identifier__, __Name__, 0U, __Span__);
    if (__Destination__ == NULL)
    {
        return 0;
    }

    switch (__Initializer_Value__.__Kind__)
    {
        case __Initializer_Expression__:
            __Initialization__ =
                __Parser_New_Statement__(__Parser_State__, __Ast_Statement_Copy__, __Span__);
            if (__Initialization__ != NULL)
            {
                __Initialization__->__As__.__Copy__.__Destination__ = __Destination__;
                __Initialization__->__As__.__Copy__.__Expression__ =
                    __Initializer_Value__.__As__.__Expression__;
            }
            break;
        case __Initializer_Vector__:
            __Initialization__ = __Parser_New_Statement__(
                __Parser_State__, __Ast_Statement_Initialize_Vector__, __Span__);
            if (__Initialization__ != NULL)
            {
                __Initialization__->__As__.__Aggregate__.__Destination__ = __Destination__;
                __Initialization__->__As__.__Aggregate__.__Values__ =
                    __Initializer_Value__.__As__.__Aggregate__.__Values__;
                __Initialization__->__As__.__Aggregate__.__Value_Count__ =
                    __Initializer_Value__.__As__.__Aggregate__.__Count__;
            }
            break;
        case __Initializer_Record__:
            __Initialization__ = __Parser_New_Statement__(
                __Parser_State__, __Ast_Statement_Initialize_Record__, __Span__);
            if (__Initialization__ != NULL)
            {
                __Initialization__->__As__.__Record__.__Destination__ = __Destination__;
                __Initialization__->__As__.__Record__.__Fields__ =
                    __Initializer_Value__.__As__.__Record__.__Fields__;
                __Initialization__->__As__.__Record__.__Field_Count__ =
                    __Initializer_Value__.__As__.__Record__.__Count__;
            }
            break;
        case __Initializer_Box__:
            __Initialization__ = __Parser_New_Statement__(
                __Parser_State__, __Ast_Statement_Initialize_Box__, __Span__);
            if (__Initialization__ != NULL)
            {
                __Initialization__->__As__.__Box__.__Destination__ = __Destination__;
                __Initialization__->__As__.__Box__.__Value__ = __Initializer_Value__.__As__.__Box__;
            }
            break;
    }

    if (__Initialization__ == NULL)
    {
        return 0;
    }
    return __Parser_Push_Statement__(__Parser_State__, __Statements__, __Initialization__);
}
