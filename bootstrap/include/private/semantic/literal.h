/* Declares integer literal value helpers. */

#ifndef SULTANC__SEMANTIC_LITERAL_H__
#define SULTANC__SEMANTIC_LITERAL_H__

#include "kernel/type/type.h"

/* Returns the semantic integer literal atom value. */
int __Semantic_Integer_Literal_Atom_Value__(const __Ast_Atom__ *__Atom__, int64_t *__Out_Value__);

/* Returns the semantic integer literal expression value. */
int __Semantic_Integer_Literal_Expression_Value__(const __Ast_Expression__ *__Expression__,
                                                  int64_t *__Out_Value__);

/* Returns the type integer literal fits. */
int __Type_Integer_Literal_Fits__(__Semantic_Context__ *__Context__,
                                  int64_t __Value__,
                                  __Ast_Type__ *__Target_Type__);

#endif
