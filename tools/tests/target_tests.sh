#!/usr/bin/env bash
# Runs tests/{en,ar}/target on every target their `// أهداف:` header names,
# with the LLVM backend:
#   check   `sultanc check --target T` accepts the program;
#   build   hosted targets build an executable (the compiler links it: its own
#           ELF, Mach-O, COFF or wasm linker, or ld.lld; see
#           docs/en/reference/targets.md), other targets an object, or
#           assembly for the assembly-only ones (nvptx64, xcore);
#   run     where the host can run the executable: natively (x86_64 Linux and
#           Android), under qemu-user (other Linux and Android architectures),
#           under a WASI runner, under node (wasm32-browser, whose `_start`
#           returns main's value), under Wine (x86_64-windows-msvc), or under
#           tools/tests/freebsd_abi_emulator.c
#           (x86_64-freebsd, single-threaded programs only). Exit status and
#           `// خرج-قياسي:` are compared.
# A target the host cannot run is reported as built, not as run.
#
#   SC=./build/sultanc bash tools/tests/target_tests.sh [target...]
# With target arguments, only those targets run. WASI_RUNNER overrides the
# WASI command (default: wasmtime, else node examples/run-wasi.mjs).
# Run from the repository root on an x86_64 Linux host.
set -u
SC=${SC:-./build/sultanc}
OUT=${OUT:-build/target-tests}
rm -rf "$OUT"; mkdir -p "$OUT"
only=" $* "

wasi_runner() {
  if [ -n "${WASI_RUNNER:-}" ]; then echo "$WASI_RUNNER"; return; fi
  if command -v wasmtime > /dev/null; then echo "wasmtime run --dir=."; return; fi
  if command -v node > /dev/null; then echo "node --no-warnings examples/run-wasi.mjs"; return; fi
}

# A wasm32-browser module has no imports; its `_start` returns main's value.
cat > "$OUT/run-browser.js" << 'EOF_JS'
const fs = require("fs");
const compiled = new WebAssembly.Module(fs.readFileSync(process.argv[2]));
process.exitCode = Number(new WebAssembly.Instance(compiled, {}).exports._start());
EOF_JS

emulator=""
if command -v cc > /dev/null && cc -O1 -o "$OUT/freebsd-emulator" tools/tests/freebsd_abi_emulator.c 2> /dev/null; then
  emulator="$OUT/freebsd-emulator"
fi

# The command that runs an executable of the target on this host, or nothing.
runner_for() {
  local q=""
  case "$1" in
    x86_64-linux|x86_64-android) echo "env"; return ;;
    aarch64-linux-gnu|aarch64-android) q=qemu-aarch64 ;;
    i686-linux-gnu) q=qemu-i386 ;;
    armv7-linux-gnueabihf|armv7-android) q=qemu-arm ;;
    riscv64-linux-gnu) q=qemu-riscv64 ;;
    loongarch64-linux-gnu) q=qemu-loongarch64 ;;
    mips64el-linux-gnuabi64) q=qemu-mips64el ;;
    powerpc64le-linux-gnu) q=qemu-ppc64le ;;
    s390x-linux-gnu) q=qemu-s390x ;;
    sparcv9-linux-gnu) q=qemu-sparc64 ;;
    wasm32-wasi) wasi_runner; return ;;
    wasm32-browser) command -v node > /dev/null && echo "node $OUT/run-browser.js"; return ;;
    x86_64-freebsd) echo "$emulator"; return ;;
    x86_64-windows-msvc) command -v wine > /dev/null && echo "env WINEDEBUG=-all wine"; return ;;
  esac
  if [ -n "$q" ] && command -v "$q" > /dev/null; then echo "$q"; fi
}

# What the compiler produces for the target: exe, obj or asm.
output_for() {
  case "$1" in
    nvptx64-*|xcore-*) echo asm ;;
    ve-*) echo obj ;;
    *-linux*|*-android*|*-freebsd*|*-darwin*|*-windows*|wasm32-*) echo exe ;;
    *) echo obj ;;
  esac
}

header() { sed -n "s|^// $1: ||p" "$2" | head -1; }

pass=0; fail=0; ran=0; built=0
failures=()
for src in $(find tests/en/target tests/ar/target -name '*.sn' | sort); do
  targets=$(header 'أهداف' "$src" | tr ',' ' ')
  status=$(header 'خروج' "$src"); status=${status:-0}
  want_out=$(header 'خرج-قياسي' "$src"); check_out=0
  if grep -q '^// خرج-قياسي: ' "$src"; then check_out=1; fi
  [ "$want_out" = "<فارغ>" ] && want_out=""
  base=$(echo "$src" | sed 's|^tests/||; s|/|_|g; s|\.sn$||')
  for t in $targets; do
    case "$only" in "  ") ;; *" $t "*) ;; *) continue ;; esac
    name="$src [$t]"
    log="$OUT/$base.$t.log"
    if ! "$SC" check --target "$t" "$src" > "$log" 2>&1; then
      fail=$((fail + 1)); failures+=("$name: check"); echo "FAIL check $name"; continue
    fi
    kind=$(output_for "$t")
    out="$OUT/$base.$t.$kind"
    case "$kind" in
      exe) args=() ;;
      obj) args=(--emit=obj) ;;
      asm) args=(--emit=asm) ;;
    esac
    if ! "$SC" --backend=llvm --target="$t" "${args[@]}" "$src" -o "$out" >> "$log" 2>&1; then
      fail=$((fail + 1)); failures+=("$name: $kind"); echo "FAIL $kind $name"; continue
    fi
    run=""
    [ "$kind" = exe ] && run=$(runner_for "$t")
    # The FreeBSD translator has no thr_new: threaded programs are built only.
    if [ "$t" = x86_64-freebsd ] && grep -q '^import std.thread\|^استورد std.thread' "$src"; then run=""; fi
    if [ -z "$run" ]; then
      pass=$((pass + 1)); built=$((built + 1)); echo "ok   $kind $name"; continue
    fi
    got=$(timeout 120 $run "$out" < /dev/null 2>> "$log"); rc=$?
    if [ "$rc" != "$status" ]; then
      fail=$((fail + 1)); failures+=("$name: exit $rc, expected $status"); echo "FAIL run $name (exit $rc)"; continue
    fi
    if [ "$check_out" = 1 ] && [ "$got" != "$want_out" ]; then
      fail=$((fail + 1)); failures+=("$name: stdout"); echo "FAIL run $name (stdout: $got)"; continue
    fi
    pass=$((pass + 1)); ran=$((ran + 1)); echo "ok   run $name"
  done
done
echo "target tests: passed=$pass (run=$ran built-only=$built) failed=$fail"
for f in "${failures[@]+"${failures[@]}"}"; do echo "  $f"; done
[ "$fail" -eq 0 ]
