/* Owns existing Stage0 LLVM native object generation. */

#include "llvm/llvm_internal.h"
#include "llvm/llvm_context.h"
#include "llvm/llvm_module.h"
#include <stdio.h>
#include <string.h>

/* The C library function a memory intrinsic (llvm.memcpy/memmove/memset)
   becomes a call to, or NULL for any other function. */
static const char *__LLVM_Memory_Intrinsic_Library_Name__(LLVMValueRef callee)
{
    /* Holds the callee's name. */
    size_t length = 0;
    /* The callee's name. */
    const char *name = callee != NULL ? LLVMGetValueName2(callee, &length) : NULL;

    if (name == NULL)
        return NULL;
    if (strncmp(name, "llvm.memcpy.", 12) == 0)
        return "memcpy";
    if (strncmp(name, "llvm.memmove.", 13) == 0)
        return "memmove";
    if (strncmp(name, "llvm.memset.", 12) == 0)
        return "memset";
    return NULL;
}

/* The module's declaration of memcpy, memmove (void *(void *, const void *,
   size_t)) or memset (void *(void *, int, size_t)), dllimport. */
static LLVMValueRef __LLVM_Library_Memory_Function__(LLVMModuleRef module, const char *name,
                                                      LLVMTypeRef *type)
{
    /* The module's context. */
    LLVMContextRef context = LLVMGetModuleContext(module);
    /* The C pointer type. */
    LLVMTypeRef pointer = LLVMPointerTypeInContext(context, 0);
    /* The parameters: destination, source or byte value, size. */
    LLVMTypeRef parameters[3];
    /* The declaration. */
    LLVMValueRef function;

    parameters[0] = pointer;
    parameters[1] = strcmp(name, "memset") == 0 ? LLVMInt32TypeInContext(context) : pointer;
    parameters[2] = LLVMInt64TypeInContext(context);
    *type = LLVMFunctionType(pointer, parameters, 3, 0);
    function = LLVMGetNamedFunction(module, name);
    if (function == NULL)
        function = LLVMAddFunction(module, name, *type);
    LLVMSetDLLStorageClass(function, LLVMDLLImportStorageClass);
    return function;
}

/* Replaces the memory intrinsics that become library calls (those whose
   length is not a small constant) with calls to the dllimport library
   function, so they too go through the import table. Small fixed-size
   copies stay intrinsics and are expanded inline. */
static void __LLVM_Import_Memory_Calls__(LLVMModuleRef module)
{
    /* The module's context. */
    LLVMContextRef context = LLVMGetModuleContext(module);
    /* Builds the replacement calls. */
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(context);
    /* Iterates the module's functions. */
    LLVMValueRef function;

    for (function = LLVMGetFirstFunction(module); function != NULL; function = LLVMGetNextFunction(function))
    {
        /* Iterates the function's blocks. */
        LLVMBasicBlockRef block;

        for (block = LLVMGetFirstBasicBlock(function); block != NULL; block = LLVMGetNextBasicBlock(block))
        {
            /* Iterates the block's instructions. */
            LLVMValueRef instruction = LLVMGetFirstInstruction(block);

            while (instruction != NULL)
            {
                /* The next instruction, read before this one may go. */
                LLVMValueRef next = LLVMGetNextInstruction(instruction);
                /* The library function the call becomes, if any. */
                const char *name = LLVMIsACallInst(instruction)
                                       ? __LLVM_Memory_Intrinsic_Library_Name__(LLVMGetCalledValue(instruction))
                                       : NULL;

                if (name != NULL)
                {
                    /* The length operand. */
                    LLVMValueRef length = LLVMGetOperand(instruction, 2);
                    /* Whether LLVM expands the intrinsic inline. */
                    int small = LLVMIsAConstantInt(length) && LLVMConstIntGetZExtValue(length) <= 128;

                    if (!small)
                    {
                        /* The library function's type. */
                        LLVMTypeRef type;
                        /* The library function. */
                        LLVMValueRef callee = __LLVM_Library_Memory_Function__(module, name, &type);
                        /* The call's arguments. */
                        LLVMValueRef arguments[3];

                        LLVMPositionBuilderBefore(builder, instruction);
                        arguments[0] = LLVMGetOperand(instruction, 0);
                        arguments[1] = LLVMGetOperand(instruction, 1);
                        if (strcmp(name, "memset") == 0)
                            arguments[1] = LLVMBuildZExt(builder, arguments[1], LLVMInt32TypeInContext(context), "");
                        arguments[2] = LLVMBuildZExtOrBitCast(builder, length, LLVMInt64TypeInContext(context), "");
                        LLVMBuildCall2(builder, type, callee, arguments, 3, "");
                        LLVMInstructionEraseFromParent(instruction);
                    }
                }
                instruction = next;
            }
        }
    }
    LLVMDisposeBuilder(builder);
}

