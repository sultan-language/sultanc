#ifndef SULTANC__FRONTEND_LEXER_LEXER_H__
#define SULTANC__FRONTEND_LEXER_LEXER_H__

#include "core/diagnostic.h"
#include "core/source.h"
#include "support/memory/arena.h"

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    const __Source_File__ *__Source__;
    /* borrowed */
    size_t __Offset__;
    uint32_t __Line__;
    uint32_t __Column__;
    __Arena__ __Literal_Arena__;
    /* owns decoded literal text */
    __Diagnostic__ __Diagnostic__;
    int __Failed__;
} __Lexer__;

#endif
