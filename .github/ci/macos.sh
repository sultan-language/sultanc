#!/bin/sh
# macOS CI (arm64 and x86_64). POSIX sh: macOS runners ship bash 3.2.
# One subcommand per workflow step (see linux.sh for the same layout):
#
#   build         pull-request tier: one build of the compiler with the
#                 previous compiler ($SULTANC_PREVIOUS, else the seed)
#                   arm64  (Native backend): -> build/sultanc
#                   x86_64 (no Native backend): Stage1 from the seed only
#   seed-build    main and nightly: arm64: seed -> Stage1 -> S2 -> S3, S2 == S3;
#                 x86_64: seed -> Stage1
#   seed-check    the committed seed equals what the sources rebuild
#   llvm-sdk      the pinned LLVM SDK (llvm.lock) in $LLVM_SDK, unless cached
#   llvm-build    (arm64) one build of an LLVM-enabled compiler from the previous compiler
#   llvm-verify   (arm64, nightly) LLVM-enabled S2 == S3, the LLVM fixpoint and
#                 cross-backend equality
#   cross-run     (x86_64, nightly) the compiler cross-built for this host on
#                 linux-x86_64 runs: version, check, run under the interpreter
#   chain         (release check) bootstrap/chain.sh: the frozen C Stage0 ->
#                 the pinned commit's compiler -> the current compiler, equal
#                 to the seed route's; on a host without a Native backend its
#                 LLVM-enabled compiler becomes build/sultanc
#   release       (release.yml) the compiler of a release, reporting
#                 $SULTANC_RELEASE_VERSION (./build.sh --release-version):
#                   Native host: from the seed, S2 == S3 (as seed-build)
#                   no Native backend: the chain's LLVM-enabled compiler
#                   (chain, run before) builds it once more with LLVM
#   test          the test suite on build/sultanc, reported to the job summary
#   seed | llvm   every step of that route in order (for local runs)
#
# LLVM, pinned by version and file name (llvm.lock), cached by the workflow
# in $LLVM_SDK:
#   arm64:  the upstream release archive; it ships only component archives
#           (LLVM 22 bitcode, which Apple ld cannot read), so libLLVM.dylib is
#           linked here from them with the archive's own clang and lld.
#   x86_64: no upstream archive exists for 22.1.8, so the pinned source is
#           built: only libLLVM and the three tools the build uses (hours,
#           once, in its own job; then cached).
set -eu

job=${1:?usage: macos.sh build|seed-build|seed-check|llvm-sdk|llvm-build|llvm-verify|cross-run|chain|release|test|seed|llvm}
# shellcheck source=.github/ci/common.sh
. "$(dirname "$0")/common.sh"
cd "$ci_root"
ci_load_llvm_lock
version=$LLVM_VERSION
major=${version%%.*}
sdk=${LLVM_SDK:-$HOME/llvm-sdk-$version}
# Seconds a build of the compiler may take before it is reported as hung.
SULTANC_COMPILE_TIMEOUT=${SULTANC_CI_COMPILE_TIMEOUT_SECONDS:-5400}
export SULTANC_COMPILE_TIMEOUT

case "$(uname -m)" in
arm64) native=yes ;;
x86_64) native=no ;;
*)
    echo "macos.sh: unsupported machine $(uname -m)" >&2
    exit 2
    ;;
esac

fetch() { # url file
    curl -fL --retry 3 -o "$2" "$1"
}

