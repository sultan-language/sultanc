/* Declares global semantic collection. */

#ifndef SULTANC__SEMANTIC_CONTEXT_COLLECTION_H__
#define SULTANC__SEMANTIC_CONTEXT_COLLECTION_H__

#include "context.h"

/* Collects the semantic globals. */
int __Semantic_Collect_Globals__(__Semantic_Context__ *__Context__);

/* Checks whether the semantic is main name. */
int __Semantic_Is_Main_Name__(__Text_Slice__ __Name__);

#endif
