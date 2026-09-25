/* Declares the LLVM C API used by Stage0. */

#ifndef SULTANC_BOOTSTRAP_LLVM_C_API_H
/* Defines the sultanc bootstrap LLVM c API h macro. */
#define SULTANC_BOOTSTRAP_LLVM_C_API_H

#include <stddef.h>

#if defined(__has_include)
#if __has_include(<llvm-c/Core.h>) && __has_include(<llvm-c/Analysis.h>) && \
      __has_include(<llvm-c/Target.h>) && __has_include(<llvm-c/TargetMachine.h>) && \
      __has_include(<llvm-c/ExecutionEngine.h>)
/* Defines the sultanc bootstrap has LLVM headers macro. */
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

/* Defines the LLVM bool type alias. */
typedef int LLVMBool;

/* Defines the LLVM context ref structure. */
typedef struct LLVMOpaqueContext *LLVMContextRef;
/* Defines the LLVM module ref structure. */
typedef struct LLVMOpaqueModule *LLVMModuleRef;
/* Defines the LLVM type ref structure. */
typedef struct LLVMOpaqueType *LLVMTypeRef;
/* Defines the LLVM value ref structure. */
typedef struct LLVMOpaqueValue *LLVMValueRef;
/* Defines the LLVM basic block ref structure. */
typedef struct LLVMOpaqueBasicBlock *LLVMBasicBlockRef;
/* Defines the LLVM builder ref structure. */
typedef struct LLVMOpaqueBuilder *LLVMBuilderRef;
/* Defines the LLVM target ref structure. */
typedef struct LLVMTarget *LLVMTargetRef;
/* Defines the LLVM target machine ref structure. */
typedef struct LLVMOpaqueTargetMachine *LLVMTargetMachineRef;
/* Defines the LLVM target data ref structure. */
typedef struct LLVMOpaqueTargetData *LLVMTargetDataRef;
/* Defines the LLVM execution engine ref structure. */
typedef struct LLVMOpaqueExecutionEngine *LLVMExecutionEngineRef;

/* Defines the LLVM verifier failure action values. */
typedef enum
{
    /* Represents the LLVM abort process action value. */
    LLVMAbortProcessAction,
    /* Represents the LLVM print message action value. */
    LLVMPrintMessageAction,
    /* Represents the LLVM return status action value. */
    LLVMReturnStatusAction
} LLVMVerifierFailureAction;

/* Defines the LLVM code gen opt level values. */
typedef enum
{
    /* Represents the LLVM code gen level none value. */
    LLVMCodeGenLevelNone,
    /* Represents the LLVM code gen level less value. */
    LLVMCodeGenLevelLess,
    /* Represents the LLVM code gen level default value. */
    LLVMCodeGenLevelDefault,
    /* Represents the LLVM code gen level aggressive value. */
    LLVMCodeGenLevelAggressive
} LLVMCodeGenOptLevel;

/* Defines the LLVM reloc mode values. */
typedef enum
{
    /* Represents the LLVM reloc default value. */
    LLVMRelocDefault,
    /* Represents the LLVM reloc static value. */
    LLVMRelocStatic,
    /* Represents the LLVM reloc pic value. */
    LLVMRelocPIC,
    /* Represents the LLVM reloc dynamic no pic value. */
    LLVMRelocDynamicNoPic,
    /* Represents the LLVM reloc ropi value. */
    LLVMRelocROPI,
    /* Represents the LLVM reloc rwpi value. */
    LLVMRelocRWPI,
    /* Represents the LLVM reloc ropi rwpi value. */
    LLVMRelocROPI_RWPI
} LLVMRelocMode;

/* Defines the LLVM code model values. */
typedef enum
{
    /* Represents the LLVM code model default value. */
    LLVMCodeModelDefault,
    /* Represents the LLVM code model JIT default value. */
    LLVMCodeModelJITDefault,
    /* Represents the LLVM code model tiny value. */
    LLVMCodeModelTiny,
    /* Represents the LLVM code model small value. */
    LLVMCodeModelSmall,
    /* Represents the LLVM code model kernel value. */
    LLVMCodeModelKernel,
    /* Represents the LLVM code model medium value. */
    LLVMCodeModelMedium,
    /* Represents the LLVM code model large value. */
    LLVMCodeModelLarge
} LLVMCodeModel;

/* Defines the LLVM code gen file type values. */
typedef enum
{
    /* Represents the LLVM assembly file value. */
    LLVMAssemblyFile,
    /* Represents the LLVM object file value. */
    LLVMObjectFile
} LLVMCodeGenFileType;

