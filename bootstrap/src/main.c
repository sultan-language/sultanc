/* Runs the Stage0 bootstrap compiler command-line entry point. */

#include "llvm/llvm_emitter.h"
#include "core/program.h"
#include "semantic/body.h"
#include "semantic/check.h"
#include "semantic/context_collection.h"
#include "semantic/context_lifecycle.h"
#include "core/diagnostic.h"
#include "target/bootstrap_target.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SULTANC_BOOTSTRAP_DEFAULT_TARGET
#define SULTANC_BOOTSTRAP_DEFAULT_TARGET ((const char *)0)
#endif

/* Returns the diagnostic argument. */
static const char *diagnostic_argument(const __Diagnostic__ *d, __Diagnostic_Argument_Key__ key)
{
    /* Tracks the loop index. */
    size_t i;

    if (d == NULL)
        return NULL;
    for (i = 0U; i < d->__Argument_Count__; ++i)
    {
        if (d->__Arguments__[i].__Key__ == key)
            return d->__Arguments__[i].__Value__;
    }
    return NULL;
}

/* Returns the diagnostic argument name. */
static const char *diagnostic_argument_name(__Diagnostic_Argument_Key__ key)
{
    switch (key)
    {
        case __Diagnostic_Argument_Value__:
            return "value";
        case __Diagnostic_Argument_Expected__:
            return "expected";
        case __Diagnostic_Argument_Found__:
            return "found";
        case __Diagnostic_Argument_Index__:
            return "index";
        case __Diagnostic_Argument_Length__:
            return "length";
        case __Diagnostic_Argument_Range__:
            return "range";
        case __Diagnostic_Argument_Pattern__:
            return "pattern";
        case __Diagnostic_Argument_Path__:
            return "path";
        case __Diagnostic_Argument_Function__:
            return "function";
        case __Diagnostic_Argument_None__:
            break;
    }
    return "detail";
}

/* Prints the diagnostic arguments. */
static void print_diagnostic_arguments(const __Diagnostic__ *d)
{
    /* Tracks the loop index. */
    size_t i;

    if (d == NULL)
        return;
    for (i = 0U; i < d->__Argument_Count__; ++i)
    {
        if (d->__Arguments__[i].__Key__ == __Diagnostic_Argument_Function__)
            continue;
        fprintf(stderr,
                "  %s: %s\n",
                diagnostic_argument_name(d->__Arguments__[i].__Key__),
                d->__Arguments__[i].__Value__);
    }
}

/* Prints the diagnostic name. */
static void print_diagnostic_name(const char *name)
{
    /* References the cursor. */
    const char *cursor = name;
    /* Stores the length. */
    size_t length;
    /* Tracks the loop index. */
    size_t i;

    if (cursor == NULL)
    {
        fputs("unknown diagnostic", stderr);
        return;
    }
    while (cursor[0] == '_' && cursor[1] == '_')
        cursor += 2;
    length = strlen(cursor);
    while (length >= 2U && cursor[length - 1U] == '_' && cursor[length - 2U] == '_')
    {
        length -= 2U;
    }
    for (i = 0U; i < length; ++i)
    {
        fputc(cursor[i] == '_' ? ' ' : cursor[i], stderr);
    }
}

/* Prints the diagnostic. */
static void print_diag(const char *phase, const __Diagnostic__ *d)
{
    /* References the code. */
    const char *code = d ? __Diagnostic_Code__(d->__Id__) : NULL;
    /* References the name. */
    const char *name = d ? __Diagnostic_Name__(d->__Id__) : NULL;
    /* References the function. */
    const char *function = diagnostic_argument(d, __Diagnostic_Argument_Function__);
    /* References the span. */
    const __Diagnostic_Source_Span__ *span = d ? &d->__Primary_Span__ : NULL;

    if (span != NULL && span->__Has_Source__)
    {
        fprintf(stderr,
                "%s:%u:%u: error[%s]: ",
                span->__Source_Name__,
                (unsigned)span->__Start__.__Line__,
                (unsigned)(span->__Start__.__Column__ + 1U),
                code ? code : "SULTANC-E????");
        print_diagnostic_name(name);
        fputc('\n', stderr);
        fprintf(stderr, "  phase: %s\n", phase);
        if (function != NULL && function[0] != '\0')
        {
            fprintf(stderr, "  function: %s\n", function);
        }
        print_diagnostic_arguments(d);
        fprintf(stderr,
                "  span: %u:%u - %u:%u\n",
                (unsigned)span->__Start__.__Line__,
                (unsigned)(span->__Start__.__Column__ + 1U),
                (unsigned)span->__End__.__Line__,
                (unsigned)(span->__End__.__Column__ + 1U));
        if (span->__Line_Text_Length__ != 0U)
        {
            fprintf(stderr, "%6u | %s\n", (unsigned)span->__Start__.__Line__, span->__Line_Text__);
        }
        return;
    }

    fprintf(stderr,
            "sultanc-stage0: %s failed%s%s%s",
            phase,
            code ? " [" : "",
            code ? code : "",
            code ? "]" : "");
    if (name != NULL)
    {
        fputs(": ", stderr);
        print_diagnostic_name(name);
    }
    fputc('\n', stderr);
    if (function != NULL && function[0] != '\0')
    {
        fprintf(stderr, "  function: %s\n", function);
    }
    print_diagnostic_arguments(d);
}

