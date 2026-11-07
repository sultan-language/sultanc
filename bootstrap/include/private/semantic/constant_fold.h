/* Declares module-constant folding (canonical: compile-time evaluation of a
 * قيمة_عليا initializer). */

#ifndef SULTANC__SEMANTIC_CONSTANT_FOLD_H__
#define SULTANC__SEMANTIC_CONSTANT_FOLD_H__

#include "semantic/context.h"

/* Folds the initializer of one module-level value into its declaration's
 * __Constant__ (external data has none). Answers 0 after a diagnostic. */
int __Semantic_Fold_Global__(__Semantic_Context__ *__Context__, __Semantic_Global_Entry__ *__Entry__);

#endif
