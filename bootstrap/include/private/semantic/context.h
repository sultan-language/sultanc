#ifndef SULTANC__SEMANTIC_CONTEXT_H__
#define SULTANC__SEMANTIC_CONTEXT_H__

#include "core/program.h"
#include "support/containers/vector.h"

typedef struct
{
    __Text_Slice__ __Name__;
    __Ast_Type_Declaration__ *__Declaration__;
    const __Program_Unit__ *__Unit__;
    int __Public__;
    size_t __Size__;
    size_t __Alignment__;
    int __Layout_State__;
    /* 0 unknown, 1 visiting, 2 ready */
} __Semantic_Type_Entry__;

typedef struct
{
    __Text_Slice__ __Name__;
    __Ast_Function__ *__Function__;
    const __Program_Unit__ *__Unit__;
    int __Public__;
    size_t __Index__;
    size_t __Reference_Return_Parameter__;
    size_t __Composite_View_Return_Parameter__;
} __Semantic_Function_Entry__;

typedef struct
{
    const __Ast_Type__ *__Type__;
    const __Program_Unit__ *__Unit__;
} __Semantic_Type_Owner_Fact__;

typedef struct
{
    __Program__ *__Program__;
    __Vector__ __Types__;
    /* __Semantic_Type_Entry__ */
    __Vector__ __Functions__;
    /* __Semantic_Function_Entry__ */
    __Vector__ __Type_Owners__;
    /* __Semantic_Type_Owner_Fact__ */
    __Semantic_Function_Entry__ *__Main__;

    /* Module identity currently active for scoped Name/Type resolution. */
    const __Program_Unit__ *__Active_Unit__;

    __Diagnostic__ __Diagnostic__;
    int __Failed__;
} __Semantic_Context__;

#endif
