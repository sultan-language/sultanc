/* Runs the Stage0 bootstrap compiler command-line entry point. */

#include "llvm/llvm_emitter.h"
#include "core/program.h"
#include "semantic/body.h"
#include "semantic/check.h"
#include "semantic/context_collection.h"
#include "semantic/context_lifecycle.h"
#include "core/diagnostic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    /* References the run argv. */
    const char **run_argv = NULL;
    /* Stores the run argc. */
    int run_argc = 0;
    /* Stores the run start. */
    int run_start = -1;
    /* Stores the run mode. */
    int run_mode = 0;
    /* Stores the process status. */
    int process_status = 1;
    /* Tracks the loop index. */
    int i;
    /* Tracks whether the operation succeeded. */
    int ok = 0;
    /* Stores the program. */
    __Program__ program;
    /* Stores the semantic context. */
    __Semantic_Context__ sem;
    /* Tracks the program ready state. */
    int program_ready = 0;
    /* Tracks the semantic context ready state. */
    int sem_ready = 0;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <input.sn> -o <output.o>\n", argv[0]);
        fprintf(stderr, "       %s <compiler.sn> --run <compiler-args...>\n", argv[0]);
        return 2;
    }

    input = argv[1];
    for (i = 2; i < argc; ++i)
    {
        if (strcmp(argv[i], "--run") == 0)
        {
            run_mode = 1;
            run_start = i + 1;
            break;
        }
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            output = argv[++i];
        }
        else if (strcmp(argv[i], "--emit-llvm-object") == 0 && i + 1 < argc)
        {
            /* Compatibility spelling: still the same single LLVM object-output path. */
            output = argv[++i];
        }
        else
        {
            fprintf(stderr, "sultanc-stage0: unknown argument: %s\n", argv[i]);
            return 2;
        }
    }

    if (!run_mode && !output)
    {
        fprintf(stderr, "sultanc-stage0: missing -o <output.o>\n");
        return 2;
    }

    __Program_Init__(&program);
    program_ready = 1;
    if (!__Program_Load_Root__(&program, input))
    {
        print_diag("load", __Program_Diagnostic__(&program));
        goto done;
    }

    __Semantic_Context_Init__(&sem, &program);
    sem_ready = 1;
    if (!__Semantic_Collect_Globals__(&sem) || !__Semantic_Check_Program__(&sem) ||
        !__Semantic_Check_Bodies__(&sem))
    {
        print_diag("semantic", &sem.__Diagnostic__);
        goto done;
    }

    if (run_mode)
    {
        /* Stores the user count. */
        int user_count = argc - run_start;
        run_argc = user_count + 1;
        run_argv = (const char **)calloc((size_t)run_argc, sizeof(*run_argv));
        if (run_argv == NULL)
        {
            fprintf(stderr, "sultanc-stage0: unable to allocate JIT argument vector\n");
            goto done;
        }
        run_argv[0] = "sultanc-stage1-bootstrap";
        for (i = 0; i < user_count; ++i)
            run_argv[i + 1] = argv[run_start + i];
        if (!__Bootstrap_Run_LLVM_Program__(&sem, run_argc, run_argv, &process_status))
        {
            fprintf(stderr,
                    "sultanc-stage0: LLVM execution failed: %s\n",
                    __Bootstrap_LLVM_Emitter_Error__());
            goto done;
        }
    }
    else
    {
        if (!__Bootstrap_Emit_LLVM_Object__(&sem, output))
        {
            fprintf(stderr,
                    "sultanc-stage0: LLVM object emission failed: %s\n",
                    __Bootstrap_LLVM_Emitter_Error__());
            goto done;
        }
        process_status = 0;
    }

    ok = 1;

done:
    free(run_argv);
    if (sem_ready)
        __Semantic_Context_Destroy__(&sem);
    if (program_ready)
        __Program_Destroy__(&program);
    return ok ? process_status : 1;
}