/* Prepares the module for Windows ARM64, where one .text holding the whole
   compiler outgrows the +-128 MiB reach of a direct branch (BL, BRANCH26).
   - Calls to the C runtime and system DLLs go through their import address
     table: every external function is declared dllimport, as MSVC's headers
     declare the CRT and Win32, so the call loads __imp_<name> and branches
     through a register instead of reaching an import thunk at the far end of
     .text. Memory intrinsics that become library calls are made such calls
     first. (A symbol a static library defines still links: the linker makes
     its __imp_ pointer.) Only __chkstk, which LLVM calls for large frames,
     is still reached by a direct branch.
   - Every defined function gets a section of its own, as MSVC's /Gy and
     clang's -ffunction-sections do, so the linker can place range-extension
     thunks between functions. The names .text$sNNNNNNNN keep them in order
     inside .text. */
static void __LLVM_Prepare_Windows_ARM64_Module__(LLVMModuleRef module)
{
    /* Counts the defined functions. */
    unsigned index = 0;
    /* Counts the imported functions. */
    unsigned imported = 0;
    /* Iterates the module's functions. */
    LLVMValueRef function;
    /* Holds one section name. */
    char name[32];

    __LLVM_Import_Memory_Calls__(module);
    for (function = LLVMGetFirstFunction(module); function != NULL; function = LLVMGetNextFunction(function))
    {
        if (!LLVMIsDeclaration(function))
        {
            snprintf(name, sizeof(name), ".text$s%08u", index);
            LLVMSetSection(function, name);
            index++;
        }
        else if (LLVMGetIntrinsicID(function) == 0)
        {
            LLVMSetDLLStorageClass(function, LLVMDLLImportStorageClass);
            imported++;
        }
    }
    /* One line in the build log shows the layout was applied. */
    printf("sultanc-stage0: Windows ARM64 layout: %u functions in sections of their own, "
           "%u external functions called through the import table\n",
           index, imported);
    fflush(stdout);
}

/* Emits the bootstrap LLVM object. */
int __Bootstrap_Emit_LLVM_Object__(__Semantic_Context__ *semantic,
                                  const char *path,
                                  const __Bootstrap_Target__ *target)
{
    /* Stores the emitter. */
    __LLVM_Emitter__ emitter;
    /* References the saved unit. */
    const __Program_Unit__ *saved_unit = NULL;
    /* Stores the machine. */
    LLVMTargetMachineRef machine = NULL;
    /* Stores the data. */
    LLVMTargetDataRef data = NULL;
    /* References the message. */
    char *message = NULL;
    /* Tracks whether the operation succeeded. */
    int ok = 0;

    memset(&emitter, 0, sizeof(emitter));
    if (path == NULL)
        return __LLVM_Fail__("missing native object output path");
    if (__LLVM_Prepare_Program_Module__(
            semantic, target, &emitter, &saved_unit, &machine, &data))
    {
        if (target != NULL && target->platform == __Bootstrap_Target_Platform_Windows__ &&
            target->architecture == __Bootstrap_Target_Architecture_AArch64__)
            __LLVM_Prepare_Windows_ARM64_Module__(emitter.module);
        if (LLVMTargetMachineEmitToFile(
                machine, emitter.module, (char *)path, LLVMObjectFile, &message))
        {
            __LLVM_Fail_Message__("LLVM native object emission failed", message);
            message = NULL;
        }
        else
        {
            ok = 1;
        }
    }
    if (message != NULL)
        LLVMDisposeMessage(message);
    __LLVM_Dispose_Program_Module__(&emitter, machine, data);
    if (semantic != NULL && saved_unit != NULL)
        semantic->__Active_Unit__ = saved_unit;
    return ok;
}
