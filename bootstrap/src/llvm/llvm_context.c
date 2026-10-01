/* Owns shared Stage0 LLVM emitter context helpers and internal errors. */

#include "llvm/llvm_context.h"
#include "frontend/identifier_identity.h"
#include <stdio.h>

/* Stores the LLVM error. */
char __LLVM_Error__[512];

/* Compares the LLVM text. */
int __LLVM_Text_Equals__(__Text_Slice__ left, __Text_Slice__ right)
{
    return __Identifier_Identity_Equals__(left, right);
}

/* Records a failure for the LLVM. */
int __LLVM_Fail__(const char *message)
{
    if (__LLVM_Error__[0] == '\0')
    {
        snprintf(__LLVM_Error__, sizeof(__LLVM_Error__), "%s", message);
    }
    return 0;
}

/* Records a failure for the LLVM message. */
int __LLVM_Fail_Message__(const char *prefix, char *message)
{
    snprintf(__LLVM_Error__,
             sizeof(__LLVM_Error__),
             "%s%s%s",
             prefix,
             message != NULL ? ": " : "",
             message != NULL ? message : "");
    if (message != NULL)
    {
        LLVMDisposeMessage(message);
    }
    return 0;
}

/* Records a failure for an LLVM error object. */
int __LLVM_Fail_Error__(const char *prefix, LLVMErrorRef error)
{
    /* References the consumed error message. */
    char *message;
    if (error == NULL)
        return __LLVM_Fail__(prefix);
    message = LLVMGetErrorMessage(error);
    snprintf(__LLVM_Error__,
             sizeof(__LLVM_Error__),
             "%s%s%s",
             prefix,
             message != NULL ? ": " : "",
             message != NULL ? message : "");
    if (message != NULL)
        LLVMDisposeErrorMessage(message);
    return 0;
}
