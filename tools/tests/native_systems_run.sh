#!/usr/bin/env bash
# Runs the tests whose `// أهداف:` header names TARGET with the Native backend
# for that target, under an emulator or compatibility layer, and checks the
# `// خروج:` status. The SultanC runner (tests/runner) knows only the host
# targets; this covers the further Native systems:
#   tools/tests/native_systems_run.sh aarch64-linux-gnu qemu-aarch64
#   tools/tests/native_systems_run.sh x86_64-windows-msvc wine
#   tools/tests/native_systems_run.sh x86_64-darwin none   (build only)
#   tools/tests/native_systems_run.sh x86_64-android env   (runs on the host)
#   tools/tests/native_systems_run.sh x86_64-freebsd none  (build only)
# SC overrides the compiler (default ./build/sultanc). Each test is also
# compiled to a relocatable object (-c). Exit status 0 only when all pass.
set -u -o pipefail
TARGET=${1:?target}
RUNNER=${2:-}
SC=${SC:-./build/sultanc}
OUT=build/native-systems/$TARGET
mkdir -p "$OUT"
case "$TARGET" in
  *windows*) exe_suffix=.exe ;;
  *) exe_suffix= ;;
esac
pass=0; fail=0; built=0
while IFS= read -r test; do
  name=$(echo "$test" | tr '/' '_' | sed 's/\.sn$//')
  expected=$(sed -n 's|^// خروج: *\([0-9][0-9]*\).*|\1|p' "$test" | head -1)
  expected=${expected:-0}
  exe="$OUT/$name$exe_suffix"
  if ! "$SC" "$test" --backend native --target "$TARGET" -o "$exe" > "$OUT/$name.build.log" 2>&1; then
    echo "FAIL (build) $test"; fail=$((fail+1)); continue
  fi
  if ! "$SC" -c "$test" --backend native --target "$TARGET" -o "$OUT/$name.o" > "$OUT/$name.object.log" 2>&1; then
    echo "FAIL (object) $test"; fail=$((fail+1)); continue
  fi
  if [ "$RUNNER" = none ]; then
    built=$((built+1)); continue
  fi
  $RUNNER "$exe" > "$OUT/$name.out" 2>&1 < /dev/null
  status=$?
  if [ "$status" -eq "$expected" ]; then
    pass=$((pass+1))
  else
    echo "FAIL (exit $status, expected $expected) $test"; fail=$((fail+1))
  fi
done < <(grep -rl --include='*.sn' -e "^// أهداف:.*$TARGET" tests/en tests/ar | sort)
echo "native-systems $TARGET: passed=$pass failed=$fail built-only=$built"
[ "$fail" -eq 0 ]
