#ifndef SULTANC__FRONTEND_AST_IDENTITY_H__
#define SULTANC__FRONTEND_AST_IDENTITY_H__

#include "expression.h"

int __Ast_Base_Identity_Matches__(__Ast_Lvalue_Base_Kind__ __Left_Kind__,
                                  __Text_Slice__ __Left_Name__,
                                  __Temporary_Id__ __Left_Temporary__,
                                  __Ast_Lvalue_Base_Kind__ __Right_Kind__,
                                  __Text_Slice__ __Right_Name__,
                                  __Temporary_Id__ __Right_Temporary__);

int __Ast_Base_Identity_Matches_Lvalue__(__Ast_Lvalue_Base_Kind__ __Kind__,
                                         __Text_Slice__ __Name__,
                                         __Temporary_Id__ __Temporary__,
                                         const __Ast_Lvalue__ *__Lvalue__);

#endif
