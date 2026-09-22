#ifndef SULTANC__FRONTEND_AST_STORAGE_H__
#define SULTANC__FRONTEND_AST_STORAGE_H__

#include "context.h"
#include "base.h"
#include "core/source.h"

#include <stddef.h>

void *__Ast_Allocate__(__Ast_Context__ *__Context__, size_t __Size__, size_t __Alignment__);

__Ast_Header__ __Ast_New_Header__(__Ast_Context__ *__Context__, __Source_Span__ __Span__);

void *__Ast_Copy_Array__(__Ast_Context__ *__Context__,
                         const void *__Data__,
                         size_t __Count__,
                         size_t __Element_Size__,
                         size_t __Alignment__);

__Text_Slice__ __Ast_Copy_Text__(__Ast_Context__ *__Context__, __Text_Slice__ __Text__);

#endif
