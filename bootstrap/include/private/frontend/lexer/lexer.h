/* Defines lexer state and lifecycle. */

#ifndef SULTANC__FRONTEND_LEXER_LEXER_H__
#define SULTANC__FRONTEND_LEXER_LEXER_H__

#include "core/diagnostic.h"
#include "core/source.h"
#include "support/memory/arena.h"

#include <stddef.h>
#include <stdint.h>

/* Defines the lexer structure. */
typedef struct
{
    /* References the source. */
    const __Source_File__ *__Source__;
    /* Borrowed source reference. */
    size_t __Offset__;
    /* Stores the line. */
    uint32_t __Line__;
    /* Stores the column. */
    uint32_t __Column__;
    /* Stores the literal arena. */
    __Arena__ __Literal_Arena__;
    /* Owns decoded literal text. */
    __Diagnostic__ __Diagnostic__;
    /* Tracks the failed state. */
    int __Failed__;
} __Lexer__;

#endif
