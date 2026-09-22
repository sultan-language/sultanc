#ifndef SULTANC__SEMANTIC_LITERAL_H__
#define SULTANC__SEMANTIC_LITERAL_H__

#include "kernel/type/type.h"

int __Semantic_Integer_Literal_Atom_Value__(const __Ast_Atom__ *__Atom__, int64_t *__Out_Value__);

int __Semantic_Integer_Literal_Expression_Value__(const __Ast_Expression__ *__Expression__,
                                                  int64_t *__Out_Value__);

int __Type_Integer_Literal_Fits__(__Semantic_Context__ *__Context__,
                                  int64_t __Value__,
                                  __Ast_Type__ *__Target_Type__);

#endif
