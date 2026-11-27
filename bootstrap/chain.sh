#!/bin/sh
# The release check of the frozen C Stage0 (docs/en/internals/bootstrap.md,
# "The frozen C Stage0"). POSIX sh: Linux and macOS (bootstrap/chain.ps1 is
# the Windows one). The build driver runs the same steps (./build.sh chain,
# tools/build/chain.sn) on a host with a Native backend; this script is for
# any POSIX host, since before the chain a host without a Native backend has
# only the seed's Stage1, which cannot build the driver for it.
#
#   sh bootstrap/chain.sh                 the whole chain and the comparison
#   sh bootstrap/chain.sh --stage0-only   build the C Stage0 only (CodeQL)
#
# 1. The C Stage0 (bootstrap/src, C11, against an LLVM SDK) is built.
# 2. It compiles the compiler of the commit bootstrap/FROZEN pins (stage0),
#    which is linked with libLLVM: the first compiler of the chain.
# 3. Each `step` commit of bootstrap/FROZEN is built by the compiler before it.
# 4. The chain's last compiler builds the current sources (A, LLVM-enabled),
#    and A builds them again, Native-only, for the comparison target (D).
# 5. Stage1 from the committed seed builds the same compiler (W): D must equal
#    W byte for byte. Two independent roots, the C Stage0 and the WebAssembly
#    seed, give one compiler.
#
# The comparison target is the host when it has a Native backend
# (x86_64-linux, arm64-darwin), else x86_64-linux: the seed's Stage1 is
# Native-only and builds a compiler for a Native target on any host.
#
# Needs git (the pinned commits must be in the checkout's history), a C
# compiler and LLVM 19 to 23 (LLVM_CONFIG=/path/to/llvm-config, or the first
# llvm-config on PATH; SULTANC_LLVM_LIBRARY names the shared library).
# Products: build/chain/ (sultanc-a is the chain's LLVM-enabled compiler for
# this host).
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
# The chain compiles for real: no compiler of it reads or writes the
# compilation cache (compiler/driver/compile_cache.sn).
SULTANC_CACHE=off
export SULTANC_CACHE
out=build/chain
stage0_only=no
for argument in "$@"; do
    case "$argument" in
        --stage0-only) stage0_only=yes ;;
        -h|--help)
            sed -n '2,/^set -eu/p' "$0" | sed '$d' | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *) printf 'chain.sh: unexpected argument: %s\n' "$argument" >&2; exit 2 ;;
    esac
done

say() { printf 'chain.sh: %s\n' "$*"; }
fail() { printf 'chain.sh: error: %s\n' "$*" >&2; exit 1; }

host=
case "$(uname -s)/$(uname -m)" in
    Linux/x86_64|Linux/amd64) host=x86_64-linux ;;
    Linux/aarch64|Linux/arm64) host=aarch64-linux-gnu ;;
    Darwin/arm64|Darwin/aarch64) host=arm64-darwin ;;
    Darwin/x86_64) host=x86_64-darwin ;;
    *) fail "no SultanC host target for $(uname -s)/$(uname -m)" ;;
esac
case "$host" in
    x86_64-linux|arm64-darwin) backend=native compare_target=$host ;;
    *) backend=llvm compare_target=x86_64-linux ;;
esac
jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

# --- LLVM -----------------------------------------------------------------------

config=${LLVM_CONFIG:-}
if [ -z "$config" ]; then
    for c in llvm-config llvm-config-23 llvm-config-22 llvm-config-21 llvm-config-20 llvm-config-19 \
        /opt/homebrew/opt/llvm/bin/llvm-config /usr/local/opt/llvm/bin/llvm-config; do
        if command -v "$c" > /dev/null 2>&1; then config=$(command -v "$c"); break; fi
    done
