#!/bin/sh
# Optional convenience entry point. All build configuration and stage policy
# belongs to cmake/; this script only invokes CMake.
set -eu

source_directory=${0%/*}
if [ "$source_directory" = "$0" ]; then
    source_directory=.
fi
build_directory="$source_directory/build/bootstrap/cmake"

stage=stage2
case "${1:-}" in
    stage0|stage1|stage2|bootstrap|verify)
        stage=$1
        shift
        ;;
esac

target=
output=
while [ "$#" -gt 0 ]; do
    case "$1" in
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
                'usage: ./build.sh [stage0|stage1|stage2|bootstrap|verify] [--target=TRIPLE] [OUTPUT]' \
                'All stages are configured and built by CMake in cmake/.' \
                '  bootstrap  Stage0 + Stage1 only (the bootstrap compiler)' \
                '  verify     Stage2, then Stage3/Stage4 convergence for native and LLVM' \
                'Products stay in build/bootstrap/cmake/bin; the requested stage is also' \
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

cmake -S "$source_directory/cmake" -B "$build_directory" \
    "-DSULTANC_REQUESTED_STAGE:STRING=$stage" \
    "-DSULTANC_REQUESTED_TARGET:STRING=$target" \
    "-DSULTANC_REQUESTED_OUTPUT:STRING=$output"

cmake --build "$build_directory" --target "sultanc-$stage"