/* Runs the Stage0 bootstrap compiler. */
int main(int argc, char **argv)
{
    /* References the input path. */
    const char *input = NULL;
    /* References the output path. */
    const char *output = NULL;
    /* References the requested LLVM object target. */
    const char *target = SULTANC_BOOTSTRAP_DEFAULT_TARGET;
    /* Owns the one canonical compilation target used by loading and emission. */
    __Bootstrap_Target__ target_model;
    /* Stores a target-model error. */
    char target_error[256];
    /* Stores the program. */
    __Program__ program;
    /* Stores the semantic context. */
    __Semantic_Context__ sem;
    /* Tracks command-line arguments. */
    int argument_index;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <input.sn> [--target=<target>] -o <output.o>\n", argv[0]);
        return 2;
    }

    input = argv[1];
    for (argument_index = 2; argument_index < argc; ++argument_index)
    {
        if (strncmp(argv[argument_index], "--target=", 9U) == 0)
        {
            target = argv[argument_index] + 9U;
            if (target[0] == '\0')
            {
                fprintf(stderr, "sultanc-stage0: --target requires a value\n");
                return 2;
            }
        }
        else if (strcmp(argv[argument_index], "--target") == 0)
        {
            if (argument_index + 1 >= argc)
            {
                fprintf(stderr, "sultanc-stage0: --target requires a value\n");
                return 2;
            }
            target = argv[++argument_index];
        }
        else if (strcmp(argv[argument_index], "-o") == 0 && argument_index + 1 < argc)
        {
            output = argv[++argument_index];
        }
        else
        {
            fprintf(stderr, "sultanc-stage0: unknown argument: %s\n", argv[argument_index]);
            return 2;
        }
    }

    if (!output)
    {
        fprintf(stderr, "sultanc-stage0: missing -o <output.o>\n");
        return 2;
    }

    /* Target selection precedes loading: top-level target conditions select
     * imports and declarations, so loading and emission share one target. */
    target_error[0] = '\0';
    if (!__Bootstrap_Target_Init__(&target_model, target, target_error, sizeof(target_error)))
    {
        fprintf(stderr, "sultanc-stage0: invalid target: %s\n", target_error);
        return 1;
    }

    __Program_Init__(&program, &target_model);
    if (!__Program_Load_Root__(&program, input))
    {
        print_diag("load", __Program_Diagnostic__(&program));
        __Program_Destroy__(&program);
        __Bootstrap_Target_Destroy__(&target_model);
        return 1;
    }

    __Semantic_Context_Init__(&sem, &program);
    if (!__Semantic_Collect_Globals__(&sem) || !__Semantic_Check_Program__(&sem) ||
        !__Semantic_Check_Bodies__(&sem))
    {
        print_diag("semantic", &sem.__Diagnostic__);
        __Semantic_Context_Destroy__(&sem);
        __Program_Destroy__(&program);
        __Bootstrap_Target_Destroy__(&target_model);
        return 1;
    }

    if (!__Bootstrap_Emit_LLVM_Object__(&sem, output, &target_model))
    {
        fprintf(stderr,
                "sultanc-stage0: LLVM object emission failed: %s\n",
                __Bootstrap_LLVM_Emitter_Error__());
        __Semantic_Context_Destroy__(&sem);
        __Program_Destroy__(&program);
        __Bootstrap_Target_Destroy__(&target_model);
        return 1;
    }

    __Semantic_Context_Destroy__(&sem);
    __Program_Destroy__(&program);
    __Bootstrap_Target_Destroy__(&target_model);
    return 0;
}
