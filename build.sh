#!/bin/sh
set -eu

PROJECT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BOOTSTRAP="$PROJECT/bootstrap"
BUILD_DIR="$PROJECT/build"
CC_BIN=${CC:-cc}

usage() {
    cat >&2 <<'USAGE'
usage:
  ./build.sh
  ./build.sh [--target=<target>] [output]
  ./build.sh stage0 [output]
  ./build.sh stage1 [--target=<host-target>] [output]
  ./build.sh bootstrap [--target=<host-target>] [output-directory]

Targets:
    arm64-darwin
    x86_64-linux

Normal source build:
  ./build.sh
  ./build.sh --target=x86_64-linux build/sultanc-linux

The bootstrap seed and Stage1 are always built for the current host so Stage1
can run locally. For a normal source build, --target selects the architecture
of the final self-hosted SultanC compiler produced by Stage1.

For stage1/bootstrap mode, an explicit --target is only a host assertion and
must match the detected host target.
USAGE
}

llvm_flags() {
    LLVM_CFLAGS=
    LLVM_LIBS=
    LLVM_DISCOVERY=

    if [ -n "${LLVM_CONFIG:-}" ] && [ -x "$LLVM_CONFIG" ]; then
        LLVM_CFLAGS=$($LLVM_CONFIG --cflags)
        LLVM_LIBS=$($LLVM_CONFIG --ldflags --libs core native target executionengine mcjit --system-libs)
        LLVM_DISCOVERY="LLVM_CONFIG=$LLVM_CONFIG"
    elif command -v llvm-config >/dev/null 2>&1; then
        LLVM_TOOL=$(command -v llvm-config)
        LLVM_CFLAGS=$($LLVM_TOOL --cflags)
        LLVM_LIBS=$($LLVM_TOOL --ldflags --libs core native target executionengine mcjit --system-libs)
        LLVM_DISCOVERY="llvm-config=$LLVM_TOOL"
    elif command -v pkg-config >/dev/null 2>&1; then
        for package in llvm LLVM llvm-20 llvm-19 llvm-18 llvm-17; do
            if pkg-config --exists "$package" 2>/dev/null; then
                LLVM_CFLAGS=$(pkg-config --cflags "$package")
                LLVM_LIBS=$(pkg-config --libs "$package")
                LLVM_DISCOVERY="pkg-config=$package"
                break
            fi
        done
    fi

    if [ -z "$LLVM_LIBS" ] && [ -n "${SULTANC_LLVM_LIBRARY:-}" ]; then
        LLVM_LIBS=$SULTANC_LLVM_LIBRARY
        LLVM_DISCOVERY="SULTANC_LLVM_LIBRARY=$SULTANC_LLVM_LIBRARY"
    fi

    if [ -z "$LLVM_LIBS" ] && command -v ldconfig >/dev/null 2>&1; then
        LLVM_LIBRARY=$(ldconfig -p 2>/dev/null | awk '/libLLVM\.so/{print $NF; exit}')
        if [ -n "$LLVM_LIBRARY" ]; then
            LLVM_LIBS=$LLVM_LIBRARY
            LLVM_DISCOVERY="ldconfig=$LLVM_LIBRARY"
        fi
    fi

    if [ -z "$LLVM_LIBS" ]; then
        echo "sultanc-bootstrap: unable to discover libLLVM; set LLVM_CONFIG or install LLVM development metadata" >&2
        exit 1
    fi
}

validate_target() {
    case "$1" in
        arm64-darwin|x86_64-linux)
            ;;
        *)
            echo "sultanc-build: unsupported target: $1" >&2
            exit 2
            ;;
    esac
}

host_target() {
    HOST_OS=$(uname -s)
    HOST_ARCH=$(uname -m)
    case "$HOST_OS:$HOST_ARCH" in
        Darwin:arm64|Darwin:aarch64)
            printf '%s\n' arm64-darwin
            ;;
        Linux:x86_64|Linux:amd64)
            printf '%s\n' x86_64-linux
            ;;
        *)
            echo "sultanc-bootstrap: unsupported host target: $HOST_OS/$HOST_ARCH" >&2
            return 1
            ;;
    esac
}

