#!/bin/sh
# Builds the SultanC compiler (docs/en/internals/bootstrap.md, "build.sh").
#
# This script only gets a first compiler from the committed seed when there is
# none (bootstrap-wasm/bootstrap.c: the seed translated to C and compiled with
# $CC, else cc), builds the build driver with it (tools/build/, written in
# SultanC) and runs the driver with the same arguments:
#
#   ./build.sh [build|verify|bootstrap|seed-check|seed-update|chain] [options]
#
# ./build.sh with no command builds the compiler: build/sultanc, the last line
# printed (`build: compiler: build/sultanc`). ./build.sh --help lists the
# commands and options; --verbose shows each command as it runs. A host
# without a Native backend (Linux arm64, macOS x86_64) builds the driver only
# with a compiler that has LLVM (build/sultanc, or --with=PATH); with the seed
# compiler (Stage1) alone this script still gives `build` (build/sultanc is
# then Stage1), `bootstrap` and `seed-check` itself.
set -eu

root=$(cd "$(dirname "$0")" && pwd)
cd "$root"

say() { printf 'build: %s\n' "$*"; }
fail() { printf 'build: error: %s\n' "$*" >&2; exit 1; }

seed_dir=build/bootstrap/seed
stage1=$seed_dir/sultanc-stage1
jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
verbose=
for argument in "$@"; do
    case "$argument" in --verbose) verbose=--verbose ;; esac
done

sha256() { # file... -> one line per file: hash  path
    if command -v sha256sum > /dev/null 2>&1; then sha256sum "$@"
    elif command -v shasum > /dev/null 2>&1; then shasum -a 256 "$@"
    else cksum "$@"
    fi
}
digest() { sha256 - | cut -d' ' -f1; } # standard input -> one hash

# Stage1 from the committed seed, translated again only when the seed or
# bootstrap.c changed. The stamp is the driver's (tools/build/steps.sn,
# بصمة_ملفات): the hash of the lines "<hash>  <path>\n". bootstrap.c writes
# the same stamp, so a Stage1 ./build-bootstrap left here is reused.
seed_stage1() {
    mkdir -p "$seed_dir"
    stamp=$(sha256 bootstrap-wasm/seed/sultanc.wasm.zst bootstrap-wasm/bootstrap.c | digest)
    if [ -x "$stage1" ] && [ -f "$seed_dir/seed.stamp" ] && [ "$(cat "$seed_dir/seed.stamp")" = "$stamp" ]; then
        return
    fi
    # shellcheck disable=SC2086 # CC may hold a command and its flags
    ${CC:-cc} -O2 -o "$seed_dir/build-bootstrap" bootstrap-wasm/bootstrap.c
    "$seed_dir/build-bootstrap" "--out=$seed_dir" "--jobs=$jobs" --stage1-only $verbose
    printf '%s' "$stamp" > "$seed_dir/seed.stamp"
}

command=${1:-build}
# The host's target: a compiler that is itself a translated WASI module (the
# seed's Stage1) would otherwise build for wasm32-wasi.
case "$(uname -s)/$(uname -m)" in
    Linux/x86_64|Linux/amd64) native_host=yes; host_target=x86_64-linux ;;
    Darwin/arm64|Darwin/aarch64) native_host=yes; host_target=arm64-darwin ;;
    *) native_host=no; host_target= ;;
esac

# The commands, without building anything first (the driver's --help has the
# options in full).
case "$command" in
    -h|--help|help)
        if [ ! -x build/bootstrap/sultanc-build ]; then
            sed -n '/^\/\/ The SultanC build driver/,/^\/\/ Run from/p' tools/build/main.sn | sed 's|^// \{0,1\}||'
            exit 0
        fi ;;
esac

# `bootstrap` is the first compiler itself.
if [ "$command" = bootstrap ] || [ "$command" = stage1 ]; then
    seed_stage1
    cp "$stage1" build/bootstrap/sultanc-stage1
    if [ "$#" -ge 2 ]; then
        case "$2" in -*) ;; *) mkdir -p "$(dirname "$2")"; cp "$stage1" "$2" ;; esac
    fi
    say "seed compiler (Stage1): build/bootstrap/sultanc-stage1"
    exit 0
fi

