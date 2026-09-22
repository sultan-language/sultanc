#ifndef SULTANC__SEMANTIC_TYPE_NAMED_H__
#define SULTANC__SEMANTIC_TYPE_NAMED_H__

#include "semantic/context.h"

const __Program_Unit__ *__Semantic_Type_Owner_Unit__(const __Semantic_Context__ *__Context__,
                                                     const __Ast_Type__ *__Type__);

int __Semantic_Resolve_Named_Entry__(__Semantic_Context__ *__Context__,
                                     __Ast_Type__ *__Type__,
                                     __Semantic_Type_Entry__ **__Out_Entry__);

#endif
