/* Defines internal diagnostic storage helpers. */

#ifndef SULTANC__DIAGNOSTICS_CORE_INTERNAL_H__
#define SULTANC__DIAGNOSTICS_CORE_INTERNAL_H__

#include "core/diagnostic.h"

/* Captures the diagnostic source span. */
void __Diagnostic_Capture_Source_Span__(__Diagnostic_Source_Span__ *__Destination__,
                                        __Source_Span__ __Source_Span__);

#endif
