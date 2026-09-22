#ifndef SULTANC__FRONTEND_AST_BASE_H__
#define SULTANC__FRONTEND_AST_BASE_H__

#include "core/source.h"

#include <stdint.h>

typedef uint64_t __Temporary_Id__;

typedef struct
{
    __Source_Span__ __Span__;
} __Ast_Header__;

typedef enum
{
    __Machine_I8__,
    __Machine_I16__,
    __Machine_I32__,
    __Machine_I64__,
    __Machine_U8__,
    __Machine_U16__,
    __Machine_U32__,
    __Machine_U64__,
    __Machine_F32__,
    __Machine_F64__
} __Machine_Type__;

typedef struct __Ast_Type__ __Ast_Type__;
typedef struct __Ast_Lvalue__ __Ast_Lvalue__;
typedef struct __Ast_Literal__ __Ast_Literal__;
typedef struct __Ast_Expression__ __Ast_Expression__;
typedef struct __Ast_Statement__ __Ast_Statement__;
typedef struct __Ast_Block__ __Ast_Block__;
typedef struct __Ast_Function__ __Ast_Function__;
typedef struct __Ast_Module_Item__ __Ast_Module_Item__;
typedef struct __Ast_Pattern__ __Ast_Pattern__;

#endif