require_host_target() {
    REQUESTED_HOST_TARGET=$1
    DETECTED_HOST_TARGET=$(host_target)
    if [ "$REQUESTED_HOST_TARGET" != "$DETECTED_HOST_TARGET" ]; then
        echo "sultanc-bootstrap: Stage1 must be host-native and runnable" >&2
        echo "sultanc-bootstrap: requested $REQUESTED_HOST_TARGET, host is $DETECTED_HOST_TARGET" >&2
        exit 2
    fi
}

build_stage0() {
    S0_OUT=$1
    S0_TMP=${TMPDIR:-/tmp}/sultanc-stage0-build.$$
    trap 'rm -rf "$S0_TMP"' EXIT HUP INT TERM
    mkdir -p "$S0_TMP" "$(dirname -- "$S0_OUT")"

    find "$BOOTSTRAP/src" -name '*.c' -type f | LC_ALL=C sort > "$S0_TMP/sources"

    llvm_flags
    # Stage0 is the only temporary host bridge. Compiler output remains native
    # object code produced through the Bootstrap Direct-LLVM path.
    # shellcheck disable=SC2086
    "$CC_BIN" -std=c11 -O1 -Wall -Wextra -Werror \
        -I"$BOOTSTRAP/include/private" -I"$BOOTSTRAP/include/public" \
        $LLVM_CFLAGS $(cat "$S0_TMP/sources") $LLVM_LIBS -o "$S0_OUT"
    printf '%s\n' "$S0_OUT"
    printf 'LLVM discovery: %s\n' "$LLVM_DISCOVERY" >&2
    rm -rf "$S0_TMP"
    trap - EXIT HUP INT TERM
}

build_stage1_with_stage0() {
    S1_OUT=$1
    S0_BIN=$2
    S1_TARGET=$(host_target)

    mkdir -p "$(dirname -- "$S1_OUT")"
    S1_DIR=$(CDPATH= cd -- "$(dirname -- "$S1_OUT")" && pwd)
    S1_ABS=$S1_DIR/$(basename -- "$S1_OUT")
    S0_DIR=$(CDPATH= cd -- "$(dirname -- "$S0_BIN")" && pwd)
    S0_ABS=$S0_DIR/$(basename -- "$S0_BIN")

    S1_TMP=${TMPDIR:-/tmp}/sultanc-stage1-native.$$
    trap 'rm -rf "$S1_TMP"' EXIT HUP INT TERM
    mkdir -p "$S1_TMP"

    case "$S1_TARGET" in
        arm64-darwin)
            (
                cd "$PROJECT"
                "$S0_ABS" compiler/main.sn -o "$S1_TMP/compiler.o"
                "$S0_ABS" bootstrap/finalize_stage1.sn --run \
                    "$S1_TMP/compiler.o" "$S1_ABS" "$S1_TARGET"
            )
            ;;
        x86_64-linux)
            (
                cd "$PROJECT"
                "$S0_ABS" compiler/main.sn -o "$S1_TMP/compiler.o"
                # Stage0 is the only bootstrap host bridge. This links the
                # host-native Stage0 object so Stage1 can run on this host.
                "$CC_BIN" "$S1_TMP/compiler.o" -o "$S1_ABS"
            )
            ;;
        *)
            echo "sultanc-bootstrap: unsupported Stage1 host target: $S1_TARGET" >&2
            return 1
            ;;
    esac

    chmod +x "$S1_ABS"
    rm -rf "$S1_TMP"
    trap - EXIT HUP INT TERM
    printf '%s\n' "$S1_ABS"
}

