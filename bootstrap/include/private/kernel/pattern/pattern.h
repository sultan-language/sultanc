#ifndef SULTANC__KERNEL_PATTERN_PATTERN_H__
#define SULTANC__KERNEL_PATTERN_PATTERN_H__

#include "frontend/ast/statement.h"
#include "semantic/context.h"
#include "core/diagnostic.h"
#include "support/containers/vector.h"

typedef struct
{
    __Text_Slice__ __Name__;
    __Ast_Type__ *__Type__;
    __Source_Span__ __Span__;
} __Pattern_Binding__;

typedef struct
{
    __Error_Id__ __Id__;
    __Source_Span__ __Span__;
} __Pattern_Error__;

typedef struct
{
    __Vector__ __Bindings__;
    int __Irrefutable__;
    int __Has_Top_Enum_Constructor__;
    size_t __Top_Enum_Constructor_Index__;
    int __Top_Enum_Constructor_Fully_Covered__;
} __Pattern_Analysis__;

void __Pattern_Analysis_Init__(__Pattern_Analysis__ *__Analysis__);

void __Pattern_Analysis_Destroy__(__Pattern_Analysis__ *__Analysis__);

int __Pattern_Analyze__(__Semantic_Context__ *__Context__,
                        __Ast_Type__ *__Expected_Type__,
                        __Ast_Pattern__ *__Pattern__,
                        __Pattern_Analysis__ *__Out_Analysis__,
                        __Pattern_Error__ *__Out_Error__);

#endif