/* Defines the LLVM int predicate values. */
typedef enum
{
    /* Represents the LLVM int eq value. */
    LLVMIntEQ = 32,
    /* Represents the LLVM int ne value. */
    LLVMIntNE = 33,
    /* Represents the LLVM int ugt value. */
    LLVMIntUGT = 34,
    /* Represents the LLVM int uge value. */
    LLVMIntUGE = 35,
    /* Represents the LLVM int ult value. */
    LLVMIntULT = 36,
    /* Represents the LLVM int ule value. */
    LLVMIntULE = 37,
    /* Represents the LLVM int sgt value. */
    LLVMIntSGT = 38,
    /* Represents the LLVM int sge value. */
    LLVMIntSGE = 39,
    /* Represents the LLVM int slt value. */
    LLVMIntSLT = 40,
    /* Represents the LLVM int sle value. */
    LLVMIntSLE = 41
} LLVMIntPredicate;

/* Creates the LLVM context. */
extern LLVMContextRef LLVMContextCreate(void);

/* Releases the LLVM context. */
extern void LLVMContextDispose(LLVMContextRef C);

/* Creates the LLVM module with name in context. */
extern LLVMModuleRef LLVMModuleCreateWithNameInContext(const char *ModuleID, LLVMContextRef C);

/* Releases the LLVM module. */
extern void LLVMDisposeModule(LLVMModuleRef M);

/* Creates the LLVM builder in context. */
extern LLVMBuilderRef LLVMCreateBuilderInContext(LLVMContextRef C);

/* Releases the LLVM builder. */
extern void LLVMDisposeBuilder(LLVMBuilderRef Builder);

/* Finishes the LLVM position builder at. */
extern void LLVMPositionBuilderAtEnd(LLVMBuilderRef Builder, LLVMBasicBlockRef Block);

/* Returns the LLVM int type in context. */
extern LLVMTypeRef LLVMIntTypeInContext(LLVMContextRef C, unsigned NumBits);

/* Returns the LLVM void type in context. */
extern LLVMTypeRef LLVMVoidTypeInContext(LLVMContextRef C);

/* Creates the LLVM struct named. */
extern LLVMTypeRef LLVMStructCreateNamed(LLVMContextRef C, const char *Name);

/* Returns the LLVM struct type in context. */
extern LLVMTypeRef LLVMStructTypeInContext(LLVMContextRef C,
                                           LLVMTypeRef *ElementTypes,
                                           unsigned ElementCount,
                                           LLVMBool Packed);

/* Returns the LLVM array type. */
extern LLVMTypeRef LLVMArrayType(LLVMTypeRef ElementType, unsigned ElementCount);

/* Returns the LLVM pointer type. */
extern LLVMTypeRef LLVMPointerType(LLVMTypeRef ElementType, unsigned AddressSpace);

/* Sets the LLVM struct body. */
extern void LLVMStructSetBody(LLVMTypeRef StructTy,
                              LLVMTypeRef *ElementTypes,
                              unsigned ElementCount,
                              LLVMBool Packed);

/* Returns the LLVM function type. */
extern LLVMTypeRef LLVMFunctionType(LLVMTypeRef ReturnType,
                                    LLVMTypeRef *ParamTypes,
                                    unsigned ParamCount,
                                    LLVMBool IsVarArg);

/* Adds the LLVM function. */
extern LLVMValueRef LLVMAddFunction(LLVMModuleRef M, const char *Name, LLVMTypeRef FunctionTy);

/* Returns the LLVM named function. */
extern LLVMValueRef LLVMGetNamedFunction(LLVMModuleRef M, const char *Name);

/* Adds the LLVM global. */
extern LLVMValueRef LLVMAddGlobal(LLVMModuleRef M, LLVMTypeRef Ty, const char *Name);

/* Sets the LLVM initializer. */
extern void LLVMSetInitializer(LLVMValueRef GlobalVar, LLVMValueRef ConstantVal);

/* Sets the LLVM global constant. */
extern void LLVMSetGlobalConstant(LLVMValueRef GlobalVar, LLVMBool IsConstant);

/* Returns the LLVM param. */
extern LLVMValueRef LLVMGetParam(LLVMValueRef Fn, unsigned Index);

/* Appends the LLVM basic block in context. */
extern LLVMBasicBlockRef
LLVMAppendBasicBlockInContext(LLVMContextRef C, LLVMValueRef Fn, const char *Name);

/* Returns the LLVM const int. */
extern LLVMValueRef LLVMConstInt(LLVMTypeRef IntTy, unsigned long long N, LLVMBool SignExtend);

/* Returns the LLVM const null. */
extern LLVMValueRef LLVMConstNull(LLVMTypeRef Ty);

/* Returns the LLVM const string in context. */
extern LLVMValueRef LLVMConstStringInContext(LLVMContextRef C,
                                             const char *Str,
                                             unsigned Length,
                                             LLVMBool DontNullTerminate);

/* Builds the LLVM alloca. */
extern LLVMValueRef LLVMBuildAlloca(LLVMBuilderRef, LLVMTypeRef Ty, const char *Name);

