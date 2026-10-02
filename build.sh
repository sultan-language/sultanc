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
  ./build.sh stage0 [--target=<llvm-target>] [output]
  ./build.sh stage1 [--target=<llvm-target>] [output]
  ./build.sh bootstrap [--target=<llvm-target>] [output-directory]

Production targets:
    arm64-darwin
    x86_64-linux

Normal source build:
  ./build.sh
  ./build.sh --target=x86_64-linux build/sultanc-linux

For a normal source build, --target selects the production native target of the
final self-hosted compiler. Stage0 itself remains a host-native C executable.

For stage0 mode, --target sets the Stage0 compiler's default LLVM object target.
Direct Stage0 invocations may also pass --target=<alias-or-LLVM-triple>.

For stage1/bootstrap mode, --target selects the LLVM target used for the Stage1
object and its custom executable finalizer. Targets without an implemented
finalizer fail explicitly instead of falling back to the host.
USAGE
}

llvm_flags() {
    LLVM_CFLAGS=
    LLVM_LIBS=
    LLVM_DISCOVERY=
    LLVM_TOOL=

    if [ -n "${LLVM_CONFIG:-}" ]; then
        if [ ! -x "$LLVM_CONFIG" ]; then
            echo "sultanc-bootstrap: LLVM_CONFIG is not executable: $LLVM_CONFIG" >&2
            exit 1
        fi
        LLVM_TOOL=$LLVM_CONFIG
    elif command -v llvm-config >/dev/null 2>&1; then
        LLVM_TOOL=$(command -v llvm-config)
    fi

    if [ -n "$LLVM_TOOL" ]; then
        LLVM_CFLAGS=$($LLVM_TOOL --cflags)
        # Stage0 initializes every target configured into this LLVM installation.
        # Ask llvm-config for the installation's complete supported link set rather
        # than deriving target libraries from the host or a target-name list.
        LLVM_LIBS=$($LLVM_TOOL --ldflags --libs all --system-libs)
        LLVM_DISCOVERY="llvm-config=$LLVM_TOOL"
    else
        LLVM_LIBS=${SULTANC_LLVM_LIBRARY:--lLLVM}
        if [ -n "${SULTANC_LLVM_LIBRARY:-}" ]; then
            LLVM_DISCOVERY="compiler/system headers; SULTANC_LLVM_LIBRARY=$SULTANC_LLVM_LIBRARY"
        else
            LLVM_DISCOVERY="compiler/system headers and libraries (-lLLVM)"
        fi
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

build_stage0() {
    S0_OUT=$1
    S0_DEFAULT_TARGET=${2:-}
    S0_TMP=${TMPDIR:-/tmp}/sultanc-stage0-build.$$
    trap 'rm -rf "$S0_TMP"' EXIT HUP INT TERM
    mkdir -p "$S0_TMP" "$(dirname -- "$S0_OUT")"

    if [ -n "$S0_DEFAULT_TARGET" ]; then
        case "$S0_DEFAULT_TARGET" in
            *[!A-Za-z0-9_.+-]*)
                echo "sultanc-bootstrap: invalid Stage0 default target spelling: $S0_DEFAULT_TARGET" >&2
                exit 2
                ;;
        esac
    fi

    find "$BOOTSTRAP/src" -name '*.c' -type f | LC_ALL=C sort > "$S0_TMP/sources"

    llvm_flags
    # Stage0 is the only temporary host bridge. LLVM owns target-specific object
    # lowering; an optional build target only changes Stage0's default object target.
    # shellcheck disable=SC2086
    if [ -n "$S0_DEFAULT_TARGET" ]; then
        "$CC_BIN" -std=c11 -O1 -Wall -Wextra -Werror \
            -I"$BOOTSTRAP/include/private" -I"$BOOTSTRAP/include/public" \
            "-DSULTANC_BOOTSTRAP_DEFAULT_TARGET=\"$S0_DEFAULT_TARGET\"" \
            $LLVM_CFLAGS $(cat "$S0_TMP/sources") $LLVM_LIBS -o "$S0_OUT"
    else
        "$CC_BIN" -std=c11 -O1 -Wall -Wextra -Werror \
            -I"$BOOTSTRAP/include/private" -I"$BOOTSTRAP/include/public" \
            $LLVM_CFLAGS $(cat "$S0_TMP/sources") $LLVM_LIBS -o "$S0_OUT"
    fi
    printf '%s\n' "$S0_OUT"
    printf 'LLVM discovery: %s\n' "$LLVM_DISCOVERY" >&2
    rm -rf "$S0_TMP"
    trap - EXIT HUP INT TERM
}
build_stage1_with_stage0() {
    S1_OUT=$1
    S0_BIN=$2
    S1_TARGET=$3

    mkdir -p "$(dirname -- "$S1_OUT")"
    S1_DIR=$(CDPATH= cd -- "$(dirname -- "$S1_OUT")" && pwd)
    S1_ABS=$S1_DIR/$(basename -- "$S1_OUT")
    S0_DIR=$(CDPATH= cd -- "$(dirname -- "$S0_BIN")" && pwd)
    S0_ABS=$S0_DIR/$(basename -- "$S0_BIN")

    S1_TMP=${TMPDIR:-/tmp}/sultanc-stage1-object.$$
    trap 'rm -rf "$S1_TMP"' EXIT HUP INT TERM
    mkdir -p "$S1_TMP"

    (
        cd "$PROJECT"
        "$S0_ABS" compiler/main.sn --target="$S1_TARGET" -o "$S1_TMP/compiler.o"
        "$S0_ABS" --finalize-stage1 "$S1_TMP/compiler.o" "$S1_ABS" "$S1_TARGET"
    )

    chmod +x "$S1_ABS"
    rm -rf "$S1_TMP"
    trap - EXIT HUP INT TERM
    printf '%s\n' "$S1_ABS"
}

