#!/usr/bin/env bash
# Runs the debugger sessions of tests/{en,ar}/debugger under real debuggers.
# Each program NAME.sn has a session NAME.dbg (format: tests/en/debugger/values.dbg)
# that sets breakpoints by source line (a `// BP:marker` comment in the
# program or a module it imports) and by function name, runs, steps, prints
# values and backtraces, and states regular expressions each output must match.
#
# For every program, backend and optimization level it builds the program and
# runs the session:
#   x86_64-linux   under gdb and under lldb;
#   aarch64-linux  under lldb attached to qemu-aarch64's gdbstub (gdb on this
#                  host cannot debug AArch64), with AARCH64=1.
# Backends: native, and llvm when the compiler includes LLVM (BACKENDS=...).
# Optimization levels: -O0 and the default (OPTS="-O0 default").
#
#   SC=./build/sultanc bash tools/tests/debugger_tests.sh [program-name...]
#
# Each check prints PASS, FAIL or BLOCKED (a debugger, qemu or the backend is
# missing). VERBOSE=1 prints every debugger transcript. The exit status is 1
# when any check failed.
set -u
SC=${SC:-./build/sultanc}
OUT=${OUT:-build/debugger-tests}
BACKENDS=${BACKENDS:-native llvm}
OPTS=${OPTS:-"-O0 default"}
AARCH64=${AARCH64:-0}
mkdir -p "$OUT"
HERE=$(cd "$(dirname "$0")" && pwd)
only=" $* "

pass=0; fail=0; blocked=0
failures=()
ok() { pass=$((pass + 1)); echo "PASS    $*"; }
bad() { fail=$((fail + 1)); failures+=("$*"); echo "FAIL    $*"; }
blk() { blocked=$((blocked + 1)); echo "BLOCKED $*"; }

GDB=$(command -v gdb || true)
LLDB=$(command -v lldb || true)
QEMU=$(command -v qemu-aarch64 || true)
export LC_ALL=C.UTF-8

# run_session SPEC PROGRAM_DIR BINARY DEBUGGER(gdb|lldb) MODE(local|qemu) LABEL
run_session() {
  local spec=$1 dir=$2 bin=$3 dbg=$4 mode=$5 label=$6
  local base="$OUT/$(basename "$bin").$dbg"
  python3 -I "$HERE/debugger_session.py" translate "$spec" "$dir" "$dbg" "$mode" > "$base.cmd" || {
    bad "$label: cannot translate $spec"; return; }
  if [ "$dbg" = gdb ]; then
    timeout 120 "$GDB" -nx -batch -x "$base.cmd" "$bin" > "$base.out" 2>&1
  elif [ "$mode" = qemu ]; then
    local port=$((20000 + RANDOM % 20000))
    "$QEMU" -g "$port" "$bin" > /dev/null 2>&1 &
    local qpid=$!
    sed -i "s/@PORT@/$port/" "$base.cmd"
    timeout 120 "$LLDB" -b -s "$base.cmd" "$bin" > "$base.out" 2>&1
    kill "$qpid" 2>/dev/null; wait "$qpid" 2>/dev/null
  else
    timeout 120 "$LLDB" -b -s "$base.cmd" "$bin" > "$base.out" 2>&1
  fi
  [ "${VERBOSE:-0}" = 1 ] && cat "$base.out"
  python3 -I "$HERE/debugger_session.py" check "$spec" "$dbg" "$base.out" "$label" > "$base.res"
  while IFS= read -r line; do
    case "$line" in
      PASS*) ok "${line#PASS }" ;;
      FAIL*) bad "${line#FAIL }" ;;
    esac
  done < "$base.res"
}

for spec in tests/en/debugger/*.dbg tests/ar/debugger/*.dbg; do
  [ -f "$spec" ] || continue
  prog="${spec%.dbg}.sn"
  name=$(basename "${spec%.dbg}")
  lang=$(basename "$(dirname "$(dirname "$spec")")")
  if [ "$only" != "  " ] && [[ "$only" != *" $name "* ]]; then continue; fi
  dir=$(dirname "$spec")
  for backend in $BACKENDS; do
    for opt in $OPTS; do
      optflag=""; [ "$opt" = default ] || optflag="$opt"
      tag="$lang/$name $backend $opt"
      bin="$OUT/$lang-$name-$backend-${opt#-}"
      if ! "$SC" "$prog" -b "$backend" $optflag -o "$bin" > "$bin.build" 2>&1; then
        if grep -q 'SULTANC-BACKEND-UNAVAILABLE' "$bin.build"; then blk "$tag: backend not in this compiler"; continue; fi
        bad "$tag: build failed ($bin.build)"; continue
      fi
      if [ -n "$GDB" ]; then run_session "$spec" "$dir" "$bin" gdb local "$tag gdb"; else blk "$tag gdb: no gdb"; fi
      if [ -n "$LLDB" ]; then run_session "$spec" "$dir" "$bin" lldb local "$tag lldb"; else blk "$tag lldb: no lldb"; fi
      if [ "$AARCH64" = 1 ]; then
        abin="$bin-aarch64"
        if ! "$SC" "$prog" -b "$backend" $optflag -t aarch64-linux-gnu -o "$abin" > "$abin.build" 2>&1; then
          bad "$tag aarch64: build failed ($abin.build)"
        elif [ -z "$QEMU" ] || [ -z "$LLDB" ]; then
          blk "$tag aarch64: needs qemu-aarch64 and lldb"
        else
          run_session "$spec" "$dir" "$abin" lldb qemu "$tag aarch64 lldb"
        fi
      fi
    done
  done
done

echo "debugger tests: $pass passed, $fail failed, $blocked blocked"
for f in "${failures[@]+"${failures[@]}"}"; do echo "  FAILED: $f"; done
[ "$fail" -eq 0 ]
