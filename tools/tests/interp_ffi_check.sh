#!/bin/sh
# Runs the programs that call C functions they declare themselves
# (docs/en/internals/interpreter.md, "Dynamic C functions") natively and interpreted, and
# prints one row per program:
#   program  native-exit  interpreted-exit  same|different
# then whether each Arabic/English pair printed the same output. The
# programs are examples/{en,ar}/ffi/extern_call.sn and
# tests/{en,ar}/runtime/ffi/libm_libc_calls.sn.
#
# Run from the repository root:
#   sh tools/tests/interp_ffi_check.sh
#       Linux: SC builds each program (with -l c -l m) and ISC interprets it.
#       ISC must be a compiler the system's dynamic loader started (an
#       LLVM-enabled build): a static compiler has no loader and reports
#       SULTANC-INTERPRETER-UNSUPPORTED-EXTERNAL-CALL.
#   MODE=wine WSC=path/to/sultanc.exe sh tools/tests/interp_ffi_check.sh
#       Windows under wine: SC (LLVM-enabled) builds each program for
#       x86_64-windows-msvc, wine runs it, and wine runs WSC (a compiler built
#       for Windows) to interpret it. WINEPREFIX is used as set. A program
#       whose extern names no library cannot be linked for Windows (the COFF
#       import needs the DLL); its reference is the host's native run.
#   MODE=macos SDK=path/to/MacOSX.sdk sh tools/tests/interp_ffi_check.sh
#       macOS without a Mac: SC builds each program for arm64-darwin and
#       x86_64-darwin and tools/audit/macos_imports_audit.py checks every
#       import against the SDK (run the default mode on a Mac to execute).
# SC defaults to ./build/sultanc and ISC to SC. Everything the script writes
# goes under build/tmp/interp_ffi_check/. Exit status 0 when every row
# matches. Plain POSIX sh.

SC=${SC:-./build/sultanc}
ISC=${ISC:-$SC}
MODE=${MODE:-host}
OUT=build/tmp/interp_ffi_check
PROGRAMS="examples/en/ffi/extern_call.sn examples/ar/ffi/extern_call.sn tests/en/runtime/ffi/libm_libc_calls.sn tests/ar/runtime/ffi/libm_libc_calls.sn"

if [ ! -x "$SC" ]; then
    echo "interp_ffi_check.sh: missing compiler $SC (build it, or set SC)" >&2
    exit 2
fi
if [ ! -d examples/en/ffi ]; then
    echo "interp_ffi_check.sh: run from the repository root" >&2
    exit 2
fi
rm -rf "$OUT"
mkdir -p "$OUT"
failures=0

name_of() { echo "$1" | sed 's#/#_#g; s#\.sn$##'; }

if [ "$MODE" = macos ]; then
    if [ -z "${SDK:-}" ]; then echo "interp_ffi_check.sh: MODE=macos needs SDK" >&2; exit 2; fi
    for program in $PROGRAMS; do
        for arch in arm64 x86_64; do
            n=$(name_of "$program")-$arch
            backend=native
            [ "$arch" = x86_64 ] && backend=llvm
            if "$SC" --backend=$backend --target=$arch-darwin "$program" -o "$OUT/$n" > "$OUT/$n.build" 2>&1 \
                && python3 tools/audit/macos_imports_audit.py --sdk "$SDK" --arch $arch "$OUT/$n" > "$OUT/$n.audit" 2>&1; then
                printf '%-46s %-7s %s\n' "$program" "$arch" "imports ok"
            else
                printf '%-46s %-7s %s\n' "$program" "$arch" "FAILED (see $OUT/$n.build, $OUT/$n.audit)"
                failures=$((failures + 1))
            fi
        done
    done
    exit $([ $failures -eq 0 ] && echo 0 || echo 1)
fi

printf '%-46s %7s %7s  %s\n' "program" "native" "interp" "result"
printf '%-46s %7s %7s  %s\n' "-------" "------" "------" "------"
for program in $PROGRAMS; do
    n=$(name_of "$program")
    if [ "$MODE" = wine ]; then
        if [ -z "${WSC:-}" ]; then echo "interp_ffi_check.sh: MODE=wine needs WSC" >&2; exit 2; fi
        # A declaration without a library (extern_call.sn) cannot be linked
        # for Windows; its reference run is then the host's native one.
        if "$SC" --backend=llvm --target=x86_64-windows-msvc "$program" -o "$OUT/$n.exe" > "$OUT/$n.build" 2>&1; then
            WINEDEBUG=-all wine "$OUT/$n.exe" < /dev/null > "$OUT/$n.native" 2> "$OUT/$n.native.err"
            native=$?
        else
            "$SC" "$program" -l c -l m -o "$OUT/$n" >> "$OUT/$n.build" 2>&1
            "$OUT/$n" < /dev/null > "$OUT/$n.native" 2> "$OUT/$n.native.err"
            native=$?
        fi
        WINEDEBUG=-all wine "$WSC" interpret "$program" < /dev/null > "$OUT/$n.interp" 2> "$OUT/$n.interp.err"
        interp=$?
    else
        "$SC" "$program" -l c -l m -o "$OUT/$n" > "$OUT/$n.build" 2>&1
        "$OUT/$n" < /dev/null > "$OUT/$n.native" 2> "$OUT/$n.native.err"
        native=$?
        "$ISC" interpret "$program" < /dev/null > "$OUT/$n.interp" 2> "$OUT/$n.interp.err"
        interp=$?
    fi
    tr -d '\r' < "$OUT/$n.native" > "$OUT/$n.native.lf"
    tr -d '\r' < "$OUT/$n.interp" > "$OUT/$n.interp.lf"
    if [ $native -eq 0 ] && [ $native -eq $interp ] && cmp -s "$OUT/$n.native.lf" "$OUT/$n.interp.lf"; then
        result=same
    else
        result=different
        failures=$((failures + 1))
    fi
    printf '%-46s %7s %7s  %s\n' "$program" "$native" "$interp" "$result"
done
for pair in examples/@/ffi/extern_call tests/@/runtime/ffi/libm_libc_calls; do
    en=$(name_of "$(echo $pair | sed 's/@/en/').sn")
    ar=$(name_of "$(echo $pair | sed 's/@/ar/').sn")
    if cmp -s "$OUT/$en.interp.lf" "$OUT/$ar.interp.lf"; then
        echo "pair $(echo $pair | sed 's/@/{en,ar}/'): same output"
    else
        echo "pair $(echo $pair | sed 's/@/{en,ar}/'): different output"
        failures=$((failures + 1))
    fi
done
for f in "$OUT"/*.interp.err; do
    if [ -s "$f" ]; then echo "== $f"; tail -n 10 "$f"; fi
done
exit $([ $failures -eq 0 ] && echo 0 || echo 1)
