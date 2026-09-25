/* Copies text into AST-owned storage. */

#include "frontend/ast/storage.h"
#include "frontend/ast/context.h"

/* Copies the AST text. */
__Text_Slice__ __Ast_Copy_Text__(__Ast_Context__ *__Context__, __Text_Slice__ __Text__)
{
    /* Stores the copy. */
    __Text_Slice__ __Copy__ = {0};
    /* References the data. */
    char *__Data__ =
        __Arena_Copy_Text__(&__Context__->__Arena__, __Text__.__Data__, __Text__.__Length__);
    if (__Data__ == NULL)
    {
        return __Copy__;
    }
    __Copy__.__Data__ = __Data__;
    __Copy__.__Length__ = __Text__.__Length__;
    return __Copy__;
}