/* Builds the LLVM store. */
extern LLVMValueRef LLVMBuildStore(LLVMBuilderRef, LLVMValueRef Val, LLVMValueRef Ptr);

/* Builds the LLVM load 2. */
extern LLVMValueRef
LLVMBuildLoad2(LLVMBuilderRef, LLVMTypeRef Ty, LLVMValueRef PointerVal, const char *Name);

/* Builds the LLVM struct gep 2. */
extern LLVMValueRef LLVMBuildStructGEP2(
    LLVMBuilderRef B, LLVMTypeRef Ty, LLVMValueRef Pointer, unsigned Idx, const char *Name);

/* Builds the LLVM gep 2. */
extern LLVMValueRef LLVMBuildGEP2(LLVMBuilderRef B,
                                  LLVMTypeRef Ty,
                                  LLVMValueRef Pointer,
                                  LLVMValueRef *Indices,
                                  unsigned NumIndices,
                                  const char *Name);

/* Builds the LLVM extract value. */
extern LLVMValueRef
LLVMBuildExtractValue(LLVMBuilderRef, LLVMValueRef AggVal, unsigned Index, const char *Name);

/* Builds the LLVM insert value. */
extern LLVMValueRef LLVMBuildInsertValue(
    LLVMBuilderRef, LLVMValueRef AggVal, LLVMValueRef EltVal, unsigned Index, const char *Name);

/* Builds the LLVM select. */
extern LLVMValueRef LLVMBuildSelect(
    LLVMBuilderRef, LLVMValueRef If, LLVMValueRef Then, LLVMValueRef Else, const char *Name);

/* Builds the LLVM mem cpy. */
extern LLVMValueRef LLVMBuildMemCpy(LLVMBuilderRef B,
                                    LLVMValueRef Dst,
                                    unsigned DstAlign,
                                    LLVMValueRef Src,
                                    unsigned SrcAlign,
                                    LLVMValueRef Size);

/* Builds the LLVM s ext. */
extern LLVMValueRef
LLVMBuildSExt(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);

/* Builds the LLVM z ext. */
extern LLVMValueRef
LLVMBuildZExt(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);

/* Builds the LLVM trunc. */
extern LLVMValueRef
LLVMBuildTrunc(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);

/* Builds the LLVM pointer to int. */
extern LLVMValueRef
LLVMBuildPtrToInt(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);

/* Builds the LLVM int to pointer. */
extern LLVMValueRef
LLVMBuildIntToPtr(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);

/* Builds the LLVM pointer cast. */
extern LLVMValueRef
LLVMBuildPointerCast(LLVMBuilderRef, LLVMValueRef Val, LLVMTypeRef DestTy, const char *Name);

