#!/bin/sh
# Linux CI (x86_64 and arm64). POSIX sh. One subcommand per workflow step,
# so every step has its own timeout and its own line in the job summary:
#
#   build         pull-request tier: one build of the compiler with the
#                 previous compiler: $SULTANC_PREVIOUS (the cached compiler of
#                 the base commit) when it exists, else Stage1 from the seed
#                   x86_64 (Native backend): -> build/sultanc
#                   arm64  (no Native backend): Stage1 from the seed only
#   seed-build    main and nightly: from the seed (only a C compiler, no LLVM)
#                   x86_64: seed -> Stage1 -> S2 -> S3, S2 == S3 (./build.sh verify)
#                   arm64:  seed -> Stage1
#   seed-check    the committed seed equals what the sources rebuild
#   seed-update   (x86_64) bootstrap.c --update-seed, then a clean bootstrap
#                 from that seed: the next seed bump works
#   llvm-sdk      the pinned LLVM SDK (llvm.lock) in $LLVM_SDK, unless cached
#   llvm-build    (x86_64) one build of an LLVM-enabled compiler from the
#                 previous compiler (pull requests that change the LLVM provider)
#   llvm-verify   (x86_64, nightly) from the seed: LLVM-enabled S2 == S3, the
#                 LLVM fixpoint and cross-backend equality (./build.sh verify --enable-llvm)
#   cross         (x86_64, nightly) the LLVM-enabled compiler builds Native-only
#                 compilers for the hosts without a Native backend and for
#                 wasm32-wasi (build/cross/<target>/)
#   cross-run     (arm64, nightly) the cross-built compiler of this host runs:
#                 version, check and run of a program under the interpreter
#   chain         (release check) bootstrap/chain.sh: the frozen C Stage0 ->
#                 the pinned commit's compiler -> the current compiler, equal
#                 to the seed route's; on a host without a Native backend its
#                 LLVM-enabled compiler becomes build/sultanc
#   release       (release.yml) the compiler of a release, reporting
#                 $SULTANC_RELEASE_VERSION (./build.sh --release-version):
#                   Native host: from the seed, S2 == S3 (as seed-build)
#                   no Native backend: the chain's LLVM-enabled compiler
#                   (chain, run before) builds it once more with LLVM
#   test          the test suite (tools/tests/ci_run.sh) on build/sultanc,
#                 reported to the job summary and build/tests-junit.xml
#   seed | llvm   every step of that route in order (for local runs)
#
# LLVM: the upstream release archive, pinned by version and file name. It
# ships only component archives, so the monolithic libLLVM the build links is
# linked here from them. The workflow caches the result in $LLVM_SDK, keyed
# on llvm.lock alone.
set -eu

job=${1:?usage: linux.sh build|seed-build|seed-check|seed-update|llvm-sdk|llvm-build|llvm-verify|cross|cross-run|chain|release|test|seed|llvm}
# shellcheck source=.github/ci/common.sh
. "$(dirname "$0")/common.sh"
cd "$ci_root"
ci_load_llvm_lock
version=$LLVM_VERSION
sdk=${LLVM_SDK:-$HOME/llvm-sdk-$version}
# A build of the compiler that runs longer than this many seconds is
# reported as hung (it takes minutes; 6-hour job timeouts hid hangs before).
SULTANC_COMPILE_TIMEOUT=${SULTANC_CI_COMPILE_TIMEOUT:-5400}
export SULTANC_COMPILE_TIMEOUT

case "$(uname -m)" in
x86_64)
    native=yes
    asset=LLVM-$version-Linux-X64
    ;;
aarch64 | arm64)
    native=no
    asset=LLVM-$version-Linux-ARM64
    ;;
*)
    echo "linux.sh: unsupported machine $(uname -m)" >&2
    exit 2
    ;;
esac

