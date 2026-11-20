/* Defines module AST nodes. */

#ifndef SULTANC__FRONTEND_AST_MODULE_H__
#define SULTANC__FRONTEND_AST_MODULE_H__

#include "statement.h"

/* Defines the AST module item kind values. */
typedef enum
{
    /* Represents the AST module item function value. */
    __Ast_Module_Item_Function__,
    /* Represents the AST module item type value. */
    __Ast_Module_Item_Type__,
    /* Represents the AST module item inline value. */
    __Ast_Module_Item_Inline__,
    /* Represents the AST module item alias value. */
    __Ast_Module_Item_Alias__,
    /* Represents one module-level value declaration (canonical قيمة_عليا). */
    __Ast_Module_Item_Global__
} __Ast_Module_Item_Kind__;

/* Empty Parent means the module-root namespace; otherwise Parent.Name is a
 * dependent child-name path. Alias declarations bind only another spelling. */
typedef struct
{
    /* Stores the parent. */
    __Text_Slice__ __Parent__;
    /* Stores the name. */
    __Text_Slice__ __Name__;
} __Ast_Alias_Path__;

/* Defines the AST module alias structure. */
typedef struct
{
    /* Stores the destination. */
    __Ast_Alias_Path__ __Destination__;
    /* Stores the target. */
    __Ast_Alias_Path__ __Target__;
} __Ast_Module_Alias__;

/* Defines the kinds of a module constant folded at compile time. */
typedef enum
{
    __Ast_Constant_Integer__,
    __Ast_Constant_Boolean__,
    __Ast_Constant_Character__,
    __Ast_Constant_Float__,
    __Ast_Constant_Text__,
    __Ast_Constant_Vector__
} __Ast_Constant_Kind__;

/* Defines one folded module constant (canonical: the compile-time evaluation
 * of a قيمة_عليا initializer). An integer is held as a signed 64-bit value;
 * `__Wide__` marks an unsigned 64-bit value above INT64_MAX, whose bits are
 * `__Integer__`. A float keeps its decimal spelling, which LLVM parses
 * exactly. A vector holds its folded elements. */
typedef struct __Ast_Constant__
{
    __Ast_Constant_Kind__ __Kind__;
    int64_t __Integer__;
    int __Wide__;
    int __Negative_Float__;
    /* A float is its decimal text parsed at __Float_Steps__[0] bits, then
     * converted through each following width (`0.1 as f32` parses at 64
     * and rounds to 32); no steps means the declared Type's width. */
    unsigned char __Float_Steps__[4];
    size_t __Float_Step_Count__;
    __Text_Slice__ __Text__;
    struct __Ast_Constant__ *__Elements__;
    size_t __Element_Count__;
} __Ast_Constant__;

/* Defines one module-level value declaration. This mirrors the canonical
 * Frontend تصريح_قيمة_عليا (compiler/frontend/ast/declarations.sn): external
 * data (تهيئة_قيمة_عليا.خارجية, an imported linker symbol with an explicit
 * name and no initializer), an expression initializer (تعبير) or a vector
 * literal (متجه). Initializers are folded to constants by the semantic pass. */
typedef struct
{
    /* Stores the header. */
    __Ast_Header__ __Header__;
    /* References the declared value Type. */
    __Ast_Type__ *__Type__;
    /* Stores the AST-owned linker symbol bytes (nonempty, without NUL). */
    __Text_Slice__ __Link_Name__;
    /* Tracks whether the value is external data. */
    int __External__;
    /* References the expression initializer, or NULL. */
    __Ast_Expression__ *__Initializer__;
    /* Tracks the vector-literal initializer form. */
    int __Vector_Form__;
    /* References the vector-literal elements. */
    __Ast_Expression__ **__Elements__;
    /* Stores the vector-literal element count. */
    size_t __Element_Count__;
    /* References the folded constant (set by the semantic pass). */
    __Ast_Constant__ *__Constant__;
    /* Tracks folding: 0 not started, 1 in progress (a cycle), 2 done. */
    int __Fold_State__;
} __Ast_Global__;

/* Defines the AST module item structure. */
struct __Ast_Module_Item__
{
    /* Stores the header. */
    __Ast_Header__ __Header__;
    /* Stores the kind. */
    __Ast_Module_Item_Kind__ __Kind__;
    /* Stores the name. */
    __Text_Slice__ __Name__;
    /* Tracks the public state. */
    int __Public__;
    /* Defines the struct structure. */
    union
    {
        /* References the function. */
        __Ast_Function__ *__Function__;
        /* References the type. */
        __Ast_Type_Declaration__ *__Type__;
        /* Stores the inline. */
        __Text_Slice__ __Inline__;
        /* Stores the alias. */
        __Ast_Module_Alias__ __Alias__;
        /* References the module-level value declaration. */
        __Ast_Global__ *__Global__;
    } __As__;
};

/* Defines the AST import kind values. */
typedef enum
{
    /* Represents the AST import relative value. */
    __Ast_Import_Relative__,
    /* Represents the AST import logical value. */
    __Ast_Import_Logical__
} __Ast_Import_Kind__;

/* Defines the AST import structure. */
typedef struct
{
    /* Stores the kind. */
    __Ast_Import_Kind__ __Kind__;
    /* Stores the path. */
    __Text_Slice__ __Path__;
} __Ast_Import__;

/* Defines the AST module structure. */
typedef struct
{
    /* References the items. */
    __Ast_Module_Item__ **__Items__;
    /* Stores the item count. */
    size_t __Item_Count__;
    /* References the imports. */
    __Ast_Import__ *__Imports__;
    /* Stores the import count. */
    size_t __Import_Count__;
} __Ast_Module__;

#endif