/* Builds the LLVM add. */
extern LLVMValueRef
LLVMBuildAdd(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM sub. */
extern LLVMValueRef
LLVMBuildSub(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM mul. */
extern LLVMValueRef
LLVMBuildMul(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM s div. */
extern LLVMValueRef
LLVMBuildSDiv(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM u div. */
extern LLVMValueRef
LLVMBuildUDiv(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM s rem. */
extern LLVMValueRef
LLVMBuildSRem(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM u rem. */
extern LLVMValueRef
LLVMBuildURem(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM and. */
extern LLVMValueRef
LLVMBuildAnd(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM or. */
extern LLVMValueRef
LLVMBuildOr(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM xor. */
extern LLVMValueRef
LLVMBuildXor(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM shl. */
extern LLVMValueRef
LLVMBuildShl(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM l shr. */
extern LLVMValueRef
LLVMBuildLShr(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM a shr. */
extern LLVMValueRef
LLVMBuildAShr(LLVMBuilderRef, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM i cmp. */
extern LLVMValueRef LLVMBuildICmp(
    LLVMBuilderRef, LLVMIntPredicate Op, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name);

/* Builds the LLVM cond br. */
extern LLVMValueRef
LLVMBuildCondBr(LLVMBuilderRef, LLVMValueRef If, LLVMBasicBlockRef Then, LLVMBasicBlockRef Else);

/* Builds the LLVM br. */
extern LLVMValueRef LLVMBuildBr(LLVMBuilderRef, LLVMBasicBlockRef Dest);

/* Returns the LLVM basic block terminator. */
extern LLVMValueRef LLVMGetBasicBlockTerminator(LLVMBasicBlockRef BB);

/* Builds the LLVM unreachable. */
extern LLVMValueRef LLVMBuildUnreachable(LLVMBuilderRef);

/* Builds the LLVM call 2. */
extern LLVMValueRef LLVMBuildCall2(LLVMBuilderRef,
                                   LLVMTypeRef,
                                   LLVMValueRef Fn,
                                   LLVMValueRef *Args,
                                   unsigned NumArgs,
                                   const char *Name);

/* Returns the LLVM insert block. */
extern LLVMBasicBlockRef LLVMGetInsertBlock(LLVMBuilderRef Builder);

/* Returns the LLVM basic block parent. */
extern LLVMValueRef LLVMGetBasicBlockParent(LLVMBasicBlockRef BB);

/* Finds the LLVM intrinsic ID. */
extern unsigned LLVMLookupIntrinsicID(const char *Name, size_t NameLen);

/* Returns the LLVM intrinsic declaration. */
extern LLVMValueRef LLVMGetIntrinsicDeclaration(LLVMModuleRef Mod,
                                                unsigned ID,
                                                LLVMTypeRef *ParamTypes,
                                                size_t ParamCount);

/* Builds the LLVM return. */
extern LLVMValueRef LLVMBuildRet(LLVMBuilderRef, LLVMValueRef V);

/* Builds the LLVM return void. */
extern LLVMValueRef LLVMBuildRetVoid(LLVMBuilderRef);

/* Returns the LLVM verify module. */
extern LLVMBool
LLVMVerifyModule(LLVMModuleRef M, LLVMVerifierFailureAction Action, char **OutMessage);

/* Returns the LLVM default target triple. */
extern char *LLVMGetDefaultTargetTriple(void);

/* Sets the LLVM target. */
extern void LLVMSetTarget(LLVMModuleRef M, const char *Triple);

/* Maps the triple to the LLVM get target. */
extern LLVMBool LLVMGetTargetFromTriple(const char *Triple, LLVMTargetRef *T, char **ErrorMessage);

/* Creates the LLVM target machine. */
extern LLVMTargetMachineRef LLVMCreateTargetMachine(LLVMTargetRef T,
                                                    const char *Triple,
                                                    const char *CPU,
                                                    const char *Features,
                                                    LLVMCodeGenOptLevel Level,
                                                    LLVMRelocMode Reloc,
                                                    LLVMCodeModel CodeModel);

/* Releases the LLVM target machine. */
extern void LLVMDisposeTargetMachine(LLVMTargetMachineRef T);

/* Creates the LLVM target data layout. */
extern LLVMTargetDataRef LLVMCreateTargetDataLayout(LLVMTargetMachineRef T);

/* Releases the LLVM target data. */
extern void LLVMDisposeTargetData(LLVMTargetDataRef TD);

/* Copies the LLVM string rep of target data. */
extern char *LLVMCopyStringRepOfTargetData(LLVMTargetDataRef TD);

/* Sets the LLVM data layout. */
extern void LLVMSetDataLayout(LLVMModuleRef M, const char *DataLayoutStr);

/* Emits the LLVM target machine to file. */
extern LLVMBool LLVMTargetMachineEmitToFile(LLVMTargetMachineRef T,
                                            LLVMModuleRef M,
                                            char *Filename,
                                            LLVMCodeGenFileType codegen,
                                            char **ErrorMessage);

/* Releases the LLVM message. */
extern void LLVMDisposeMessage(char *Message);

/* Returns the LLVM version. */
extern void LLVMGetVersion(unsigned *Major, unsigned *Minor, unsigned *Patch);

/* Links the LLVM in mcjit. */
extern void LLVMLinkInMCJIT(void);

/* Creates the LLVM execution engine for module. */
extern LLVMBool
LLVMCreateExecutionEngineForModule(LLVMExecutionEngineRef *OutEE, LLVMModuleRef M, char **OutError);

/* Runs the LLVM function as main. */
extern int LLVMRunFunctionAsMain(LLVMExecutionEngineRef EE,
                                 LLVMValueRef F,
                                 unsigned ArgC,
                                 const char *const *ArgV,
                                 const char *const *EnvP);

/* Releases the LLVM execution engine. */
extern void LLVMDisposeExecutionEngine(LLVMExecutionEngineRef EE);

/* Initializes the LLVM x 86 target info. */
extern void LLVMInitializeX86TargetInfo(void);

/* Initializes the LLVM x 86 target. */
extern void LLVMInitializeX86Target(void);

/* Initializes the LLVM x 86 target mc. */
extern void LLVMInitializeX86TargetMC(void);

/* Initializes the LLVM x 86 asm printer. */
extern void LLVMInitializeX86AsmPrinter(void);

/* Initializes the LLVM a arch 64 target info. */
extern void LLVMInitializeAArch64TargetInfo(void);

/* Initializes the LLVM a arch 64 target. */
extern void LLVMInitializeAArch64Target(void);

/* Initializes the LLVM a arch 64 target mc. */
extern void LLVMInitializeAArch64TargetMC(void);

/* Initializes the LLVM a arch 64 asm printer. */
extern void LLVMInitializeAArch64AsmPrinter(void);
#endif

#endif