llvm_sdk() {
    if [ -f "$sdk/lib/libLLVM.so.22.1" ] && [ -x "$sdk/bin/llvm-config" ]; then
        echo "LLVM SDK restored from cache: $sdk"
        return
    fi
    work=${RUNNER_TEMP:-/tmp}/llvm-work
    rm -rf "$work" "$sdk"
    mkdir -p "$work"
    curl -fL --retry 3 -o "$work/$asset.tar.xz" \
        "https://github.com/llvm/llvm-project/releases/download/llvmorg-$version/$asset.tar.xz"
    # Only what the build uses: headers, component archives, llvm-config,
    # clang and lld (the full archive does not fit a runner's disk).
    tar -xf "$work/$asset.tar.xz" -C "$work" --wildcards \
        "$asset/include/*" "$asset/lib/libLLVM*.a" "$asset/lib/libPolly*.a" \
        "$asset/bin/llvm-config" "$asset/bin/llvm-nm" "$asset/bin/llvm-readobj" \
        "$asset/bin/clang" "$asset/bin/clang++" "$asset/bin/clang-22" \
        "$asset/bin/lld" "$asset/bin/ld.lld"
    rm -f "$work/$asset.tar.xz"
    root=$work/$asset
    # Monolithic shared libLLVM from the component archives, linked with the
    # archive's own clang and lld. LineEditor (libedit; LLVM's interactive
    # tools only) stays out, as on macOS.
    archives=$("$root/bin/llvm-config" --link-static --libfiles all | tr ' ' '\n' |
        grep -v 'libLLVMLineEditor\.a$' | tr '\n' ' ')
    # shellcheck disable=SC2086 # one word per archive
    "$root/bin/clang++" -shared -fuse-ld=lld -Wl,-soname,libLLVM.so.22.1 \
        -o "$root/lib/libLLVM.so.22.1" \
        -Wl,--whole-archive $archives -Wl,--no-whole-archive \
        -lrt -ldl -lm -lpthread -l:libz.so.1 -l:libzstd.so.1 -l:libxml2.so.2
    mkdir -p "$sdk/bin" "$sdk/lib"
    cp "$root/bin/llvm-config" "$root/bin/llvm-nm" "$root/bin/llvm-readobj" "$sdk/bin/"
    cp -R "$root/include" "$sdk/include"
    cp "$root/lib/libLLVM.so.22.1" "$sdk/lib/"
    ln -sf libLLVM.so.22.1 "$sdk/lib/libLLVM-22.so"
    ln -sf libLLVM.so.22.1 "$sdk/lib/libLLVM.so"
    rm -rf "$work"
    echo "LLVM SDK built: $sdk"
}

llvm_env() {
    [ -x "$sdk/bin/llvm-config" ] || {
        echo "linux.sh: no LLVM SDK in $sdk (run: linux.sh llvm-sdk)" >&2
        exit 2
    }
    LLVM_CONFIG="$sdk/bin/llvm-config"
    LD_LIBRARY_PATH="$sdk/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export LLVM_CONFIG LD_LIBRARY_PATH
    echo "LLVM $("$LLVM_CONFIG" --version) from $sdk"
}

# The previous compiler of a pull request: the cached compiler of its base
# commit, when the cache has it (an N-1 compiler); else the seed.
build_compiler() {
    if [ "$native" = no ]; then
        ./build.sh bootstrap
        build/bootstrap/sultanc-stage1 version
        return
    fi
    if [ -n "${SULTANC_PREVIOUS:-}" ] && [ -x "$SULTANC_PREVIOUS" ]; then
        echo "previous compiler: $SULTANC_PREVIOUS (the base commit's)"
        ./build.sh "--with=$SULTANC_PREVIOUS"
    else
        echo "previous compiler: Stage1 from the seed"
        ./build.sh --bootstrap=wasm
    fi
    build/sultanc version
}

seed_build() {
    if [ "$native" = yes ]; then
        ./build.sh verify
    else
        ./build.sh bootstrap
        build/bootstrap/sultanc-stage1 version
    fi
}

seed_check() {
    ./build.sh seed-check
}

seed_update() {
    out=build/ci-seed-update
    rm -rf "$out" "$out-check"
    mkdir -p "$out"
    cc -O2 -o "$out/build-bootstrap" bootstrap-wasm/bootstrap.c
    "$out/build-bootstrap" --out="$out" --output="$out/sultanc" --update-seed
    "$out/build-bootstrap" --out="$out-check" --output="$out-check/sultanc" --verify --check-seed
}

llvm_build() {
    llvm_env
    if [ -n "${SULTANC_PREVIOUS:-}" ] && [ -x "$SULTANC_PREVIOUS" ]; then
        ./build.sh --enable-llvm "--with=$SULTANC_PREVIOUS"
    else
        ./build.sh --enable-llvm --bootstrap=wasm
    fi
    build/sultanc version
}

llvm_verify() {
    llvm_env
    ./build.sh verify --enable-llvm
}

