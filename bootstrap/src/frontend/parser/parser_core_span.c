/* Builds source spans from parser positions. */

#include "frontend/parser/span.h"
#include "frontend/parser/internal.h"

/* Returns the parser span. */
__Source_Span__ __Parser_Span__(__Source_Position__ __Start__, __Source_Position__ __End__)
{
    /* Stores the span. */
    __Source_Span__ __Span__;
    __Span__.__Start__ = __Start__;
    __Span__.__End__ = __End__;
    return __Span__;
}
