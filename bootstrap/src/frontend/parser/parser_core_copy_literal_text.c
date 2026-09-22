#include "frontend/ast/storage.h"
#include "frontend/parser/storage.h"
#include "frontend/parser/internal.h"

__Text_Slice__ __Parser_Copy_Literal_Text__(__Parser__ *__Parser_State__, __Text_Slice__ __Text__)
{
    return __Ast_Copy_Text__(__Parser_State__->__Ast__, __Text__);
}