llvm_sdk() {
    if [ -f "$sdk/lib/libLLVM.dylib" ] && [ -x "$sdk/bin/llvm-config" ]; then
        echo "LLVM SDK restored from cache: $sdk"
        return
    fi
    work=${RUNNER_TEMP:-/tmp}/llvm-work
    rm -rf "$work" "$sdk"
    mkdir -p "$work" "$sdk/bin" "$sdk/lib"
    base=https://github.com/llvm/llvm-project/releases/download/llvmorg-$version
    if [ "$native" = yes ]; then
        asset=LLVM-$version-macOS-ARM64
        fetch "$base/$asset.tar.xz" "$work/$asset.tar.xz"
        # Only what the build uses: headers, component archives, tools.
        tar -xf "$work/$asset.tar.xz" -C "$work" \
            "$asset/include" "$asset/lib/libLLVM*.a" "$asset/lib/libPolly*.a" \
            "$asset/bin/llvm-config" "$asset/bin/llvm-nm" "$asset/bin/llvm-readobj" \
            "$asset/bin/clang" "$asset/bin/clang++" "$asset/bin/clang-22" \
            "$asset/bin/lld" "$asset/bin/ld64.lld"
        rm -f "$work/$asset.tar.xz"
        root=$work/$asset
        # LineEditor (libedit, used only by LLVM's interactive tools) is not
        # part of the C API and its libedit dependency is not in
        # --system-libs: leave it out of the dylib.
        archives=$("$root/bin/llvm-config" --link-static --libfiles all | tr ' ' '\n' |
            grep -v 'libLLVMLineEditor\.a$' | tr '\n' ' ')
        system_libs=$("$root/bin/llvm-config" --link-static --system-libs)
        # shellcheck disable=SC2086 # one word per archive / library
        SDKROOT=$(xcrun --show-sdk-path) "$root/bin/clang++" -fuse-ld=lld \
            -dynamiclib -o "$root/lib/libLLVM.dylib" -install_name @rpath/libLLVM.dylib \
            -Wl,-all_load $archives $system_libs
        cp "$root/bin/llvm-config" "$root/bin/llvm-nm" "$root/bin/llvm-readobj" "$sdk/bin/"
        cp -R "$root/include" "$sdk/include"
        # The real file carries the major (libLLVM-22.dylib), as an LLVM
        # dylib build names it; build.sh (llvm_library) looks for that name
        # first.
        cp "$root/lib/libLLVM.dylib" "$sdk/lib/libLLVM-$major.dylib"
        ln -sf "libLLVM-$major.dylib" "$sdk/lib/libLLVM.dylib"
    else
        fetch "$base/llvm-project-$version.src.tar.xz" "$work/src.tar.xz"
        tar -xf "$work/src.tar.xz" -C "$work"
        cmake -S "$work/llvm-project-$version.src/llvm" -B "$work/build" \
            -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$sdk" \
            -DLLVM_TARGETS_TO_BUILD=all \
            -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_LINK_LLVM_DYLIB=ON \
            -DLLVM_ENABLE_LIBXML2=OFF -DLLVM_ENABLE_ZSTD=OFF -DLLVM_ENABLE_LIBEDIT=OFF \
            -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF \
            -DLLVM_INCLUDE_BENCHMARKS=OFF -DLLVM_INCLUDE_DOCS=OFF \
            -DLLVM_ENABLE_BINDINGS=OFF -DLLVM_BUILD_UTILS=OFF
        jobs=$(sysctl -n hw.ncpu)
        # Only libLLVM and the tools the build uses, installed by component;
        # the full build and install remain the fallback.
        if cmake --build "$work/build" --parallel "$jobs" \
            --target LLVM llvm-config llvm-nm llvm-readobj; then
            for component in LLVM llvm-config llvm-nm llvm-readobj llvm-headers; do
                cmake --install "$work/build" --component "$component"
            done
        else
            cmake --build "$work/build" --parallel "$jobs"
            cmake --install "$work/build"
        fi
    fi
    rm -rf "$work"
    echo "LLVM SDK built: $sdk"
}

llvm_env() {
    [ -x "$sdk/bin/llvm-config" ] || {
        echo "macos.sh: no LLVM SDK in $sdk (run: macos.sh llvm-sdk)" >&2
        exit 2
    }
    # An SDK cached before the dylib carried its major has the real file at
    # libLLVM.dylib, and libLLVM-<major>.dylib as a link to it or not at all:
    # the real file takes the versioned name, and libLLVM.dylib links to it.
    if [ -f "$sdk/lib/libLLVM.dylib" ] && [ ! -L "$sdk/lib/libLLVM.dylib" ] &&
        { [ -L "$sdk/lib/libLLVM-$major.dylib" ] || [ ! -e "$sdk/lib/libLLVM-$major.dylib" ]; }; then
        rm -f "$sdk/lib/libLLVM-$major.dylib"
        mv "$sdk/lib/libLLVM.dylib" "$sdk/lib/libLLVM-$major.dylib"
        ln -s "libLLVM-$major.dylib" "$sdk/lib/libLLVM.dylib"
    fi
    LLVM_CONFIG="$sdk/bin/llvm-config"
    DYLD_LIBRARY_PATH="$sdk/lib${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
    export LLVM_CONFIG DYLD_LIBRARY_PATH
    echo "LLVM $("$LLVM_CONFIG" --version) from $sdk"
    # What the build's LLVM selection will see (build.sh, llvm_library).
    echo "llvm-config --prefix: $("$LLVM_CONFIG" --prefix 2>&1)"
    echo "llvm-config --libdir: $("$LLVM_CONFIG" --libdir 2>&1)"
    found=no
    for lib in "$sdk"/lib/libLLVM.* "$sdk"/lib/libLLVM-*; do
        [ -e "$lib" ] || [ -L "$lib" ] || continue
        ls -l "$lib"
        found=yes
    done
    [ "$found" = yes ] || echo "no libLLVM dylib in $sdk/lib"
}

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

cross_run() {
    sc=build/cross/x86_64-darwin/sultanc
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
    v=${SULTANC_RELEASE_VERSION:?macos.sh release: set SULTANC_RELEASE_VERSION}
    if [ "$native" = yes ]; then
        ./build.sh verify "--release-version=$v"
    else
        llvm_env
        [ -x build/chain/sultanc-a ] || {
            echo "macos.sh: release needs the chain's compiler build/chain/sultanc-a (macos.sh chain)" >&2
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
            echo "macos.sh: x86_64 has no Native backend; only the release check's chain builds a compiler here" >&2
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
llvm-sdk) ci_step "LLVM $version SDK" llvm_sdk ;;
llvm-build) ci_step "LLVM-enabled compiler" llvm_build ;;
llvm-verify) ci_step "LLVM fixpoint and cross-backend" llvm_verify ;;
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
    echo "usage: macos.sh build|seed-build|seed-check|llvm-sdk|llvm-build|llvm-verify|cross-run|chain|release|test|seed|llvm" >&2
    exit 2
    ;;
esac
