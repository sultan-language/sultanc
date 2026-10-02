/* Owns canonical Stage0 target aliases, normalized triples, and bootstrap target policy. */

#include "target/bootstrap_target.h"

#include <stdio.h>
#include <string.h>

/* Records a target-model error. */
static int target_fail(char *error, size_t error_size, const char *message)
{
    if (error != NULL && error_size != 0U)
        snprintf(error, error_size, "%s", message != NULL ? message : "invalid bootstrap target");
    return 0;
}

/* Returns the canonical LLVM spelling for a stable Sultan target alias. */
static const char *target_alias(const char *requested)
{
    if (requested == NULL)
        return NULL;
    if (strcmp(requested, "arm64-darwin") == 0)
        return "aarch64-apple-darwin";
    if (strcmp(requested, "x86_64-linux") == 0)
        return "x86_64-unknown-linux-gnu";
    return requested;
}

/* Copies one normalized triple component into a fixed buffer. */
static int target_component(const char *triple,
                            unsigned component,
                            char *buffer,
                            size_t buffer_size)
{
    const char *start;
    const char *end;
    unsigned index = 0U;
    size_t length;

    if (triple == NULL || buffer == NULL || buffer_size == 0U)
        return 0;
    start = triple;
    while (index < component)
    {
        start = strchr(start, '-');
        if (start == NULL)
            return 0;
        ++start;
        ++index;
    }
    end = strchr(start, '-');
    length = end == NULL ? strlen(start) : (size_t)(end - start);
    if (length == 0U || length >= buffer_size)
        return 0;
    memcpy(buffer, start, length);
    buffer[length] = '\0';
    return 1;
}

/* Parses only the target identity facts SultanC actually consumes. */
static void target_classify(__Bootstrap_Target__ *target)
{
    char architecture[32];
    char platform[32];
    char environment[32];
    char format[32];

    if (target == NULL || target->triple == NULL)
        return;

    if (target_component(target->triple, 0U, architecture, sizeof(architecture)))
    {
        if (strcmp(architecture, "aarch64") == 0 || strcmp(architecture, "arm64") == 0)
            target->architecture = __Bootstrap_Target_Architecture_AArch64__;
        else if (strcmp(architecture, "x86_64") == 0 || strcmp(architecture, "amd64") == 0)
            target->architecture = __Bootstrap_Target_Architecture_X86_64__;
        else if (strcmp(architecture, "riscv64") == 0)
            target->architecture = __Bootstrap_Target_Architecture_RISCV64__;
        if (strncmp(architecture, "wasm", 4U) == 0)
            target->object_format = __Bootstrap_Target_Object_Wasm__;
    }

    if (target_component(target->triple, 2U, platform, sizeof(platform)))
    {
        if (strncmp(platform, "darwin", 6U) == 0 || strncmp(platform, "macos", 5U) == 0)
        {
            target->platform = __Bootstrap_Target_Platform_Darwin__;
            target->object_format = __Bootstrap_Target_Object_MachO__;
        }
        else if (strcmp(platform, "linux") == 0)
        {
            target->platform = __Bootstrap_Target_Platform_Linux__;
            target->object_format = __Bootstrap_Target_Object_ELF__;
        }
        else if (strcmp(platform, "windows") == 0 || strcmp(platform, "win32") == 0)
        {
            target->platform = __Bootstrap_Target_Platform_Windows__;
            target->object_format = __Bootstrap_Target_Object_COFF__;
        }
    }

    if (target_component(target->triple, 3U, environment, sizeof(environment)))
    {
        if (strncmp(environment, "gnu", 3U) == 0)
            target->environment = __Bootstrap_Target_Environment_GNU__;
        else if (strncmp(environment, "musl", 4U) == 0)
            target->environment = __Bootstrap_Target_Environment_Musl__;
        else if (strcmp(environment, "msvc") == 0)
            target->environment = __Bootstrap_Target_Environment_MSVC__;
    }

    if (target_component(target->triple, 4U, format, sizeof(format)))
    {
        if (strcmp(format, "elf") == 0)
            target->object_format = __Bootstrap_Target_Object_ELF__;
        else if (strcmp(format, "macho") == 0)
            target->object_format = __Bootstrap_Target_Object_MachO__;
        else if (strcmp(format, "coff") == 0)
            target->object_format = __Bootstrap_Target_Object_COFF__;
        else if (strcmp(format, "wasm") == 0)
            target->object_format = __Bootstrap_Target_Object_Wasm__;
    }
}

int __Bootstrap_Target_Init__(__Bootstrap_Target__ *target,
                              const char *requested,
                              char *error,
                              size_t error_size)
{
    const char *source;
    char *default_triple = NULL;

    if (target == NULL)
        return target_fail(error, error_size, "missing bootstrap target configuration");
    memset(target, 0, sizeof(*target));
    if (requested != NULL && requested[0] == '\0')
        return target_fail(error, error_size, "empty bootstrap target");

    target->explicit_target = requested != NULL;
    if (requested != NULL)
    {
        source = target_alias(requested);
    }
    else
    {
        default_triple = LLVMGetDefaultTargetTriple();
        if (default_triple == NULL || default_triple[0] == '\0')
        {
            if (default_triple != NULL)
                LLVMDisposeMessage(default_triple);
            return target_fail(error, error_size, "LLVM did not provide a default target triple");
        }
        source = default_triple;
    }

    target->triple = LLVMNormalizeTargetTriple(source);
    if (default_triple != NULL)
        LLVMDisposeMessage(default_triple);
    if (target->triple == NULL || target->triple[0] == '\0')
    {
        __Bootstrap_Target_Destroy__(target);
        return target_fail(error, error_size, "LLVM could not normalize the requested target triple");
    }
    target_classify(target);
    return 1;
}

void __Bootstrap_Target_Destroy__(__Bootstrap_Target__ *target)
{
    if (target == NULL)
        return;
    if (target->triple != NULL)
        LLVMDisposeMessage(target->triple);
    memset(target, 0, sizeof(*target));
}

int __Bootstrap_Target_Runtime_Qualified__(const __Bootstrap_Target__ *target)
{
    if (target == NULL)
        return 0;
    if (target->architecture == __Bootstrap_Target_Architecture_AArch64__ &&
        target->platform == __Bootstrap_Target_Platform_Darwin__)
        return 1;
    return target->architecture == __Bootstrap_Target_Architecture_X86_64__ &&
           target->platform == __Bootstrap_Target_Platform_Linux__ &&
           target->environment == __Bootstrap_Target_Environment_GNU__;
}

__Bootstrap_Target_Finalizer__
__Bootstrap_Target_Finalizer_Kind__(const __Bootstrap_Target__ *target)
{
    if (target == NULL)
        return __Bootstrap_Target_Finalizer_Unsupported__;
    if (target->architecture == __Bootstrap_Target_Architecture_AArch64__ &&
        target->platform == __Bootstrap_Target_Platform_Darwin__)
        return __Bootstrap_Target_Finalizer_MachO_ARM64__;
    if (target->architecture == __Bootstrap_Target_Architecture_X86_64__ &&
        target->platform == __Bootstrap_Target_Platform_Linux__ &&
        target->environment == __Bootstrap_Target_Environment_GNU__)
        return __Bootstrap_Target_Finalizer_ELF_X86_64__;
    return __Bootstrap_Target_Finalizer_Unsupported__;
}
