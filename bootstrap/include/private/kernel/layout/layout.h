/* Declares canonical type layout queries. */

#ifndef SULTANC__KERNEL_LAYOUT_LAYOUT_H__
#define SULTANC__KERNEL_LAYOUT_LAYOUT_H__

#include "semantic/context.h"

/* Returns the layout type. */
int __Layout_Type__(__Semantic_Context__ *__Context__,
                    __Ast_Type__ *__Type__,
                    size_t *__Out_Size__,
                    size_t *__Out_Alignment__);

/* Returns the layout struct field. */
int __Layout_Struct_Field__(__Semantic_Context__ *__Context__,
                            __Semantic_Type_Entry__ *__Entry__,
                            __Text_Slice__ __Field_Name__,
                            size_t *__Out_Offset__,
                            __Ast_Type__ **__Out_Type__);

/* Returns the layout tagged storage. */
int __Layout_Tagged_Storage__(__Semantic_Context__ *__Context__,
                              __Ast_Type__ *__Type__,
                              size_t *__Out_Tag_Size__,
                              size_t *__Out_Payload_Offset__,
                              size_t *__Out_Payload_Size__,
                              size_t *__Out_Alignment__);

/* Returns the layout tagged payload. */
int __Layout_Tagged_Payload__(__Semantic_Context__ *__Context__,
                              __Ast_Type__ *__Type__,
                              size_t __Constructor_Index__,
                              size_t __Payload_Index__,
                              size_t *__Out_Offset__,
                              __Ast_Type__ **__Out_Type__);

/* Returns the layout string data offset. */
size_t __Layout_String_Data_Offset__(void);

/* Returns the layout string length offset. */
size_t __Layout_String_Length_Offset__(void);

/* Returns the layout string capacity offset. */
size_t __Layout_String_Capacity_Offset__(void);

/* Returns the layout vector data offset. */
size_t __Layout_Vector_Data_Offset__(void);

/* Returns the layout vector length offset. */
size_t __Layout_Vector_Length_Offset__(void);

/* Returns the layout vector capacity offset. */
size_t __Layout_Vector_Capacity_Offset__(void);

#endif
