/* Declares semantic diagnostic helpers. */

#ifndef SULTANC__SEMANTIC_DIAGNOSTIC_H__
#define SULTANC__SEMANTIC_DIAGNOSTIC_H__

#include "context.h"

/* Begins a structured semantic diagnostic that callers may enrich with facts. */
__Diagnostic__ *__Semantic_Begin_Diagnostic__(__Semantic_Context__ *__Context__,
                                              __Error_Id__ __Id__,
                                              __Source_Span__ __Span__);

/* Generic semantic failure: emits the catalog-default structured diagnostic. */
int __Semantic_Fail__(__Semantic_Context__ *__Context__,
                      __Error_Id__ __Id__,
                      __Source_Span__ __Span__);

#endif
