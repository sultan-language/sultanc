/* Declares the canonical Stage0 bootstrap target model. */

#ifndef SULTANC_BOOTSTRAP_TARGET_BOOTSTRAP_TARGET_H
#define SULTANC_BOOTSTRAP_TARGET_BOOTSTRAP_TARGET_H

#include "llvm/llvm_c_api.h"
#include <stddef.h>

/* Identifies the target architecture facts consumed by Stage0 runtime lowering. */
typedef enum
{
    __Bootstrap_Target_Architecture_Unknown__,
    __Bootstrap_Target_Architecture_AArch64__,
    __Bootstrap_Target_Architecture_X86_64__,
    __Bootstrap_Target_Architecture_RISCV64__
} __Bootstrap_Target_Architecture__;

/* Identifies the target operating-system facts consumed by Stage0 runtime lowering. */
typedef enum
{
    __Bootstrap_Target_Platform_Unknown__,
    __Bootstrap_Target_Platform_Darwin__,
    __Bootstrap_Target_Platform_Linux__,
    __Bootstrap_Target_Platform_Windows__
} __Bootstrap_Target_Platform__;

/* Identifies the target ABI environment facts consumed by Stage0 runtime lowering. */
typedef enum
{
    __Bootstrap_Target_Environment_Unknown__,
    __Bootstrap_Target_Environment_GNU__,
    __Bootstrap_Target_Environment_Musl__,
    __Bootstrap_Target_Environment_MSVC__
} __Bootstrap_Target_Environment__;

/* Identifies the object format implied by the normalized target. */
typedef enum
{
    __Bootstrap_Target_Object_Unknown__,
    __Bootstrap_Target_Object_ELF__,
    __Bootstrap_Target_Object_MachO__,
    __Bootstrap_Target_Object_COFF__,
    __Bootstrap_Target_Object_Wasm__
} __Bootstrap_Target_Object_Format__;

/* Identifies the custom Stage1 executable finalizers that actually exist. */
typedef enum
{
    __Bootstrap_Target_Finalizer_Unsupported__,
    __Bootstrap_Target_Finalizer_MachO_ARM64__,
    __Bootstrap_Target_Finalizer_ELF_X86_64__
} __Bootstrap_Target_Finalizer__;

/* Stores one normalized bootstrap compilation target. */
typedef struct
{
    /* Owns the normalized LLVM target triple. */
    char *triple;
    /* Stores the architecture identity needed by bootstrap runtime semantics. */
    __Bootstrap_Target_Architecture__ architecture;
    /* Stores the operating-system identity needed by bootstrap runtime semantics. */
    __Bootstrap_Target_Platform__ platform;
    /* Stores the ABI environment identity needed by bootstrap runtime semantics. */
    __Bootstrap_Target_Environment__ environment;
    /* Stores the object format implied by the target identity. */
    __Bootstrap_Target_Object_Format__ object_format;
    /* Tracks whether the target came from an explicit user request. */
    int explicit_target;
} __Bootstrap_Target__;

/* Initializes one canonical target from an optional Sultan alias or LLVM triple. */
int __Bootstrap_Target_Init__(__Bootstrap_Target__ *target,
                              const char *requested,
                              char *error,
                              size_t error_size);

/* Releases one canonical bootstrap target. */
void __Bootstrap_Target_Destroy__(__Bootstrap_Target__ *target);

/* Returns whether the existing bootstrap runtime ABI is qualified for this target. */
int __Bootstrap_Target_Runtime_Qualified__(const __Bootstrap_Target__ *target);

/* Returns the existing custom Stage1 executable finalizer for this target. */
__Bootstrap_Target_Finalizer__
__Bootstrap_Target_Finalizer_Kind__(const __Bootstrap_Target__ *target);

#endif
