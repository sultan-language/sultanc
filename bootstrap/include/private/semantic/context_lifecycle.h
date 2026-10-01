/* Declares semantic context lifecycle operations. */

#ifndef SULTANC__SEMANTIC_CONTEXT_LIFECYCLE_H__
#define SULTANC__SEMANTIC_CONTEXT_LIFECYCLE_H__

#include "context.h"

/* Initializes the semantic context. */
void __Semantic_Context_Init__(__Semantic_Context__ *__Context__, __Program__ *__Program_State__);

/* Releases the semantic context. */
void __Semantic_Context_Destroy__(__Semantic_Context__ *__Context__);

#endif
