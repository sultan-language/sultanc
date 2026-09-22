#ifndef SULTANC_BOOTSTRAP_LLVM_C_API_H
#define SULTANC_BOOTSTRAP_LLVM_C_API_H

#include <stddef.h>

#if defined(__has_include)
#if __has_include(<llvm-c/Core.h>) && __has_include(<llvm-c/Analysis.h>) && \
      __has_include(<llvm-c/Target.h>) && __has_include(<llvm-c/TargetMachine.h>) && \
      __has_include(<llvm-c/ExecutionEngine.h>)
#define SULTANC_BOOTSTRAP_HAS_LLVM_HEADERS 1
#endif
#endif

#ifdef SULTANC_BOOTSTRAP_HAS_LLVM_HEADERS
#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#else

typedef int LLVMBool;

typedef struct LLVMOpaqueContext *LLVMContextRef;
typedef struct LLVMOpaqueModule *LLVMModuleRef;
typedef struct LLVMOpaqueType *LLVMTypeRef;
typedef struct LLVMOpaqueValue *LLVMValueRef;
typedef struct LLVMOpaqueBasicBlock *LLVMBasicBlockRef;
typedef struct LLVMOpaqueBuilder *LLVMBuilderRef;
typedef struct LLVMTarget *LLVMTargetRef;
typedef struct LLVMOpaqueTargetMachine *LLVMTargetMachineRef;
typedef struct LLVMOpaqueTargetData *LLVMTargetDataRef;
typedef struct LLVMOpaqueExecutionEngine *LLVMExecutionEngineRef;

typedef enum
{
    LLVMAbortProcessAction,
    LLVMPrintMessageAction,
    LLVMReturnStatusAction
} LLVMVerifierFailureAction;

typedef enum
{
    LLVMCodeGenLevelNone,
    LLVMCodeGenLevelLess,
    LLVMCodeGenLevelDefault,
    LLVMCodeGenLevelAggressive
} LLVMCodeGenOptLevel;

typedef enum
{
    LLVMRelocDefault,
    LLVMRelocStatic,
    LLVMRelocPIC,
    LLVMRelocDynamicNoPic,
    LLVMRelocROPI,
    LLVMRelocRWPI,
    LLVMRelocROPI_RWPI
} LLVMRelocMode;

typedef enum
{
    LLVMCodeModelDefault,
    LLVMCodeModelJITDefault,
    LLVMCodeModelTiny,
    LLVMCodeModelSmall,
    LLVMCodeModelKernel,
    LLVMCodeModelMedium,
    LLVMCodeModelLarge
} LLVMCodeModel;

typedef enum
{
    LLVMAssemblyFile,
    LLVMObjectFile
} LLVMCodeGenFileType;

typedef enum
{
    LLVMIntEQ = 32,
    LLVMIntNE = 33,
    LLVMIntUGT = 34,
    LLVMIntUGE = 35,
    LLVMIntULT = 36,
    LLVMIntULE = 37,
    LLVMIntSGT = 38,
    LLVMIntSGE = 39,
    LLVMIntSLT = 40,
    LLVMIntSLE = 41
} LLVMIntPredicate;

extern LLVMContextRef LLVMContextCreate(void);

extern void LLVMContextDispose(LLVMContextRef C);

extern LLVMModuleRef LLVMModuleCreateWithNameInContext(const char *ModuleID, LLVMContextRef C);

extern void LLVMDisposeModule(LLVMModuleRef M);

extern LLVMBuilderRef LLVMCreateBuilderInContext(LLVMContextRef C);

extern void LLVMDisposeBuilder(LLVMBuilderRef Builder);

extern void LLVMPositionBuilderAtEnd(LLVMBuilderRef Builder, LLVMBasicBlockRef Block);

extern LLVMTypeRef LLVMIntTypeInContext(LLVMContextRef C, unsigned NumBits);

extern LLVMTypeRef LLVMVoidTypeInContext(LLVMContextRef C);

extern LLVMTypeRef LLVMStructCreateNamed(LLVMContextRef C, const char *Name);

extern LLVMTypeRef LLVMStructTypeInContext(LLVMContextRef C,
                                           LLVMTypeRef *ElementTypes,
                                           unsigned ElementCount,
                                           LLVMBool Packed);

