#!/usr/bin/env bash
# Builds, links and runs tests/{en,ar}/target on the Cortex-M targets their
# `// أهداف:` header names (thumbv6m-none-eabi, thumbv7m-none-eabi,
# thumbv7em-none-eabi, thumbv7em-none-eabihf), with the LLVM backend:
#   check   `sultanc check --target T` accepts the program;
#   obj     the compiler emits a relocatable ELF object that references no
#           run-time routine: its only undefined symbols are the section
#           bounds the linker script defines;
#   link    the object, which carries the vector table and reset handler the
#           compiler emits for a Cortex-M program, is linked by an explicit
#           `ld.lld -T` with the board's linker script and semihosting.ld
#           (library/std/os/platform/none/arm/cortex_m). SultanC's own ELF
#           linker does not link these targets yet, so this step is visible
#           and external;
#   run     under qemu-system-arm with semihosting, on every board of the
#           target: the exit status is main's result, compared with
#           `// خروج:`.
# A division by zero must end the run with status 132 (the HardFault path of
# the start-up). It also checks that float operations the target's FPU cannot do (every
# one on the eabi targets, f64 on eabihf) are rejected with
# SULTANC-TARGET-FEATURE-UNSUPPORTED, at the source statement and with the
# source function's name, instead of calling soft-float routines.
#
#   SC=./build/sultanc bash tools/tests/cortex_m_tests.sh [target...]
# Needs a compiler built with LLVM, ld.lld and qemu-system-arm; a missing tool
# makes its steps BLOCKED. No assembler or C toolchain is used.
set -u
SC=${SC:-./build/sultanc}
OUT=${OUT:-build/cortex-m-tests}
SUPPORT=library/std/os/platform/none/arm/cortex_m
rm -rf "$OUT"; mkdir -p "$OUT"
only=" $* "

find_lld() {
  if [ -n "${SULTANC_LINKER:-}" ]; then echo "$SULTANC_LINKER"; return; fi
  if [ -n "${LLVM_CONFIG:-}" ] && [ -x "$("$LLVM_CONFIG" --bindir)/ld.lld" ]; then
    echo "$("$LLVM_CONFIG" --bindir)/ld.lld"; return
  fi
  command -v ld.lld
}
LLD=$(find_lld)
QEMU=$(command -v qemu-system-arm)

# QEMU boards per target, with the linker script of their memory map.
boards_for() {
  case "$1" in
    thumbv6m-none-eabi) echo "microbit:nrf51" ;;
    thumbv7m-none-eabi) echo "lm3s6965evb:lm3s6965 mps2-an385:mps2" ;;
    thumbv7em-none-eabi|thumbv7em-none-eabihf) echo "mps2-an386:mps2 mps2-an500:mps2" ;;
  esac
}

header() { sed -n "s|^// $1: ||p" "$2" | head -1; }

pass=0; fail=0; blocked=0
failures=()
ok() { pass=$((pass + 1)); echo "PASS    $*"; }
bad() { fail=$((fail + 1)); failures+=("$*"); echo "FAIL    $*"; }
blk() { blocked=$((blocked + 1)); echo "BLOCKED $*"; }

