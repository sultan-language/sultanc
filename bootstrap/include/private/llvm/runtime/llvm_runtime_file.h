/* Declares private Stage0 LLVM runtime file bridges. */

#ifndef SULTANC_BOOTSTRAP_LLVM_RUNTIME_FILE_H
#define SULTANC_BOOTSTRAP_LLVM_RUNTIME_FILE_H

#include "llvm/llvm_internal.h"

__LLVM_Value__ __LLVM_Emit_Runtime_Open_File_Service__(__LLVM_Emitter__ *emitter,
                                                       __Ast_Expression__ *expression,
                                                       __Ast_Type__ *expected,
                                                       int create_for_write);
__LLVM_Value__ __LLVM_Emit_Runtime_Read_Byte_Service__(__LLVM_Emitter__ *emitter,
                                                       __Ast_Expression__ *expression,
                                                       __Ast_Type__ *expected,
                                                       int stdin_mode);
__LLVM_Value__ __LLVM_Emit_Runtime_Read_Segment_Service__(__LLVM_Emitter__ *emitter,
                                                          __Ast_Expression__ *expression,
                                                          __Ast_Type__ *expected,
                                                          int stdin_mode);
__LLVM_Value__ __LLVM_Emit_Runtime_Close_File_Service__(__LLVM_Emitter__ *emitter,
                                                        __Ast_Expression__ *expression,
                                                        __Ast_Type__ *expected);
__LLVM_Value__ __LLVM_Emit_Runtime_Write_Segment_Service__(__LLVM_Emitter__ *emitter,
                                                           __Ast_Expression__ *expression,
                                                           __Ast_Type__ *expected);
__LLVM_Value__ __LLVM_Emit_Runtime_Write_Executable_Bytes__(__LLVM_Emitter__ *emitter,
                                                            __Ast_Expression__ *expression,
                                                            __Ast_Type__ *result_type);

#endif
