[العربية](../../ar/contributing/tests.md) | **English**

# Tests

SultanC's qualification harness is written in SultanC and lives in `tests/runner/`. It discovers test
programs under `tests/ar/` and `tests/en/`, runs them against a built compiler, and adds structural,
robustness, object-format and system checks of its own. This page explains how to run the suites, what
the runner's test directives mean, and how to add a test. Python, shell test scripts and TSV manifests
are not part of the harness; the shell scripts in `tools/tests/` are entry points and
interpreter-specific checks.

## Running the tests

Build the compiler first (`./build.sh`, see the [contributor guide](README.md)), then run everything
from the repository root.

```sh
bash tools/tests/ci_run.sh              # runner + iSpeak suite, as CI runs them
```

`tools/tests/ci_run.sh` builds `tests/runner/main.sn` with the compiler under test, runs it, runs the
iSpeak suite (`tools/tests/ispeak_test.sh`), and writes `build/tests-result.json`:

```json
{"executed": 0, "passed": 0, "failed": 0, "skipped": 0, "compile_only": 0,
 "target": "x86_64-linux", "errors": [],
 "suites": {"runner": {}, "ispeak": {}}}
```

The counts mean:

| Field | Meaning |
|---|---|
| `executed` | tests that ran, everything except skipped; `passed + failed + compile_only` |
| `passed` | executed tests that passed and were checked at run time where they have a run step |
| `compile_only` | run tests that were only compiled because the target is not the host; neither passed nor failed |
| `skipped` | tests not executed at all (wrong target, opt-in tests) |
| `errors` | one entry for each suite that did not build, exited non-zero, or left a missing or malformed summary |

`ci_run.sh` exits non-zero when any test fails, when a suite does not build or exits non-zero, and when
a summary is missing or malformed. A suite that cannot report counts as one more executed, failed test.

Environment variables:

| Variable | Effect |
|---|---|
| `SC` | compiler for `ci_run.sh` and `ispeak_test.sh` (default `./build/sultanc`, `build/sultanc.exe` on Windows) |
| `TARGET` | target of the suite; defaults to the host's target |
| `SULTANC_BIN` | compiler the runner tests (default `./build/sultanc`); `ci_run.sh` sets it from `SC` |
| `SULTANC_STAGE0_BIN` | a C Stage0 binary; enables the `// مرحلة0: نعم` comparison |

The runner alone, and the iSpeak suite alone:

```sh
./build/sultanc tests/runner/main.sn -o build/sultan-tests && ./build/sultan-tests
bash tools/tests/ispeak_test.sh [path-filter]       # SC=, TARGET= to change compiler and target
```

The runner prints a summary as its last line. `ناجح` includes the compile-only tests, which
`ci_run.sh` reports separately.

```text
اختبارات SultanC: الكلي=N ناجح=P فاشل=F متجاوز=S تجميع-فقط=C
```

The runner writes its scratch files under `build/tests-sultanc/`. `tests/` may contain only the
directories `ar`, `en` and `runner`; the runner fails on anything else.

## Test layout

Test cases are discovered recursively from `tests/ar/**` and `tests/en/**`. The directories
`tests/ar/ispeak` and `tests/en/ispeak` belong to `tools/tests/ispeak_test.sh`, because their benchmark
loops are too long for the interpreter path; a directory named `ispeak` anywhere else is an ordinary
test directory. Categories sit below each language root (`language`, `stdlib`, `runtime`, `frontend`,
`codegen`, `diagnostics`, `programs`, `torture` and others). Each `.sn` file describes itself in header
comments.

### Directives

A directive is a line `// <name>: <value>` and the runner finds it anywhere in the file, though by
convention the directives open the file. `// اختبار:` makes a file a test; files without it are helpers
or modules of other tests.

| Directive | Meaning |
|---|---|
| `// اختبار: قبول` | accept: `check` must succeed |
| `// اختبار: رفض` | reject: `check` must fail |
| `// اختبار: تشغيل` | run: build, run on the host, compare |
| `// اختبار: تفاضلي` | differential: a run test whose interpreted run must also agree with the native one |
| `// أهداف: arm64-darwin,x86_64-linux` | targets the test is built for; the runner knows these two |
| `// خروج: N` | expected exit status (default 0; 1 for a reject test) |
| `// تشخيص: SULTANC-…` | expected diagnostic code of a reject test, or the warning an accept test must report |
| `// خرج-تشخيص-عربي: <file>`, `// خرج-تشخيص-إنجليزي: <file>` | the exact human output of `check --lang ar\|en --color never`, stored in that file beside the test |
| `// خرج-قياسي: <text>`, `// خطأ-قياسي: <text>` | expected standard output or error of the run; `<فارغ>` means empty |
| `// خطأ-قياسي-ملف: <file>` | the exact standard error of the run, stored in a file beside the test |
| `// بيئة: NAME=value` | one environment assignment for the test program's runs, native and interpreted |
| `// تفاضلي: نعم` | on a differential test, also compare standard error between native and interpreted runs |
| `// نظير: tests/ar/…` | the Arabic or English counterpart of the test |
| `// مرحلة0: نعم` | the C Stage0 must agree with the compiler under test |

### What each kind checks

An accept test runs `check` on the file and expects the exit status to be `خروج`. It also looks for
the warning named by `تشخيص`, compares `خرج-قياسي` and `خطأ-قياسي` (so `// خطأ-قياسي: <فارغ>` states
that `check` reports nothing), and compares the per-language diagnostic files. When it lists targets,
it also compiles an object for each listed target.