for src in $(find tests/en/target tests/ar/target -name '*.sn' | sort); do
  status=$(header 'خروج' "$src"); status=${status:-0}
  base=$(echo "$src" | sed 's|^tests/||; s|/|_|g; s|\.sn$||')
  for t in $(header 'أهداف' "$src" | tr ',' ' '); do
    case "$t" in thumbv*) ;; *) continue ;; esac
    case "$only" in "  ") ;; *" $t "*) ;; *) continue ;; esac
    name="$src [$t]"
    log="$OUT/$base.$t.log"
    if ! "$SC" check --target "$t" "$src" > "$log" 2>&1; then bad "check $name"; continue; fi
    obj="$OUT/$base.$t.o"
    if ! "$SC" --backend=llvm --target="$t" --emit=obj "$src" -o "$obj" >> "$log" 2>&1; then
      bad "obj $name"; continue
    fi
    # Only the section bounds cortex_m.ld defines may be undefined.
    undefined=$(llvm-nm -u "$obj" 2> /dev/null | awk '{print $2}' \
      | grep -vxE '__(stack_top|data_start|data_end|data_load|bss_start|bss_end|init_array_start|init_array_end)' \
      | tr '\n' ' ')
    if [ -n "$undefined" ]; then bad "obj $name: undefined $undefined"; continue; fi
    ok "obj $name"
    if [ -z "$LLD" ]; then blk "link $name (no ld.lld)"; continue; fi
    for b in $(boards_for "$t"); do
      machine=${b%%:*}; script=${b#*:}
      elf="$OUT/$base.$t.$machine.elf"
      link=("$LLD" -T "$SUPPORT/board_$script.ld" -L "$SUPPORT" --gc-sections
        "$obj" "$SUPPORT/semihosting.ld" -o "$elf")
      echo "  ${link[*]}" >> "$log"
      if ! "${link[@]}" >> "$log" 2>&1; then bad "link $name ($machine)"; continue; fi
      ok "link $name ($machine, external ld.lld)"
      if [ -z "$QEMU" ]; then blk "run $name ($machine: no qemu-system-arm)"; continue; fi
      timeout 120 "$QEMU" -M "$machine" -nographic -monitor none -serial none \
        -semihosting-config enable=on,target=native -kernel "$elf" < /dev/null >> "$log" 2>&1
      rc=$?
      if [ "$rc" != "$status" ]; then bad "run $name ($machine): exit $rc, expected $status"; continue; fi
      ok "run $name ($machine)"
    done
  done
done

# A trap (here a division by zero: UDF, escalated to a HardFault) ends the
# run with status 132 through the semihosting HardFault handler the compiler
# emits with the start-up.
cat > "$OUT/trap.sn" << 'EOF_SN'
def divide(a: u32, b: u32): u32 { return a / b; }
def main(): int { return divide(84, 0) as int; }
EOF_SN
for t in thumbv6m-none-eabi thumbv7m-none-eabi thumbv7em-none-eabi thumbv7em-none-eabihf; do
  case "$only" in "  ") ;; *" $t "*) ;; *) continue ;; esac
  log="$OUT/trap.$t.log"
  if ! "$SC" --backend=llvm --target="$t" --emit=obj "$OUT/trap.sn" -o "$OUT/trap.$t.o" > "$log" 2>&1; then
    bad "obj trap [$t]"; continue
  fi
  if [ -z "$LLD" ] || [ -z "$QEMU" ]; then blk "run trap [$t] (no ld.lld or qemu-system-arm)"; continue; fi
  set -- $(boards_for "$t"); b=$1; machine=${b%%:*}; script=${b#*:}
  if ! "$LLD" -T "$SUPPORT/board_$script.ld" -L "$SUPPORT" --gc-sections "$OUT/trap.$t.o" \
      "$SUPPORT/semihosting.ld" -o "$OUT/trap.$t.elf" >> "$log" 2>&1; then
    bad "link trap [$t]"; continue
  fi
  timeout 120 "$QEMU" -M "$machine" -nographic -monitor none -serial none \
    -semihosting-config enable=on,target=native -kernel "$OUT/trap.$t.elf" < /dev/null >> "$log" 2>&1
  rc=$?
  if [ "$rc" != 132 ]; then bad "run trap [$t] ($machine): exit $rc, expected 132"; else ok "run trap [$t] ($machine)"; fi
done

# Float operations without hardware support are rejected, never left as
# calls to soft-float routines.
cat > "$OUT/soft_float.sn" << 'EOF_SN'
def scale(a: f32, b: f32): f32 { return a * b; }
def main(): int { return scale(2.0, 3.0) as int; }
EOF_SN
cat > "$OUT/double_float.sn" << 'EOF_SN'
def scale(a: f64, b: f64): f64 { return a * b; }
def main(): int { return scale(2.0, 3.0) as int; }
EOF_SN
for c in thumbv6m-none-eabi:soft_float thumbv7m-none-eabi:soft_float thumbv7em-none-eabi:soft_float \
    thumbv7em-none-eabihf:double_float; do
  t=${c%%:*}; p=${c#*:}
  case "$only" in "  ") ;; *" $t "*) ;; *) continue ;; esac
  log="$OUT/$p.$t.log"
  if "$SC" --lang=en --backend=llvm --target="$t" --emit=obj "$OUT/$p.sn" -o "$OUT/$p.$t.o" > "$log" 2>&1; then
    bad "reject $p [$t]: compiled"
  elif ! grep -q 'SULTANC-TARGET-FEATURE-UNSUPPORTED' "$log"; then
    bad "reject $p [$t]: other error"
  # At the user's statement in scale, never at an internal symbol.
  elif ! grep -q "$p.sn:1:34" "$log" || ! grep -q '`scale`' "$log" \
      || grep -q 'sultanc_fn_' "$log"; then
    bad "reject $p [$t]: not reported at the source of scale"
  else
    ok "reject $p [$t]"
  fi
done

echo "cortex-m tests: passed=$pass failed=$fail blocked=$blocked"
for f in "${failures[@]+"${failures[@]}"}"; do echo "  $f"; done
[ "$fail" -eq 0 ]
