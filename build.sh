#!/bin/sh
# Optional convenience entry point. All build configuration and stage policy
# belongs to cmake/; this script only invokes CMake.
set -eu

source_directory=${0%/*}
if [ "$source_directory" = "$0" ]; then
    source_directory=.
fi

stage=stage2
case "${1:-}" in
    stage0|stage1|stage2|bootstrap|verify)
        stage=$1
        shift
        ;;
esac

target=
output=
# Compiler-build capability of the self-hosted compiler (SULTANC_COMPILER_LLVM):
# Native-only unless --enable-llvm is given.
compiler_llvm=OFF
# Stage1 bootstrap route (SULTANC_BOOTSTRAP): c = bootstrap/ (C Stage0),
# wasm = bootstrap-wasm/ (WebAssembly seed translated to C).
bootstrap_route=c
# --bootstrap=existing: the compiler that stands in for Stage1 (default build/sultanc).
existing_compiler=
# Producer backend (SULTANC_STAGE2_BACKEND): the backend Stage1 uses to
# generate Stage2 (on Windows, also the verify stages' backend). It selects the
# code generator of the compiler build only; it is not the backends the built
# compiler contains (that is --enable-llvm) nor the backend of programs the
# built compiler compiles (their own --backend). Empty keeps the build tree's
# cached value (native when the tree is new).
backend=
while [ "$#" -gt 0 ]; do
    case "$1" in
        --enable-llvm) compiler_llvm=ON ;;
        --bootstrap=*) bootstrap_route=${1#--bootstrap=} ;;
        --with=*) existing_compiler=${1#--with=} ;;
        --backend=*) backend=${1#--backend=} ;;
        --backend)
            if [ "$#" -lt 2 ]; then
                printf '%s\n' 'build.sh: --backend requires native or llvm' >&2
                exit 2
            fi
            backend=$2
            shift
            ;;
        --bootstrap)
            if [ "$#" -lt 2 ]; then
                printf '%s\n' 'build.sh: --bootstrap requires c or wasm' >&2
                exit 2
            fi
            bootstrap_route=$2
            shift
            ;;
        --target=*) target=${1#--target=} ;;
        --target)
            if [ "$#" -lt 2 ]; then
                printf '%s\n' 'build.sh: --target requires a value' >&2
                exit 2
            fi
            target=$2
            shift
            ;;
        -h|--help|help)
            printf '%s\n' \
                'usage: ./build.sh [stage0|stage1|stage2|bootstrap|verify] [--target=TRIPLE] [--enable-llvm]' \
                '                  [--backend=native|llvm] [--bootstrap=c|wasm|existing [--with=PATH]] [OUTPUT]' \
                'All stages are configured and built by CMake in cmake/.' \
                '  bootstrap  Stage0 + Stage1 only (the bootstrap compiler)' \
                '  verify     Stage2, then Stage3/Stage4 convergence (LLVM convergence needs --enable-llvm)' \
                '  --enable-llvm  include the LLVM backend in the built compiler (default: Native-only,' \
                '             no libLLVM dependency); with --bootstrap=c, Stage0/Stage1 always use LLVM' \
                '  --backend=native|llvm  backend Stage1 uses to generate Stage2' \
                '             (SULTANC_STAGE2_BACKEND; default native). Windows has no Native backend:' \
                '             use --bootstrap=c --enable-llvm --backend=llvm there' \
                '  --bootstrap=c     Stage1 from the C Stage0 in bootstrap/ (default; needs LLVM)' \
                '  --bootstrap=wasm  Stage1 from the WebAssembly seed in bootstrap-wasm/' \
                '             (translated to C; needs only a C compiler unless --enable-llvm)' \
                '  --bootstrap=existing  Stage1 is an already-built compiler (--with=PATH, default' \
                '             build/sultanc): one self-host step, the fast route for development' \
                '             (as Go uses GOROOT_BOOTSTRAP). Use c or wasm on a new system, when the' \
                '             existing compiler is too old for the sources, and to verify a release' \
                'Products stay in build/bootstrap/cmake/bin (cmake-wasm/bin for --bootstrap=wasm);' \
                'the requested stage is also' \
                'copied to build/sultanc (Stage2) or build/bootstrap/ (Stage0/Stage1).' \
                'LLVM selection: LLVM_CONFIG=/path/to/llvm-config, or -DLLVM_DIR via cmake.' \
                'For direct builds: cmake -S cmake -B build/bootstrap/cmake' \
                '                   cmake --build build/bootstrap/cmake --target sultanc-stage2'
            exit 0
            ;;
        -*)
            printf 'build.sh: unexpected option: %s\n' "$1" >&2
            exit 2
            ;;
        *)
            if [ -n "$output" ]; then
                printf 'build.sh: unexpected argument: %s\n' "$1" >&2
                exit 2
            fi
            output=$1
            ;;
    esac
    shift
done

case "$backend" in
    ''|native|llvm) ;;
    *)
        printf 'build.sh: --backend must be native or llvm, not %s\n' "$backend" >&2
        exit 2
        ;;
esac

case "$bootstrap_route" in
    c) build_directory="$source_directory/build/bootstrap/cmake" ;;
    wasm) build_directory="$source_directory/build/bootstrap/cmake-wasm" ;;
    existing) build_directory="$source_directory/build/bootstrap/cmake-existing" ;;
    *)
        printf 'build.sh: --bootstrap must be c, wasm or existing, not %s\n' "$bootstrap_route" >&2
        exit 2
        ;;
esac

cmake -S "$source_directory/cmake" -B "$build_directory" \
    "-DSULTANC_BOOTSTRAP:STRING=$bootstrap_route" \
    "-DSULTANC_REQUESTED_STAGE:STRING=$stage" \
    "-DSULTANC_REQUESTED_TARGET:STRING=$target" \
    "-DSULTANC_REQUESTED_OUTPUT:STRING=$output" \
    "-DSULTANC_COMPILER_LLVM:BOOL=$compiler_llvm" \
    ${backend:+"-DSULTANC_STAGE2_BACKEND:STRING=$backend"} \
    ${existing_compiler:+"-DSULTANC_EXISTING_COMPILER:FILEPATH=$existing_compiler"}

cmake --build "$build_directory" --target "sultanc-$stage"
