#ifndef SULTANC__KERNEL_LAYOUT_LAYOUT_H__
#define SULTANC__KERNEL_LAYOUT_LAYOUT_H__

#include "semantic/context.h"

int __Layout_Type__(__Semantic_Context__ *__Context__,
                    __Ast_Type__ *__Type__,
                    size_t *__Out_Size__,
                    size_t *__Out_Alignment__);

int __Layout_Struct_Field__(__Semantic_Context__ *__Context__,
                            __Semantic_Type_Entry__ *__Entry__,
                            __Text_Slice__ __Field_Name__,
                            size_t *__Out_Offset__,
                            __Ast_Type__ **__Out_Type__);

int __Layout_Tagged_Storage__(__Semantic_Context__ *__Context__,
                              __Ast_Type__ *__Type__,
                              size_t *__Out_Tag_Size__,
                              size_t *__Out_Payload_Offset__,
                              size_t *__Out_Payload_Size__,
                              size_t *__Out_Alignment__);

int __Layout_Tagged_Payload__(__Semantic_Context__ *__Context__,
                              __Ast_Type__ *__Type__,
                              size_t __Constructor_Index__,
                              size_t __Payload_Index__,
                              size_t *__Out_Offset__,
                              __Ast_Type__ **__Out_Type__);

size_t __Layout_String_Data_Offset__(void);

size_t __Layout_String_Length_Offset__(void);

size_t __Layout_String_Capacity_Offset__(void);

size_t __Layout_Vector_Data_Offset__(void);

size_t __Layout_Vector_Length_Offset__(void);

size_t __Layout_Vector_Capacity_Offset__(void);

#endif