extern LLVMTypeRef LLVMArrayType(LLVMTypeRef ElementType, unsigned ElementCount);

extern LLVMTypeRef LLVMPointerType(LLVMTypeRef ElementType, unsigned AddressSpace);

extern void LLVMStructSetBody(LLVMTypeRef StructTy,
                              LLVMTypeRef *ElementTypes,
                              unsigned ElementCount,
                              LLVMBool Packed);

extern LLVMTypeRef LLVMFunctionType(LLVMTypeRef ReturnType,
                                    LLVMTypeRef *ParamTypes,
                                    unsigned ParamCount,
                                    LLVMBool IsVarArg);

extern LLVMValueRef LLVMAddFunction(LLVMModuleRef M, const char *Name, LLVMTypeRef FunctionTy);

extern LLVMValueRef LLVMGetNamedFunction(LLVMModuleRef M, const char *Name);

extern LLVMValueRef LLVMAddGlobal(LLVMModuleRef M, LLVMTypeRef Ty, const char *Name);

extern void LLVMSetInitializer(LLVMValueRef GlobalVar, LLVMValueRef ConstantVal);

extern void LLVMSetGlobalConstant(LLVMValueRef GlobalVar, LLVMBool IsConstant);

extern LLVMValueRef LLVMGetParam(LLVMValueRef Fn, unsigned Index);

extern LLVMBasicBlockRef
LLVMAppendBasicBlockInContext(LLVMContextRef C, LLVMValueRef Fn, const char *Name);

extern LLVMValueRef LLVMConstInt(LLVMTypeRef IntTy, unsigned long long N, LLVMBool SignExtend);

extern LLVMValueRef LLVMConstNull(LLVMTypeRef Ty);

extern LLVMValueRef LLVMConstStringInContext(LLVMContextRef C,
                                             const char *Str,
                                             unsigned Length,
                                             LLVMBool DontNullTerminate);

extern LLVMValueRef LLVMBuildAlloca(LLVMBuilderRef, LLVMTypeRef Ty, const char *Name);

extern LLVMValueRef LLVMBuildStore(LLVMBuilderRef, LLVMValueRef Val, LLVMValueRef Ptr);

extern LLVMValueRef
LLVMBuildLoad2(LLVMBuilderRef, LLVMTypeRef Ty, LLVMValueRef PointerVal, const char *Name);

extern LLVMValueRef LLVMBuildStructGEP2(
    LLVMBuilderRef B, LLVMTypeRef Ty, LLVMValueRef Pointer, unsigned Idx, const char *Name);

extern LLVMValueRef LLVMBuildGEP2(LLVMBuilderRef B,
                                  LLVMTypeRef Ty,
                                  LLVMValueRef Pointer,
                                  LLVMValueRef *Indices,
                                  unsigned NumIndices,
                                  const char *Name);

extern LLVMValueRef
LLVMBuildExtractValue(LLVMBuilderRef, LLVMValueRef AggVal, unsigned Index, const char *Name);

extern LLVMValueRef LLVMBuildInsertValue(
    LLVMBuilderRef, LLVMValueRef AggVal, LLVMValueRef EltVal, unsigned Index, const char *Name);

extern LLVMValueRef LLVMBuildSelect(
    LLVMBuilderRef, LLVMValueRef If, LLVMValueRef Then, LLVMValueRef Else, const char *Name);

extern LLVMValueRef LLVMBuildMemCpy(LLVMBuilderRef B,
                                    LLVMValueRef Dst,
                                    unsigned DstAlign,
                                    LLVMValueRef Src,
                                    unsigned SrcAlign,
                                    LLVMValueRef Size);

extern LLVMValueRef
LLVMBuildSExt(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);

extern LLVMValueRef
LLVMBuildZExt(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);

extern LLVMValueRef
LLVMBuildTrunc(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);

extern LLVMValueRef
LLVMBuildPointerCast(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);