A reject test expects `check` to fail with the status in `خروج` (1 by default), to print the code named
by `تشخيص`, and to match the per-language diagnostic files when it names them. The runner treats any
internal compiler failure code in the output as a failure of the test.

A run test compiles an object for each listed target, or for both known targets when it lists none. It
then builds an executable for the host and runs it, comparing the exit status and any expected output.
If the targets exclude the host, the test is only compiled, and the summary reports it as compile-only.

A differential test does everything a run test does and also runs the same source through the
interpreter (`sultanc interpret`). The status must equal the expected status and the native status, and
standard output must be identical. With `// تفاضلي: نعم`, standard error must be identical too. These
tests are the CI subset of the interpreter-versus-native comparison; the full sweep is
`tools/tests/interp_sweep.sh` ([Interpreter](../internals/interpreter.md)).

### Arabic and English pairs

SultanC has an Arabic and an English surface for the same language, so tests come in pairs: a file in
`tests/en/` and the same program in `tests/ar/`, each naming the other with `// نظير:`. The English
file of a passing pair is run by the runner together with its counterpart: both are interpreted and
must produce the same status and output, and both are compiled to objects for the known targets. A
pair of reject tests is two independent reject tests, each checked on its own. Keep both files in step
when one changes, and keep their relative paths the same. This is the English file of a pair; its
Arabic counterpart is `tests/ar/programs/sum.sn`:

```sultan
// اختبار: تشغيل
// خروج: 7
// خرج-قياسي: sum=7
// نظير: tests/ar/programs/sum.sn
import std.io.println;

def add(a: int, b: int): int {
    return a + b;
}

def main(): int {
    println("sum=7");
    return add(3, 4);
}
```

Tests that exist only in one language, such as ABI and layout checks, omit `نظير`.

### Stage0 agreement

Compiler and std sources must stay within the language the C Stage0 accepts
([Bootstrap](../internals/bootstrap.md)). A test that guards such a contract carries
`// مرحلة0: نعم`. When `SULTANC_STAGE0_BIN` names a Stage0 binary, the runner also compiles the file
with it for the host target. An accept test must be accepted, and a reject test must be rejected with
the expected diagnostic code in the output. Without a Stage0 binary the check is skipped.

## Adding a test

1. Choose the category directory that owns the behavior and create the file under `tests/en/`. Give it
   a descriptive snake_case name.
2. Write the header. Use `رفض` with a `تشخيص` code for a diagnostic, `قبول` for a program that must
   type-check, and `تشغيل` or `تفاضلي` for behavior. List both targets (`arm64-darwin,x86_64-linux`) for
   portable behavior.
3. Create the Arabic counterpart under `tests/ar/` at the same relative path, and add the `نظير` line
   to both.
4. For a diagnostic test, add the `خرج-تشخيص-عربي` and `خرج-تشخيص-إنجليزي` files when the exact text
   matters. The `tests/*/diagnostics/e2e` directories hold examples.
5. Run the test with the runner (`bash tools/tests/ci_run.sh`), and check that it fails when the
   behavior it guards is broken.

A new directory under `tests/` is rejected; place new tests inside `ar` or `en`.

## Checks owned by the runner

Besides the discovered tests, the runner has its own suites (`tests/runner/suites/`):

- compiler architecture: source ownership, forbidden imports between compiler trees, and the layout of
  `tests/`;
- CLI behavior and diagnostic transport;
- malformed-source robustness, including structured mutations, deep nesting, and the allocator's traps
  on bad releases;
- ELF and Mach-O structural checks, relocations, determinism and Darwin code-signature verification;
- network and TLS qualification programs, and a live HTTP/2 check;
- resident-set-size regression checks and optional self-host convergence.

## Optional expensive checks

These are skipped unless the environment variable is set to `1`.

- `SULTANC_TEST_RSS`: peak resident set size of the same compiler source through `check` (frontend and
  MSIR verification), object emission and executable emission. The regression-gate samples are encoded
  in `tests/runner/suites/system.sn`.
- `SULTANC_TEST_ALLOCATIONS`: on macOS, exact high-water allocation reports through `malloc_history`
  and `vmmap`. Only system inspection tools are used; the orchestration, thresholds, parsing and
  reporting are SultanC code.
- `SULTANC_TEST_CONVERGENCE`: Stage2 to Stage3 to Stage4 byte convergence.

## Other test tools

| Tool | Purpose |
|---|---|
| `tools/tests/ispeak_test.sh` | the iSpeak suite on one target; reads the same headers as the runner |
| `tools/tests/interp_sweep.sh` | builds and runs every example and run test natively and interpreted and compares them |
| `tools/tests/interp_ffi_check.sh` | the interpreter's dynamic C function bridge, natively, under wine and by import audit |
| `tools/tests/mac_interp_check.sh` | stdlib tests that meet host memory, natively and interpreted, on macOS |
| `tools/tests/freebsd_examples.sh` | the x86_64 examples through a ptrace system-call translator on Linux |

## See also

- [Contributor guide](README.md)
- [Examples](examples.md)
- [Interpreter](../internals/interpreter.md)
- [iSpeak](../internals/ispeak/README.md)
- [Bootstrap](../internals/bootstrap.md)
