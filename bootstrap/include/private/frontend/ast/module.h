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
    __Ast_Module_Item_Alias__
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