# The compiler that builds the driver: build/sultanc, --with, or Stage1.
with=
for argument in "$@"; do
    case "$argument" in --with=*) with=${argument#--with=} ;; esac
done
driver=build/bootstrap/sultanc-build
driver_compiler=
for candidate in build/sultanc "$with"; do
    if [ -n "$candidate" ] && [ -x "$candidate" ]; then driver_compiler=$candidate; break; fi
done
if [ -z "$driver_compiler" ]; then
    seed_stage1
    driver_compiler=$stage1
fi

# Builds the driver with $1; the stamp is the hash of its sources and of $1.
build_driver() {
    mkdir -p build/bootstrap
    rm -f "$driver" "$driver.new"
    "$1" -O1 ${host_target:+--target="$host_target"} tools/build/main.sn -o "$driver.new" > build/bootstrap/sultanc-build.log 2>&1 || return 1
    chmod 0755 "$driver.new"
    mv -f "$driver.new" "$driver"
    { sha256 tools/build/*.sn; sha256 "$1" | cut -d' ' -f1; } | digest > "$driver.stamp"
}
driver_stamp=$( { sha256 tools/build/*.sn; sha256 "$driver_compiler" | cut -d' ' -f1; } | digest)
if [ ! -x "$driver" ] || [ ! -f "$driver.stamp" ] || [ "$(cat "$driver.stamp")" != "$driver_stamp" ]; then
    say "building the build driver (tools/build)"
    if build_driver "$driver_compiler"; then
        :
    elif [ "$native_host" = yes ] && [ "$driver_compiler" != "$stage1" ]; then
        # The compiler there may be too old or broken for the driver: Stage1.
        seed_stage1
        build_driver "$stage1" ||
            { cat build/bootstrap/sultanc-build.log >&2; fail "the build driver (tools/build) does not build"; }
    elif [ "$native_host" = yes ]; then
        cat build/bootstrap/sultanc-build.log >&2
        fail "the build driver (tools/build) does not build"
    else
        driver=
    fi
fi

if [ -n "$driver" ]; then
    exec "$driver" "$@"
fi

# No driver: a host without a Native backend and no compiler that has LLVM.
# The compiler here is Stage1, the seed compiler (as bootstrap.c leaves it),
# and Stage1 still checks the seed (it rebuilds the seed module itself).
# build/sultanc.stamp "seed compiler" marks a build/sultanc that is Stage1
# (bootstrap.c writes it too); the driver's stamps never read so.
plain=yes
for argument in "$@"; do
    case "$argument" in build|--verbose) ;; *) plain=no ;; esac
done
if [ "$plain" = yes ] && { [ ! -e build/sultanc ] || [ "$(cat build/sultanc.stamp 2>/dev/null)" = "seed compiler" ]; }; then
    seed_stage1
    if ! cmp -s "$stage1" build/sultanc; then
        rm -f build/sultanc.new
        cp "$stage1" build/sultanc.new
        chmod 0755 build/sultanc.new
        mv -f build/sultanc.new build/sultanc
    fi
    printf '%s\n' "seed compiler" > build/sultanc.stamp
    say "no native backend for this host; build/sultanc is the seed compiler (targets x86_64-linux, arm64-darwin, wasm32-wasi)"
    say "compiler: build/sultanc"
    exit 0
fi
if [ "$command" = seed-check ]; then
    seed_stage1
    "$stage1" compiler/main.sn -O2 --backend=native --target=wasm32-wasi \
        --module-source=compiler.backend.provider.llvm.llvm_codegen=compiler/backend/provider/llvm/unavailable.sn \
        -o "$seed_dir/sultanc.wasm"
    "$seed_dir/sultanc-wasm2c" --decompress bootstrap-wasm/seed/sultanc.wasm.zst "$seed_dir/sultanc.committed.wasm"
    if cmp -s "$seed_dir/sultanc.wasm" "$seed_dir/sultanc.committed.wasm"; then
        say "seed check: the committed seed is reproduced exactly (by Stage1)"
        exit 0
    fi
    printf '%s\n' "build: seed check: the current sources give a different seed (a seed bump: ./build.sh seed-update)" >&2
    exit 3
fi
cat build/bootstrap/sultanc-build.log >&2 2> /dev/null || true
fail "this host has no Native backend, and $driver_compiler cannot build the build driver for it:
  ./build.sh, ./build.sh bootstrap and ./build.sh seed-check work here with Stage1; everything else needs a compiler
  that has LLVM (--with=PATH, or build/sultanc: a cross-built or bootstrap/chain.sh compiler)"
