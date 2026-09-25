/* Declares AST context allocation helpers. */

#ifndef SULTANC__FRONTEND_AST_CONTEXT_H__
#define SULTANC__FRONTEND_AST_CONTEXT_H__

#include "support/memory/arena.h"

/* Defines the AST context structure. */
typedef struct
{
    /* Stores the arena. */
    __Arena__ __Arena__;
} __Ast_Context__;

#endif