fi
[ -n "$config" ] || fail "the C Stage0 needs LLVM 19 to 23: set LLVM_CONFIG=/path/to/llvm-config"
include=$("$config" --includedir)
libdir=$("$config" --libdir)
major=$("$config" --version | cut -d. -f1)
resolve() {
    p=$1
    while [ -L "$p" ]; do
        l=$(readlink "$p")
        case "$l" in /*) p=$l ;; *) p=$(dirname "$p")/$l ;; esac
    done
    printf '%s/%s\n' "$(cd "$(dirname "$p")" && pwd -P)" "$(basename "$p")"
}
library=${SULTANC_LLVM_LIBRARY:-}
if [ -z "$library" ]; then
    for name in "libLLVM-$major.so" "libLLVM.so.$major.1" "libLLVM.so.$major" "libLLVM.so" \
        "libLLVM-$major.dylib" "libLLVM.dylib"; do
        if [ -f "$libdir/$name" ]; then library=$libdir/$name; break; fi
    done
fi
if [ -z "$library" ] || [ ! -f "$library" ]; then fail "no shared libLLVM in $libdir (from $config)"; fi
library=$(resolve "$library")
say "LLVM $("$config" --version): $library"

# --- 1. the C Stage0 ------------------------------------------------------------

mkdir -p "$out/stage0-objects"
cc=${CC:-cc}
say "building the C Stage0 ($cc, C11, warnings are errors)"
find bootstrap/src -name '*.c' | LC_ALL=C sort > "$out/stage0-sources.list"
# One object per source, named by its path, compiled $jobs at a time.
# Warnings are errors, as CodeQL and every release check build it; a newer C
# compiler with a new warning can turn that off (SULTANC_STAGE0_WERROR=0).
werror=-Werror
[ "${SULTANC_STAGE0_WERROR:-1}" != 0 ] || werror=
export cc include out werror
# shellcheck disable=SC2016 # the child shell expands them
xargs -P "$jobs" -n 1 sh -c '
    object=$out/stage0-objects/$(printf "%s" "$1" | tr "/" "_").o
    $cc -std=c11 -O1 -Wall -Wextra $werror -Ibootstrap/include/private -Ibootstrap/include/public \
        "-I$include" -c "$1" -o "$object"' sh < "$out/stage0-sources.list"
# shellcheck disable=SC2046 # one word per object (the names have no spaces)
$cc -o "$out/sultanc-stage0" $(sed 's|/|_|g; s|^|'"$out"'/stage0-objects/|; s|$|.o|' "$out/stage0-sources.list") \
    "$library" "-Wl,-rpath,$(dirname "$library")"
say "Stage0: $out/sultanc-stage0"
[ "$stage0_only" = no ] || exit 0

# --- 2 and 3. the pinned commits -------------------------------------------------

command -v git > /dev/null 2>&1 || fail "the chain needs git: the pinned commits come from the history"
tree_of() { # commit -> a directory holding that commit's compiler/ and library/
    t=$out/tree-$(printf '%s' "$1" | cut -c1-12)
    if [ ! -f "$t/compiler/main.sn" ]; then
        git cat-file -e "$1^{commit}" 2> /dev/null ||
            fail "commit $1 is not in this checkout (git fetch origin $1, or a full clone)"
        rm -rf "$t"
        mkdir -p "$t"
        git archive "$1" compiler library | tar -x -C "$t"
    fi
    printf '%s\n' "$t"
}

previous=
index=0
while read -r kind commit rest; do
    case "$kind" in ''|'#'*) continue ;; esac
    [ -z "$rest" ] || fail "bootstrap/FROZEN: unexpected text after $commit"
    tree=$(tree_of "$commit")
    product=$(cd "$out" && pwd)/chain-$index
    case "$kind" in
        stage0)
            [ -z "$previous" ] || fail "bootstrap/FROZEN: stage0 must be the first line"
            say "Stage0 compiles the compiler of $commit"
            # The Stage1 the C route always built: the interpreter's host
            # threads, the copy of later-phase facts and the width-exact raw
            # memory access are their stubs (Stage0 has neither threads nor
            # closures, nor `استنسخ`, nor integer-pointer casts).
            (cd "$tree" && "$root/$out/sultanc-stage0" compiler/main.sn "--target=$host" \
                "--module-source=compiler.interpreter.machine.host_threads=compiler/interpreter/machine/host_threads_unavailable.sn" \
                "--module-source=compiler.frontend.later_phase_facts_copy=compiler/frontend/later_phase_facts_copy_unavailable.sn" \
                "--module-source=compiler.interpreter.machine.raw_memory_access=compiler/interpreter/machine/raw_memory_access_unavailable.sn" \
                -o "$product.o")
            $cc -o "$product" "$product.o" "$library" "-Wl,-rpath,$(dirname "$library")"
            ;;
        step)
            [ -n "$previous" ] || fail "bootstrap/FROZEN: a step needs the stage0 line before it"
            say "the compiler of the line before builds the compiler of $commit"
            (cd "$tree" && "$previous" compiler/main.sn -O2 "--backend=$backend" "--target=$host" \
                -o "$product" "$library")
            ;;
        *) fail "bootstrap/FROZEN: unknown line kind $kind (stage0 or step)" ;;
    esac
    chmod 0755 "$product"
    previous=$product
    index=$((index + 1))
done < bootstrap/FROZEN
[ -n "$previous" ] || fail "bootstrap/FROZEN names no stage0 commit"

# --- 4. the current sources -------------------------------------------------------

native_only=--module-source=compiler.backend.provider.llvm.llvm_codegen=compiler/backend/provider/llvm/unavailable.sn
say "the chain's last compiler builds the current sources (A, LLVM-enabled)"
"$previous" compiler/main.sn -O2 "--backend=$backend" "--target=$host" -o "$out/sultanc-a" "$library"
chmod 0755 "$out/sultanc-a"
say "A builds them again, Native-only, for $compare_target (D)"
"$out/sultanc-a" compiler/main.sn -O2 --backend=native "--target=$compare_target" "$native_only" -o "$out/sultanc-d"

# --- 5. the seed route -----------------------------------------------------------

say "Stage1 from the seed builds the same compiler (W)"
./build.sh bootstrap
stage1=build/bootstrap/seed/sultanc-stage1
"$stage1" compiler/main.sn -O2 --backend=native "--target=$compare_target" "$native_only" -o "$out/sultanc-w"
chmod 0755 "$out/sultanc-w" "$out/sultanc-d"
if cmp -s "$out/sultanc-d" "$out/sultanc-w"; then
    say "the C Stage0 chain and the seed give one compiler ($compare_target): $out/sultanc-d == $out/sultanc-w"
else
    fail "the C Stage0 chain and the seed give different compilers: $out/sultanc-d and $out/sultanc-w differ
  (with a current seed the seed route is the fixpoint: a difference is a compiler bug, or a
  step bootstrap/FROZEN lacks; see docs/en/internals/bootstrap.md)"
fi
say "the chain's LLVM-enabled compiler for $host: $out/sultanc-a"
