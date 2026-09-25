/* Declares pattern analysis results and queries. */

#ifndef SULTANC__KERNEL_PATTERN_PATTERN_H__
#define SULTANC__KERNEL_PATTERN_PATTERN_H__

#include "frontend/ast/statement.h"
#include "semantic/context.h"
#include "core/diagnostic.h"
#include "support/containers/vector.h"

/* Defines the pattern binding structure. */
typedef struct
{
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* References the type. */
    __Ast_Type__ *__Type__;
    /* Stores the span. */
    __Source_Span__ __Span__;
} __Pattern_Binding__;

/* Defines the pattern error structure. */
typedef struct
{
    /* Stores the ID. */
    __Error_Id__ __Id__;
    /* Stores the span. */
    __Source_Span__ __Span__;
} __Pattern_Error__;

/* Defines the pattern analysis structure. */
typedef struct
{
    /* Stores the bindings. */
    __Vector__ __Bindings__;
    /* Stores the irrefutable. */
    int __Irrefutable__;
    /* Tracks whether the top enum constructor is present. */
    int __Has_Top_Enum_Constructor__;
    /* Tracks the top enum constructor index. */
    size_t __Top_Enum_Constructor_Index__;
    /* Tracks the top enum constructor fully covered state. */
    int __Top_Enum_Constructor_Fully_Covered__;
} __Pattern_Analysis__;

/* Initializes the pattern analysis. */
void __Pattern_Analysis_Init__(__Pattern_Analysis__ *__Analysis__);

/* Releases the pattern analysis. */
void __Pattern_Analysis_Destroy__(__Pattern_Analysis__ *__Analysis__);

/* Analyzes the pattern. */
int __Pattern_Analyze__(__Semantic_Context__ *__Context__,
                        __Ast_Type__ *__Expected_Type__,
                        __Ast_Pattern__ *__Pattern__,
                        __Pattern_Analysis__ *__Out_Analysis__,
                        __Pattern_Error__ *__Out_Error__);

#endif
