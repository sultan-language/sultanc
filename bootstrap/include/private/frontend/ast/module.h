#ifndef SULTANC__FRONTEND_AST_MODULE_H__
#define SULTANC__FRONTEND_AST_MODULE_H__

#include "statement.h"

typedef enum
{
    __Ast_Module_Item_Function__,
    __Ast_Module_Item_Type__,
    __Ast_Module_Item_Inline__,
    __Ast_Module_Item_Alias__
} __Ast_Module_Item_Kind__;

/* Empty Parent means the module-root namespace; otherwise Parent.Name is a
 * dependent child-name path. Alias declarations bind only another spelling. */
typedef struct
{
    __Text_Slice__ __Parent__;
    __Text_Slice__ __Name__;
} __Ast_Alias_Path__;

typedef struct
{
    __Ast_Alias_Path__ __Destination__;
    __Ast_Alias_Path__ __Target__;
} __Ast_Module_Alias__;

struct __Ast_Module_Item__
{
    __Ast_Header__ __Header__;
    __Ast_Module_Item_Kind__ __Kind__;
    __Text_Slice__ __Name__;
    int __Public__;
    union
    {
        __Ast_Function__ *__Function__;
        __Ast_Type_Declaration__ *__Type__;
        __Text_Slice__ __Inline__;
        __Ast_Module_Alias__ __Alias__;
    } __As__;
};

typedef struct
{
    __Ast_Module_Item__ **__Items__;
    size_t __Item_Count__;
    __Text_Slice__ *__Imports__;
    size_t __Import_Count__;
} __Ast_Module__;

#endif
