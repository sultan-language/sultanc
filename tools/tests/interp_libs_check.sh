#!/bin/sh
# Checks the libraries `sultanc run` loads (-l, -L; docs/en/internals/interpreter.md,
# "Dynamic C functions"): it builds two small shared libraries with the host C
# compiler, then runs tools/tests/interp_libs/libraries.{en,ar}.sn natively and
# interpreted with the libraries in both orders, and expects the same exit
# status both ways (42 with -l interp_first first, 43 with it second: the first
# library that defines a symbol provides it, and a library finds the symbols of
# one loaded before it). It also runs sqlite.{en,ar}.sn with -l sqlite3 when the
# host has libsqlite3, and checks that a library whose dependency is missing is
# reported as SULTANC-RUN-LIBRARY-LOAD-FAILED before the program starts.
#
# Run from the repository root (Linux or macOS):
#   sh tools/tests/interp_libs_check.sh
# SC (default ./build/sultanc) builds the executables; ISC (default SC) runs
# the programs. On Linux ISC must be a compiler the system's dynamic loader
# started (an LLVM-enabled build): a static compiler has no loader, and `run`
# then builds and runs an executable instead, so the interpreted column checks
# nothing. CC (default cc) builds the libraries. Everything goes under
# build/tmp/interp_libs_check/. Exit status 0 when every row matches.

SC=${SC:-./build/sultanc}
ISC=${ISC:-$SC}
CC=${CC:-cc}
DIR=tools/tests/interp_libs
OUT=build/tmp/interp_libs_check

if [ ! -x "$SC" ]; then
    echo "interp_libs_check.sh: missing compiler $SC (build it, or set SC)" >&2
    exit 2
fi
if [ ! -d "$DIR" ]; then
    echo "interp_libs_check.sh: run from the repository root" >&2
    exit 2
fi
rm -rf "$OUT"
mkdir -p "$OUT/lib"
failures=0

case $(uname -s) in
    Darwin) ext=dylib; shared="-dynamiclib -undefined dynamic_lookup" ;;
    *) ext=so; shared="-shared -fPIC" ;;
esac
LIB=$(cd "$OUT/lib" && pwd)
$CC $shared "$DIR/first.c" -o "$LIB/libinterp_first.$ext" || exit 2
$CC $shared "$DIR/second.c" -o "$LIB/libinterp_second.$ext" || exit 2
$CC $shared "$DIR/gone.c" -o "$LIB/libinterp_gone.$ext" || exit 2
$CC $shared "$DIR/missing_dependency.c" -L"$LIB" -linterp_gone -o "$LIB/libinterp_broken.$ext" || exit 2
rm -f "$LIB/libinterp_gone.$ext"

# One row: $1 program, $2 expected status, the rest the -l options.
row() {
    program=$1
    expected=$2
    shift 2
    n=$(basename "$program" .sn)-$(echo "$*" | tr -c 'a-z0-9\n' '_')
    if "$SC" "$program" -L "$LIB" "$@" -o "$OUT/$n" > "$OUT/$n.build" 2>&1; then
        LD_LIBRARY_PATH="$LIB" DYLD_LIBRARY_PATH="$LIB" "$OUT/$n" < /dev/null > "$OUT/$n.native" 2>&1
        native=$?
    else
        native=build-failed
    fi
    "$ISC" run -L "$LIB" "$@" "$program" < /dev/null > "$OUT/$n.interp" 2>&1
    interp=$?
    if [ "$native" = "$expected" ] && [ "$interp" = "$expected" ]; then result=same; else result=different; failures=$((failures + 1)); fi
    printf '%-24s %-36s %6s %6s %6s  %s\n' "$(basename "$program")" "$*" "$expected" "$native" "$interp" "$result"
}

printf '%-24s %-36s %6s %6s %6s  %s\n' program libraries expect native interp result
for lang in en ar; do
    row "$DIR/libraries.$lang.sn" 42 -l interp_first -l interp_second
    row "$DIR/libraries.$lang.sn" 43 -l interp_second -l interp_first
done
if [ -e /usr/lib/x86_64-linux-gnu/libsqlite3.so ] || [ -e /usr/lib/aarch64-linux-gnu/libsqlite3.so ] \
    || [ -e /usr/lib/libsqlite3.dylib ] || [ "$(uname -s)" = Darwin ]; then
    for lang in en ar; do row "$DIR/sqlite.$lang.sn" 42 -l sqlite3; done
else
    echo "sqlite3: no shared libsqlite3 on this host; skipped"
fi

# The load failure: reported, the program never runs.
"$ISC" run -L "$LIB" -l interp_broken "$DIR/broken.en.sn" < /dev/null > "$OUT/broken.interp" 2>&1
status=$?
if [ $status -ne 0 ] && grep -q 'SULTANC-RUN-LIBRARY-LOAD-FAILED' "$OUT/broken.interp"; then
    echo "load failure: reported (exit $status)"
else
    echo "load failure: NOT reported (exit $status, see $OUT/broken.interp)"
    failures=$((failures + 1))
fi
exit $([ $failures -eq 0 ] && echo 0 || echo 1)
