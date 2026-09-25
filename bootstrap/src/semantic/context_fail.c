/* Builds semantic diagnostics. */

#include "semantic/diagnostic.h"
#include "semantic/context.h"

/* Begins the semantic diagnostic. */
__Diagnostic__ *__Semantic_Begin_Diagnostic__(__Semantic_Context__ *__Context__,
                                              __Error_Id__ __Id__,
                                              __Source_Span__ __Span__)
{
    if (__Context__ == NULL)
    {
        return NULL;
    }

    if (!__Context__->__Failed__)
    {
        __Diagnostic_Begin__(&__Context__->__Diagnostic__, __Id__, __Span__);
    }
    __Context__->__Failed__ = 1;
    return &__Context__->__Diagnostic__;
}

/* Records a failure for the semantic. */
int __Semantic_Fail__(__Semantic_Context__ *__Context__,
                      __Error_Id__ __Id__,
                      __Source_Span__ __Span__)
{
    (void)__Semantic_Begin_Diagnostic__(__Context__, __Id__, __Span__);
    return 0;
}