build_stage1() {
    S1_OUT=$1
    S1_TARGET=$2
    S1_TMP_STAGE0=${TMPDIR:-/tmp}/sultanc-stage1-stage0.$$
    trap 'rm -f "$S1_TMP_STAGE0"' EXIT HUP INT TERM
    build_stage0 "$S1_TMP_STAGE0" >/dev/null
    build_stage1_with_stage0 "$S1_OUT" "$S1_TMP_STAGE0" "$S1_TARGET"
    rm -f "$S1_TMP_STAGE0"
    trap - EXIT HUP INT TERM
}

build_bootstrap() {
    OUT_DIR=$1
    S1_TARGET=$2
    mkdir -p "$OUT_DIR"
    build_stage0 "$OUT_DIR/sultanc-stage0"
    build_stage1_with_stage0 "$OUT_DIR/sultanc-stage1" "$OUT_DIR/sultanc-stage0" "$S1_TARGET"
}

build_current_compiler() {
    OUTPUT=$1
    OUTPUT_TARGET=$2
    BOOTSTRAP_OUT="$BUILD_DIR/bootstrap"
    HOST_BOOTSTRAP_TARGET=$(host_target)
    mkdir -p "$BUILD_DIR" "$BOOTSTRAP_OUT" "$(dirname -- "$OUTPUT")"

    # The Stage1 compiler used by this source build must remain host-native so it can execute here.
    build_bootstrap "$BOOTSTRAP_OUT" "$HOST_BOOTSTRAP_TARGET"

    # The requested production target applies here: host-native Stage1 cross-compiles
    # the final self-hosted compiler using SultanC's registered native Target layer.
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
TARGET_SET=0
OUTPUT=
while [ "$#" -gt 0 ]; do
    case "$1" in
        --target=*)
            TARGET=${1#--target=}
            TARGET_SET=1
            if [ -z "$TARGET" ]; then
                echo "sultanc-build: --target requires a value" >&2
                exit 2
            fi
            ;;
        --target)
            if [ "$#" -lt 2 ]; then
                echo "sultanc-build: --target requires a value" >&2
                exit 2
            fi
            TARGET=$2
            TARGET_SET=1
            if [ -z "$TARGET" ]; then
                echo "sultanc-build: --target requires a value" >&2
                exit 2
            fi
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
        if [ "$TARGET_SET" -eq 0 ]; then
            TARGET=$(host_target)
        fi
        validate_target "$TARGET"
        build_current_compiler "${OUTPUT:-$BUILD_DIR/sultanc}" "$TARGET"
        ;;
    stage0)
        build_stage0 "${OUTPUT:-$BUILD_DIR/bootstrap/sultanc-stage0}" "$TARGET"
        ;;
    stage1)
        if [ "$TARGET_SET" -eq 0 ]; then
            TARGET=$(host_target)
        fi
        build_stage1 "${OUTPUT:-$BUILD_DIR/bootstrap/sultanc-stage1}" "$TARGET"
        ;;
    bootstrap)
        if [ "$TARGET_SET" -eq 0 ]; then
            TARGET=$(host_target)
        fi
        build_bootstrap "${OUTPUT:-$BUILD_DIR/bootstrap}" "$TARGET"
        ;;
    *)
        usage
        exit 2
        ;;
esac