extern LLVMValueRef
LLVMBuildAdd(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildSub(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildMul(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildSDiv(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildUDiv(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildSRem(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildURem(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildAnd(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildOr(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildXor(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildShl(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildLShr(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildAShr(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef LLVMBuildICmp(
    LLVMBuilderRef, LLVMIntPredicate Op, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

extern LLVMValueRef
LLVMBuildCondBr(LLVMBuilderRef, LLVMValueRef If, LLVMBasicBlockRef Then, LLVMBasicBlockRef Else);

extern LLVMValueRef LLVMBuildBr(LLVMBuilderRef, LLVMBasicBlockRef Dest);

extern LLVMValueRef LLVMGetBasicBlockTerminator(LLVMBasicBlockRef BB);

extern LLVMValueRef LLVMBuildUnreachable(LLVMBuilderRef);

extern LLVMValueRef LLVMBuildCall2(LLVMBuilderRef,
                                   LLVMTypeRef,
                                   LLVMValueRef Fn,
                                   LLVMValueRef *Args,
                                   unsigned NumArgs,
                                   const char *Name);

extern LLVMBasicBlockRef LLVMGetInsertBlock(LLVMBuilderRef Builder);

extern LLVMValueRef LLVMGetBasicBlockParent(LLVMBasicBlockRef BB);

extern unsigned LLVMLookupIntrinsicID(const char *Name, size_t NameLen);

extern LLVMValueRef LLVMGetIntrinsicDeclaration(LLVMModuleRef Mod,
                                                unsigned ID,
                                                LLVMTypeRef *ParamTypes,
                                                size_t ParamCount);

extern LLVMValueRef LLVMBuildRet(LLVMBuilderRef, LLVMValueRef V);

extern LLVMValueRef LLVMBuildRetVoid(LLVMBuilderRef);

extern LLVMBool
LLVMVerifyModule(LLVMModuleRef M, LLVMVerifierFailureAction Action, char **OutMessage);

extern char *LLVMGetDefaultTargetTriple(void);

extern void LLVMSetTarget(LLVMModuleRef M, const char *Triple);

extern LLVMBool LLVMGetTargetFromTriple(const char *Triple, LLVMTargetRef *T, char **ErrorMessage);

extern LLVMTargetMachineRef LLVMCreateTargetMachine(LLVMTargetRef T,
                                                    const char *Triple,
                                                    const char *CPU,
                                                    const char *Features,
                                                    LLVMCodeGenOptLevel Level,
                                                    LLVMRelocMode Reloc,
                                                    LLVMCodeModel CodeModel);

extern void LLVMDisposeTargetMachine(LLVMTargetMachineRef T);

extern LLVMTargetDataRef LLVMCreateTargetDataLayout(LLVMTargetMachineRef T);

extern void LLVMDisposeTargetData(LLVMTargetDataRef TD);

extern char *LLVMCopyStringRepOfTargetData(LLVMTargetDataRef TD);

extern void LLVMSetDataLayout(LLVMModuleRef M, const char *DataLayoutStr);

extern LLVMBool LLVMTargetMachineEmitToFile(LLVMTargetMachineRef T,
                                            LLVMModuleRef M,
                                            char *Filename,
                                            LLVMCodeGenFileType codegen,
                                            char **ErrorMessage);

extern void LLVMDisposeMessage(char *Message);

extern void LLVMGetVersion(unsigned *Major, unsigned *Minor, unsigned *Patch);

extern void LLVMLinkInMCJIT(void);

extern LLVMBool
LLVMCreateExecutionEngineForModule(LLVMExecutionEngineRef *OutEE, LLVMModuleRef M, char **OutError);

extern int LLVMRunFunctionAsMain(LLVMExecutionEngineRef EE,
                                 LLVMValueRef F,
                                 unsigned ArgC,
                                 const char *const *ArgV,
                                 const char *const *EnvP);

extern void LLVMDisposeExecutionEngine(LLVMExecutionEngineRef EE);

extern void LLVMInitializeX86TargetInfo(void);

extern void LLVMInitializeX86Target(void);

extern void LLVMInitializeX86TargetMC(void);

extern void LLVMInitializeX86AsmPrinter(void);

extern void LLVMInitializeAArch64TargetInfo(void);

extern void LLVMInitializeAArch64Target(void);

extern void LLVMInitializeAArch64TargetMC(void);

extern void LLVMInitializeAArch64AsmPrinter(void);
#endif

#endif
