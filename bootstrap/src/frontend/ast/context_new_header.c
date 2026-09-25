/* Creates common AST node metadata. */

#include "frontend/ast/storage.h"
#include "frontend/ast/context.h"

/* Creates the AST header. */
__Ast_Header__ __Ast_New_Header__(__Ast_Context__ *__Context__, __Source_Span__ __Span__)
{
    /* Stores the header. */
    __Ast_Header__ __Header__;
    (void)__Context__;
    __Header__.__Span__ = __Span__;
    return __Header__;
}
