/* Declares shared private Stage0 LLVM context and error helpers. */

#ifndef SULTANC_BOOTSTRAP_LLVM_CONTEXT_H
#define SULTANC_BOOTSTRAP_LLVM_CONTEXT_H

#include "llvm/llvm_internal.h"

/* Stores the LLVM error. */
extern char __LLVM_Error__[512];

int __LLVM_Text_Equals__(__Text_Slice__ left, __Text_Slice__ right);
int __LLVM_Fail__(const char *message);
int __LLVM_Fail_Message__(const char *prefix, char *message);
int __LLVM_Fail_Error__(const char *prefix, LLVMErrorRef error);

#endif
