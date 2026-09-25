/* Defines semantic context state. */

#ifndef SULTANC__SEMANTIC_CONTEXT_H__
#define SULTANC__SEMANTIC_CONTEXT_H__

#include "core/program.h"
#include "support/containers/vector.h"

/* Defines the semantic type entry structure. */
typedef struct
{
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* References the declaration. */
    __Ast_Type_Declaration__ *__Declaration__;
    /* References the unit. */
    const __Program_Unit__ *__Unit__;
    /* Tracks the public state. */
    int __Public__;
    /* Stores the size. */
    size_t __Size__;
    /* Stores the alignment. */
    size_t __Alignment__;
    /* Stores the layout state. */
    int __Layout_State__;
    /* State: 0 unknown, 1 visiting, 2 ready. */
} __Semantic_Type_Entry__;

/* Defines the semantic function entry structure. */
typedef struct
{
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* References the function. */
    __Ast_Function__ *__Function__;
    /* References the unit. */
    const __Program_Unit__ *__Unit__;
    /* Tracks the public state. */
    int __Public__;
    /* Tracks the index. */
    size_t __Index__;
    /* Stores the reference return parameter. */
    size_t __Reference_Return_Parameter__;
    /* Stores the composite view return parameter. */
    size_t __Composite_View_Return_Parameter__;
} __Semantic_Function_Entry__;

/* Defines the semantic type owner fact structure. */
typedef struct
{
    /* References the type. */
    const __Ast_Type__ *__Type__;
    /* References the unit. */
    const __Program_Unit__ *__Unit__;
} __Semantic_Type_Owner_Fact__;

/* Defines the semantic context structure. */
typedef struct
{
    /* References the program. */
    __Program__ *__Program__;
    /* Stores the types. */
    __Vector__ __Types__;
    /* Stores semantic type entries. */
    __Vector__ __Functions__;
    /* Stores semantic function entries. */
    __Vector__ __Type_Owners__;
    /* Stores semantic type-owner facts. */
    __Semantic_Function_Entry__ *__Main__;

    /* Module identity currently active for scoped Name/Type resolution. */
    const __Program_Unit__ *__Active_Unit__;

    /* Stores the diagnostic. */
    __Diagnostic__ __Diagnostic__;
    /* Tracks the failed state. */
    int __Failed__;
} __Semantic_Context__;

#endif
