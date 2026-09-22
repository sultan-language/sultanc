#ifndef SULTANC__SEMANTIC_CONTEXT_LIFECYCLE_H__
#define SULTANC__SEMANTIC_CONTEXT_LIFECYCLE_H__

#include "context.h"

void __Semantic_Context_Init__(__Semantic_Context__ *__Context__, __Program__ *__Program_State__);

void __Semantic_Context_Destroy__(__Semantic_Context__ *__Context__);

#endif
