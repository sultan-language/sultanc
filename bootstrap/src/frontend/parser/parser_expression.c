#include "frontend/parser/expression_internal.h"
#include "frontend/parser/internal.h"

__Ast_Expression__ *__Parser_Parse_Expression__(__Parser__ *__Parser_State__)
{
    return __Parser_Parse_Expression_Min__(__Parser_State__, 1);
}
