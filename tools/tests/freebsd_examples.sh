#!/usr/bin/env bash
# Builds every examples/en program for x86_64-freebsd and runs it on Linux
# x86-64 under tools/tests/freebsd_abi_emulator.c (FreeBSD system calls
# translated by ptrace), comparing its output with the native x86_64-linux
# build. Run from the repository root:
#   SC=./build/sultanc bash tools/tests/freebsd_examples.sh
# SC must have the LLVM backend; ld.lld must be on PATH. This exercises std's
# FreeBSD code paths; it does not replace a run on FreeBSD.
set -u
SC=${SC:-./build/sultanc}
OUT=${OUT:-build/freebsd-examples}
rm -rf "$OUT"; mkdir -p "$OUT"
cc -O1 -o "$OUT/emulator" tools/tests/freebsd_abi_emulator.c || exit 2
same=0; different=0; unbuilt=0
for src in $(find examples/en -name '*.sn' | sort); do
  n=$(echo "${src#examples/en/}" | sed 's/\.sn$//; s|/|_|g')
  if ! "$SC" --backend=llvm --target=x86_64-freebsd "$src" -o "$OUT/$n.freebsd" > "$OUT/$n.log" 2>&1; then
    unbuilt=$((unbuilt + 1)); echo "not built: $src"; continue
  fi
  "$SC" "$src" -o "$OUT/$n.native" >> "$OUT/$n.log" 2>&1 || { echo "no native build: $src"; continue; }
  mkdir -p "$OUT/run-native-$n" "$OUT/run-freebsd-$n"
  (cd "$OUT/run-native-$n" && printf 'hi\n' | timeout 30 "../$n.native" a1 b2 > "../$n.native.out" 2>&1; echo "rc=$?" >> "../$n.native.out")
  (cd "$OUT/run-freebsd-$n" && printf 'hi\n' | timeout 60 ../emulator "../$n.freebsd" a1 b2 > "../$n.freebsd.out" 2>&1; echo "rc=$?" >> "../$n.freebsd.out")
  if cmp -s "$OUT/$n.native.out" "$OUT/$n.freebsd.out"; then same=$((same + 1)); else different=$((different + 1)); echo "different: $src"; fi
done
echo "same=$same different=$different not-built=$unbuilt"
[ "$different" -eq 0 ]
