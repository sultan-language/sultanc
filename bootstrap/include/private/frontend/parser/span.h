/* Declares parser span helpers. */

#ifndef SULTANC__FRONTEND_PARSER_SPAN_H__
#define SULTANC__FRONTEND_PARSER_SPAN_H__

#include "internal.h"

/* Returns the parser span. */
__Source_Span__ __Parser_Span__(__Source_Position__ __Start__, __Source_Position__ __End__);

#endif