# The hosts without a Native backend get a compiler built here (Native-only,
# generated by LLVM); wasm32-wasi and arm64-darwin by the Native backend.
cross_targets="aarch64-linux-gnu x86_64-darwin x86_64-windows-msvc aarch64-windows-msvc"
cross_native_targets="arm64-darwin wasm32-wasi"
cross_build() {
    llvm_env
    [ -x build/sultanc ] || { echo "linux.sh: cross needs the LLVM-enabled build/sultanc (llvm-verify)" >&2; exit 2; }
    cp build/sultanc build/ci-cross-producer
    for t in $cross_targets; do
        ./build.sh --target="$t" --backend=llvm --with=build/ci-cross-producer
    done
    for t in $cross_native_targets; do
        ./build.sh --target="$t" --backend=native --with=build/ci-cross-producer
    done
    ls -l build/cross/*/
}

# The compiler cross-built for this host (downloaded into build/cross).
cross_run() {
    t=aarch64-linux-gnu
    sc=build/cross/$t/sultanc
    chmod 0755 "$sc"
    "$sc" version
    "$sc" check examples/en/basics/hello.sn
    "$sc" run examples/en/basics/hello.sn
}

# The release check (bootstrap/chain.sh): the frozen C Stage0 builds the
# pinned commit's compiler, the chain climbs to the current sources, and the
# result must equal the seed route's compiler. On a host without a Native
# backend its LLVM-enabled compiler is the one the tests run.
# On a Native host the build driver runs it (./build.sh chain); on a host
# without one only Stage1 from the seed exists before the chain, and the
# driver cannot be built with it, so the POSIX script runs it.
chain() {
    llvm_env
    if [ "$native" = yes ]; then
        ./build.sh chain
    else
        sh bootstrap/chain.sh
        cp build/chain/sultanc-a build/sultanc
    fi
}

# The compiler of a release (release.yml): the same routes as seed-build and
# chain, with the version the release reports. On a host without a Native
# backend the chain's LLVM-enabled compiler (build/chain/sultanc-a) builds the
# current sources with LLVM, as its step A does, now with the version.
release_compiler() {
    v=${SULTANC_RELEASE_VERSION:?linux.sh release: set SULTANC_RELEASE_VERSION}
    if [ "$native" = yes ]; then
        ./build.sh verify "--release-version=$v"
    else
        llvm_env
        [ -x build/chain/sultanc-a ] || {
            echo "linux.sh: release needs the chain's compiler build/chain/sultanc-a (linux.sh chain)" >&2
            exit 2
        }
        ./build.sh --enable-llvm --backend=llvm --with=build/chain/sultanc-a "--release-version=$v"
    fi
    build/sultanc version
}

run_tests() {
    if [ -x "$sdk/bin/llvm-config" ]; then llvm_env; fi
    if [ "$native" = no ]; then
        if [ ! -x build/sultanc ]; then
            echo "linux.sh: arm64 has no Native backend; only the release check's chain builds a compiler here" >&2
            return 0
        fi
        # The chain's compiler was generated by LLVM: the runner builds the
        # compiler the same way for its fixpoint check.
        export SULTANC_TEST_STAGE2_BACKEND=llvm
    fi
    ci_run_tests build/sultanc
}

case "$job" in
build) ci_step "Compiler from the previous compiler" build_compiler ;;
seed-build) ci_step "Seed bootstrap, S2 == S3" seed_build ;;
seed-check) ci_step "Seed reproducible" seed_check ;;
seed-update) ci_step "Seed update" seed_update ;;
llvm-sdk) ci_step "LLVM $version SDK" llvm_sdk ;;
llvm-build) ci_step "LLVM-enabled compiler" llvm_build ;;
llvm-verify) ci_step "LLVM fixpoint and cross-backend" llvm_verify ;;
cross) ci_step "Cross-target compilers" cross_build ;;
cross-run) ci_step "Cross-built compiler runs" cross_run ;;
chain) ci_step "C Stage0 chain" chain ;;
release) ci_step "Release compiler" release_compiler ;;
test) ci_step "Tests" run_tests ;;
seed)
    ci_step "Seed bootstrap, S2 == S3" seed_build
    ci_step "Seed reproducible" seed_check
    if [ "$native" = yes ]; then ci_step "Tests" run_tests; fi
    ;;
llvm)
    ci_step "LLVM $version SDK" llvm_sdk
    ci_step "LLVM fixpoint and cross-backend" llvm_verify
    ci_step "Tests" run_tests
    ;;
*)
    echo "usage: linux.sh build|seed-build|seed-check|seed-update|llvm-sdk|llvm-build|llvm-verify|cross|cross-run|chain|release|test|seed|llvm" >&2
    exit 2
    ;;
esac
