/* Defines common AST node metadata. */

#ifndef SULTANC__FRONTEND_AST_BASE_H__
#define SULTANC__FRONTEND_AST_BASE_H__

#include "core/source.h"

#include <stdint.h>

/* Defines the temporary ID type alias. */
typedef uint64_t __Temporary_Id__;

/* Defines the AST header structure. */
typedef struct
{
    /* Stores the span. */
    __Source_Span__ __Span__;
} __Ast_Header__;

/* Defines the machine type values. */
typedef enum
{
    /* Represents the machine i 8 value. */
    __Machine_I8__,
    /* Represents the machine i 16 value. */
    __Machine_I16__,
    /* Represents the machine i 32 value. */
    __Machine_I32__,
    /* Represents the machine i 64 value. */
    __Machine_I64__,
    /* Represents the machine u 8 value. */
    __Machine_U8__,
    /* Represents the machine u 16 value. */
    __Machine_U16__,
    /* Represents the machine u 32 value. */
    __Machine_U32__,
    /* Represents the machine u 64 value. */
    __Machine_U64__,
    /* Represents the machine f 32 value. */
    __Machine_F32__,
    /* Represents the machine f 64 value. */
    __Machine_F64__
} __Machine_Type__;

/* Defines the AST type structure. */
typedef struct __Ast_Type__ __Ast_Type__;
/* Defines the AST lvalue structure. */
typedef struct __Ast_Lvalue__ __Ast_Lvalue__;
/* Defines the AST literal structure. */
typedef struct __Ast_Literal__ __Ast_Literal__;
/* Defines the AST expression structure. */
typedef struct __Ast_Expression__ __Ast_Expression__;
/* Defines the AST statement structure. */
typedef struct __Ast_Statement__ __Ast_Statement__;
/* Defines the AST block structure. */
typedef struct __Ast_Block__ __Ast_Block__;
/* Defines the AST function structure. */
typedef struct __Ast_Function__ __Ast_Function__;
/* Defines the AST module item structure. */
typedef struct __Ast_Module_Item__ __Ast_Module_Item__;
/* Defines the AST pattern structure. */
typedef struct __Ast_Pattern__ __Ast_Pattern__;

#endif
