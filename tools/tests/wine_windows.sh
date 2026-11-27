#!/bin/sh
# Windows x86-64 checks on Linux, under Wine: no Windows runner needed.
#
#   tools/tests/wine_windows.sh build [SULTANC]   cross-build build/sultanc.exe
#   tools/tests/wine_windows.sh run ARGS...       run that compiler under Wine
#   tools/tests/wine_windows.sh exe FILE ARGS...  run any Windows program under Wine
#   tools/tests/wine_windows.sh smoke             the checks below, PASS or FAIL each
#   tools/tests/wine_windows.sh tests [SULTANC]   the whole test runner under Wine
#
# build: SULTANC (default build/sultanc) compiles compiler/main.sn for
# x86_64-windows-msvc with --backend=llvm when it has the LLVM backend (as the
# Windows CI's compiler is generated), else with the native backend; the
# result is Native-only (no LLVM-C.dll needed). Wine runs with a clean
# environment (UTF-8 locale, for Arabic file names) and its own prefix
# (build/wine/prefix): a large inherited
# environment can make Wine abort at start ("free(): invalid pointer"), and
# its crash debugger is turned off, so a crash ends the program at once.
# Only TARGET and the variables named SULTANC_* pass through (SULTANC_INTERPRETER=checked,
# SULTANC_INTERPRETER_SELECTOR=..., SULTANC_LANG).
#
# tests: SULTANC builds tests/runner/main.sn for Windows and the runner checks
# build\sultanc.exe as the Windows CI does (shell-free, about 20 minutes);
# the summary must end with 0 failed. Its own shell and env come from Linux.
set -eu
cd "$(dirname "$0")/../.."
out=build/wine
# build\sultanc.exe, as on the Windows runner: std is found at <bin>/../library.
exe=build/sultanc.exe

wine_bin() {
    for w in /usr/lib/wine/wine64 "$(command -v wine64 2>/dev/null || true)" "$(command -v wine 2>/dev/null || true)"; do
        if [ -n "$w" ] && [ -x "$w" ]; then echo "$w"; return; fi
    done
    echo "wine_windows.sh: Wine is not installed (apt-get install wine64)" >&2
    exit 2
}

in_wine() {
    w=$(wine_bin)
    mkdir -p "$out/prefix"
    # shellcheck disable=SC2046 # one word per SULTANC_* variable
    env -i HOME="${HOME:-/root}" PATH=/usr/bin:/bin LANG=C.UTF-8 WINEPREFIX="$PWD/$out/prefix" WINEDEBUG=-all \
        WINEDLLOVERRIDES=winedbg.exe=d $(env | grep '^SULTANC_\|^TARGET=' || true) "$w" "$@"
}

build() {
    sc=${1:-build/sultanc}
    [ -x "$sc" ] || { echo "wine_windows.sh: no compiler at $sc (./build.sh first)" >&2; exit 2; }
    backend=native
    if "$sc" version 2>/dev/null | grep -qi llvm; then backend=llvm; fi
    mkdir -p "$out"
    echo "$sc -O2 --backend=$backend --target x86_64-windows-msvc -> $exe"
    "$sc" -O2 --backend=$backend --target x86_64-windows-msvc \
        --module-source=compiler.backend.provider.llvm.llvm_codegen=compiler/backend/provider/llvm/unavailable.sn \
        -o "$exe" compiler/main.sn
}

# One check: NAME, expected exit status, then the compiler's arguments; the
# output must contain MARK (empty for none).
check() {
    name=$1 want=$2 mark=$3
    shift 3
    set +e
    got=$(in_wine "$exe" "$@" 2>&1)
    status=$?
    set -e
    if [ "$status" -eq "$want" ] && { [ -z "$mark" ] || printf '%s' "$got" | grep -qF -- "$mark"; }; then
        echo "PASS $name"
    else
        echo "FAIL $name (exit $status, want $want${mark:+, output with \"$mark\"})"
        printf '%s\n' "$got" | head -n 8 | sed 's/^/    /'
        failed=$((failed + 1))
    fi
}

smoke() {
    [ -f "$exe" ] || build
    failed=0
    check "run: println" 0 "Hello from SultanC" --lang en run examples/en/basics/hello.sn
    check "run: std on the machine engine" 42 "" --lang en run tests/en/stdlib/behavior/leb128.sn
    check "run: std on the checked interpreter" 42 "" --lang en run tests/en/stdlib/behavior/unicode_package.sn
    check "run: runtime check report" 1 "SULTANC-RUNTIME-PROGRAM-STOPPED" \
        --lang en --color never run tests/runner/fixtures/thread_trap/main_trap.sn
    check "test: native" 1 "FAIL exits_seven: exit status 7" \
        --lang en --color never test tests/runner/fixtures/testing/en/main.sn
    check "test: --interpret" 1 "PASS doubles" \
        --lang en --color never test --interpret tests/runner/fixtures/testing/en/main.sn
    check "check: iSpeak fixture" 1 "SULTANC-PARSE-EXPRESSION" \
        --lang en --color never check tests/en/ispeak/fixtures/invalid_expression.sn
    echo "wine_windows.sh: $failed failed"
    [ "$failed" -eq 0 ]
}

tests() {
    sc=${1:-build/sultanc}
    [ -f "$exe" ] || build "$sc"
    mkdir -p "$out"
    "$sc" --target x86_64-windows-msvc tests/runner/main.sn -o "$out/runner.exe"
    rm -rf build/tests-sultanc
    (
        export SULTANC_BIN='build\sultanc.exe' TARGET=x86_64-windows-msvc \
            SULTANC_TEST_SH='Z:\bin\sh' SULTANC_TEST_ENV='Z:\usr\bin\env' \
            SULTANC_TEST_TIMEOUT="${SULTANC_TEST_TIMEOUT:-300}"
        in_wine "$out/runner.exe"
    )
}

case "${1:-}" in
build) shift; build "$@" ;;
run) shift; in_wine "$exe" "$@" ;;
exe) shift; in_wine "$@" ;;
smoke) smoke ;;
tests) shift; tests "$@" ;;
*)
    sed -n '2,9p' "$0" >&2
    exit 2
    ;;
esac
