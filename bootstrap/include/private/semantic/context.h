/* Defines semantic context state. */

#ifndef SULTANC__SEMANTIC_CONTEXT_H__
#define SULTANC__SEMANTIC_CONTEXT_H__

#include "core/program.h"
#include "support/containers/vector.h"
#include "support/memory/arena.h"

/* Defines the semantic type entry structure. */
typedef struct
{
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* References the declaration. */
    __Ast_Type_Declaration__ *__Declaration__;
    /* References the generic declaration template for instantiated types. */
    __Ast_Type_Declaration__ *__Template_Declaration__;
    /* References concrete generic type arguments for this instance. */
    __Ast_Type__ **__Type_Arguments__;
    /* Stores concrete generic type argument count. */
    size_t __Type_Argument_Count__;
    /* Tracks whether this entry is a generic declaration template. */
    int __Is_Generic_Template__;
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
    /* References an arena-owned callable function type for this concrete function. */
    __Ast_Type__ *__Callable_Type__;
    /* References the generic function template for instantiated functions. */
    __Ast_Function__ *__Template_Function__;
    /* References concrete generic type arguments for this instance. */
    __Ast_Type__ **__Type_Arguments__;
    /* Stores concrete generic type argument count. */
    size_t __Type_Argument_Count__;
    /* Tracks whether this entry is a generic declaration template. */
    int __Is_Generic_Template__;
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

/* Defines one unit's contiguous global declaration ranges. */
typedef struct
{
    /* References the owning unit. */
    const __Program_Unit__ *__Unit__;
    /* Stores the first type entry. */
    size_t __Type_Begin__;
    /* Stores one-past the last type entry. */
    size_t __Type_End__;
    /* Stores the first function entry. */
    size_t __Function_Begin__;
    /* Stores one-past the last function entry. */
    size_t __Function_End__;
} __Semantic_Unit_Name_Range__;

/* Defines the semantic context structure. */
typedef struct
{
    /* References the program. */
    __Program__ *__Program__;
    /* Stores the types. */
    __Vector__ __Types__;
    /* Stores semantic type entries. */
    __Vector__ __Functions__;
    /* Stores arena-owned instantiated generic type entry pointers. */
    __Vector__ __Generic_Types__;
    /* Stores arena-owned instantiated generic semantic function entry pointers. */
    __Vector__ __Generic_Functions__;
    /* Stores semantic function entries. */
    __Vector__ __Type_Owners__;
    /* Stores semantic type-owner facts. */
    __Vector__ __Unit_Name_Ranges__;
    /* Stores one declaration range for each Program unit, indexed by Program_Unit.__Index__. */
    __Semantic_Function_Entry__ *__Main__;

    /* Module identity currently active for scoped Name/Type resolution. */
    const __Program_Unit__ *__Active_Unit__;


    /* Owns semantic generic instances and substituted AST types. */
    __Arena__ __Generic_Arena__;

    /* Stores the diagnostic. */
    __Diagnostic__ __Diagnostic__;
    /* Tracks the failed state. */
    int __Failed__;
} __Semantic_Context__;

#endif