build_stage1() {
    S1_OUT=$1
    S1_TMP_STAGE0=${TMPDIR:-/tmp}/sultanc-stage1-stage0.$$
    trap 'rm -f "$S1_TMP_STAGE0"' EXIT HUP INT TERM
    build_stage0 "$S1_TMP_STAGE0" >/dev/null
    build_stage1_with_stage0 "$S1_OUT" "$S1_TMP_STAGE0"
    rm -f "$S1_TMP_STAGE0"
    trap - EXIT HUP INT TERM
}

build_bootstrap() {
    OUT_DIR=$1
    mkdir -p "$OUT_DIR"
    build_stage0 "$OUT_DIR/sultanc-stage0"
    build_stage1_with_stage0 "$OUT_DIR/sultanc-stage1" "$OUT_DIR/sultanc-stage0"
}

build_current_compiler() {
    OUTPUT=$1
    OUTPUT_TARGET=$2
    BOOTSTRAP_OUT="$BUILD_DIR/bootstrap"
    mkdir -p "$BUILD_DIR" "$BOOTSTRAP_OUT" "$(dirname -- "$OUTPUT")"

    # Bootstrap executables must remain host-native so they can execute here.
    build_bootstrap "$BOOTSTRAP_OUT"

    # The requested target applies here: host-native Stage1 cross-compiles the
    # final self-hosted compiler using SultanC's registered Target layer.
    (
        cd "$PROJECT"
        "$BOOTSTRAP_OUT/sultanc-stage1" \
            --target="$OUTPUT_TARGET" \
            -o "$OUTPUT" \
            compiler/main.sn
    )

    if [ ! -f "$OUTPUT" ]; then
        echo "sultanc-build: compiler output was not produced: $OUTPUT" >&2
        exit 1
    fi
    if [ ! -x "$OUTPUT" ]; then
        echo "sultanc-build: compiler output is not executable: $OUTPUT" >&2
        exit 1
    fi
    printf '%s\n' "$OUTPUT"
}

MODE=${1:-build}
if [ "$MODE" = "stage0" ] || [ "$MODE" = "stage1" ] || [ "$MODE" = "bootstrap" ]; then
    shift
else
    MODE=build
fi

TARGET=
OUTPUT=
while [ "$#" -gt 0 ]; do
    case "$1" in
        --target=*)
            TARGET=${1#--target=}
            ;;
        --target)
            if [ "$#" -lt 2 ]; then
                echo "sultanc-build: --target requires a value" >&2
                exit 2
            fi
            TARGET=$2
            shift
            ;;
        build)
            echo "sultanc-build: omit 'build'; use ./build.sh [--target=<target>] [output]" >&2
            exit 2
            ;;
        -h|--help|help)
            usage
            exit 0
            ;;
        *)
            if [ -n "$OUTPUT" ]; then
                echo "sultanc-build: unexpected argument: $1" >&2
                exit 2
            fi
            OUTPUT=$1
            ;;
    esac
    shift
done

case "$MODE" in
    build)
        if [ -z "$TARGET" ]; then
            TARGET=$(host_target)
        fi
        validate_target "$TARGET"
        build_current_compiler "${OUTPUT:-$BUILD_DIR/sultanc}" "$TARGET"
        ;;
    stage0)
        if [ -n "$TARGET" ]; then
            echo "sultanc-bootstrap: stage0 is always host-native; --target is not valid for stage0" >&2
            exit 2
        fi
        build_stage0 "${OUTPUT:-$BUILD_DIR/bootstrap/sultanc-stage0}"
        ;;
    stage1)
        if [ -n "$TARGET" ]; then
            validate_target "$TARGET"
            require_host_target "$TARGET"
        fi
        build_stage1 "${OUTPUT:-$BUILD_DIR/bootstrap/sultanc-stage1}"
        ;;
    bootstrap)
        if [ -n "$TARGET" ]; then
            validate_target "$TARGET"
            require_host_target "$TARGET"
        fi
        build_bootstrap "${OUTPUT:-$BUILD_DIR/bootstrap}"
        ;;
    *)
        usage
        exit 2
        ;;
esac
