# Windows CI (x64 and ARM64). PowerShell. Windows has no SultanC Native
# backend: the nightly tiers build Stage1 from the seed here and run the
# compiler cross-built for Windows on linux-x86_64; the release check builds an
# LLVM-enabled Windows compiler from the frozen C Stage0. One job per workflow
# step (as linux.sh and macos.sh):
#
#   seed          Stage1 from the seed (bootstrap.c, no LLVM) with every C
#                 toolchain: MSVC (cl), clang-cl, and MinGW gcc (x64). The
#                 MSVC Stage1 rebuilds the committed seed, which must match
#                 byte for byte.
#   cross-run     the compiler cross-built for this host on linux-x86_64 runs
#   llvm-sdk      the pinned LLVM SDK (llvm.lock) in $env:LLVM_SDK, unless cached
#   chain         (release check) bootstrap/chain.ps1: the frozen C Stage0 ->
#                 the pinned commit's compiler -> the current compiler, equal
#                 to the seed route's; the LLVM-enabled result -> build\sultanc.exe
#   release       (release.yml) the chain's LLVM-enabled compiler (chain, run
#                 before) builds the current sources once more with LLVM and
#                 the version the release reports ($env:SULTANC_RELEASE_VERSION)
#                 -> build\sultanc.exe
#   test          the test suite on build\sultanc.exe, reported to the job summary
#
# LLVM: the upstream Windows archive builds only some targets, so the SDK
# (LLVM-C.dll + LLVM-C.lib) is built from the pinned source with the pinned
# archive's clang-cl, once, and cached by the workflow in $env:LLVM_SDK.
param([Parameter(Mandatory)][ValidateSet('seed', 'cross-run', 'llvm-sdk', 'chain', 'release', 'test')][string]$Job)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

function Check([string]$what) {
    if ($LASTEXITCODE -ne 0) { throw "$what failed with exit code $LASTEXITCODE" }
}

$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
Set-Location $root
$lock = Get-Content (Join-Path $PSScriptRoot 'llvm.lock') | Where-Object { $_ -match '^LLVM_VERSION=' }
$version = ($lock -split '=', 2)[1].Trim()
$arm = $env:PROCESSOR_ARCHITECTURE -eq 'ARM64'
$arch = if ($arm) { 'arm64' } else { 'amd64' }
$triple = if ($arm) { 'aarch64-pc-windows-msvc' } else { 'x86_64-pc-windows-msvc' }
$base = "https://github.com/llvm/llvm-project/releases/download/llvmorg-$version"
$sdk = if ($env:LLVM_SDK) { $env:LLVM_SDK } else { Join-Path $HOME "llvm-sdk-$version" }
$work = Join-Path $env:RUNNER_TEMP 'llvm-work'

# Downloads are pinned by version and file name.
function Fetch([string]$name) {
    New-Item -ItemType Directory -Force $work | Out-Null
    $file = Join-Path $work $name
    if (-not (Test-Path $file)) { Invoke-WebRequest "$base/$name" -OutFile $file }
    tar -xf $file -C $work; Check "extract $name"
}

# The pinned upstream clang-cl/lld-link (also the clang-cl seed driver).
function Get-HostLlvm {
    $bin = Join-Path $work "clang+llvm-$version-$triple\bin"
    if (-not (Test-Path "$bin\clang-cl.exe")) { Fetch "clang+llvm-$version-$triple.tar.xz" }
    return $bin
}

# The compiler cross-built for this host on linux-x86_64 (nightly, ci.yml
# cross-run; downloaded into build\cross): it runs, checks and interprets.
if ($Job -eq 'cross-run') {
    $t = if ($arm) { 'aarch64-windows-msvc' } else { 'x86_64-windows-msvc' }
    $sc = "build\cross\$t\sultanc.exe"
    & $sc version; Check "$sc version"
    & $sc check examples/en/basics/hello.sn; Check "$sc check"
    & $sc run examples/en/basics/hello.sn; Check "$sc run"
    exit 0
}

# Native MSVC developer environment of the runner's Visual Studio.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products '*' -property installationPath
Import-Module (Join-Path $vs 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments "-arch=$arch -host_arch=$arch"

# Stage1 from the seed with bootstrap.c and the C compiler named, into $out.
function Invoke-SeedStage1([string]$cc, [string]$out) {
    if (Test-Path $out) { Remove-Item -Recurse -Force $out }
    New-Item -ItemType Directory -Force $out | Out-Null
    cl /nologo "/Fe:$out\build-bootstrap.exe" bootstrap-wasm/bootstrap.c; Check 'bootstrap.c'
    & "$out\build-bootstrap.exe" "--cc=$cc" "--out=$out" --jobs=4 --stage1-only; Check "Stage1 ($cc)"
    & "$out\sultanc-stage1.exe" version; Check "Stage1 ($cc) run"
}

if ($Job -eq 'seed') {
    # 1. MSVC (cl), then the seed check: this Stage1 rebuilds the committed
    #    seed (Native-only, wasm32-wasi, -O2), which must match byte for byte.
    $out = 'build\ci-seed-msvc'
    Invoke-SeedStage1 'cl' $out
    & "$out\sultanc-stage1.exe" compiler/main.sn -O2 --backend=native --target=wasm32-wasi `
        '--module-source=compiler.backend.provider.llvm.llvm_codegen=compiler/backend/provider/llvm/unavailable.sn' `
        -o "$out\sultanc.wasm"; Check 'seed rebuild'
    & "$out\sultanc-wasm2c.exe" --decompress bootstrap-wasm/seed/sultanc.wasm.zst "$out\sultanc.committed.wasm"; Check 'seed decompress'
    if ((Get-FileHash "$out\sultanc.wasm").Hash -ne (Get-FileHash "$out\sultanc.committed.wasm").Hash) {
        throw 'seed check: the current sources give a different seed (./build.sh seed-update)'
    }
    Write-Host 'seed check: the committed seed is reproduced exactly'

    # 2. clang-cl (the pinned upstream clang-cl).
    Invoke-SeedStage1 (Join-Path (Get-HostLlvm) 'clang-cl.exe') 'build\ci-seed-clang-cl'

    # 3. MinGW gcc (x64 runners ship MinGW-w64).
    if (-not $arm) {
        $out = 'build\ci-seed-mingw'
        if (Test-Path $out) { Remove-Item -Recurse -Force $out }
        New-Item -ItemType Directory -Force $out | Out-Null
        gcc -O2 -o "$out\build-bootstrap.exe" bootstrap-wasm/bootstrap.c; Check 'MinGW driver'
        & "$out\build-bootstrap.exe" --cc=gcc --out=$out --stage1-only; Check 'MinGW Stage1'
        & "$out\sultanc-stage1.exe" version; Check 'MinGW Stage1 run'
    }
    exit 0
}

# --- LLVM SDK and the release check ----------------------------------------------
function Write-Summary([string]$line) {
    if ($env:GITHUB_STEP_SUMMARY) { Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $line -Encoding utf8 }
    else { Write-Host "summary: $line" }
}

function Step([string]$name, [scriptblock]$body) {
    Write-Host "::group::$name"
    $watch = [Diagnostics.Stopwatch]::StartNew()
    # The step's output is also kept, so a failure can show its end outside
    # the collapsed group.
    $log = Join-Path $root 'build\ci-step.log'
    New-Item -ItemType Directory -Force (Split-Path $log) | Out-Null
    try {
        & $body *>&1 | Tee-Object -FilePath $log | Out-Host
        $ok = $true
    } catch {
        $ok = $false
        $failure = $_
    }
    $watch.Stop()
    Write-Host '::endgroup::'
    # A failed test run's failures (report.py), outside the collapsed group.
    $failures = Join-Path $root 'build\tests-failures.txt'
    if (-not $ok -and (Test-Path $failures)) { Get-Content -Encoding utf8 $failures | Write-Host }
    elseif (-not $ok -and (Test-Path $log)) {
        Write-Host "--- $name failed; the last 80 lines of its output:"
        Get-Content $log -Tail 80 | Write-Host
    }
    $time = '{0}:{1:00}' -f [int][Math]::Floor($watch.Elapsed.TotalMinutes), $watch.Elapsed.Seconds
    if ($ok) { Write-Summary "- ✅ $name ($time)" }
    else {
        Write-Summary "- ❌ ${name}: $failure ($time)"
        Write-Host "::error title=$name::$failure"
        throw $failure
    }
}

function Invoke-LlvmSdk {
    if (Test-Path "$sdk\bin\LLVM-C.dll") { Write-Host "LLVM SDK restored from cache: $sdk"; return }
    $hostLlvm = Get-HostLlvm
    Fetch "llvm-project-$version.src.tar.xz"
    $install = Join-Path $work 'install'
    cmake -S "$work\llvm-project-$version.src\llvm" -B "$work\build" -G Ninja `
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$install" `
        -DCMAKE_C_COMPILER="$hostLlvm\clang-cl.exe" -DCMAKE_CXX_COMPILER="$hostLlvm\clang-cl.exe" `
        -DCMAKE_LINKER="$hostLlvm\lld-link.exe" -DCMAKE_AR="$hostLlvm\llvm-lib.exe" `
        -DLLVM_TARGETS_TO_BUILD=all -DLLVM_BUILD_LLVM_C_DYLIB=ON `
        -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
        -DLLVM_ENABLE_ZLIB=OFF -DLLVM_ENABLE_ZSTD=OFF -DLLVM_ENABLE_LIBXML2=OFF -DLLVM_ENABLE_DIA_SDK=OFF `
        -DLLVM_INCLUDE_TESTS=OFF -DLLVM_INCLUDE_EXAMPLES=OFF -DLLVM_INCLUDE_BENCHMARKS=OFF `
        -DLLVM_INCLUDE_DOCS=OFF -DLLVM_ENABLE_BINDINGS=OFF -DLLVM_BUILD_UTILS=OFF `
        -DLLVM_PARALLEL_LINK_JOBS=1; Check 'LLVM configure'
    cmake --build "$work\build" --parallel 4; Check 'LLVM build'
    cmake --install "$work\build"; Check 'LLVM install'
    # Keep only what the SultanC build uses: a small cache.
    New-Item -ItemType Directory -Force "$sdk\bin", "$sdk\lib" | Out-Null
    foreach ($f in 'llvm-config.exe', 'llvm-nm.exe', 'llvm-readobj.exe', 'LLVM-C.dll') {
        Copy-Item "$install\bin\$f" "$sdk\bin\"
    }
    Copy-Item "$install\lib\LLVM-C.lib" "$sdk\lib\"
    Copy-Item -Recurse "$install\include" "$sdk\include"
    Remove-Item -Recurse -Force $work
    Write-Host "LLVM SDK built: $sdk"
}

# The release check (bootstrap/chain.ps1): the frozen C Stage0 builds the
# pinned commit's compiler, the chain climbs to the current sources, and the
# result must equal the seed route's compiler. Its LLVM-enabled compiler is
# the one the tests run.
function Invoke-Chain {
    if (-not (Test-Path "$sdk\bin\LLVM-C.dll")) { throw "no LLVM SDK in $sdk (run: windows.ps1 llvm-sdk)" }
    $env:LLVM_SDK = $sdk
    & (Join-Path $root 'bootstrap\chain.ps1'); Check 'C Stage0 chain'
    Copy-Item build\chain\sultanc-a.exe build\sultanc.exe
    Copy-Item "$sdk\bin\LLVM-C.dll" build\LLVM-C.dll
    & build\sultanc.exe version; Check 'chain compiler run'
}

# The compiler of a release (release.yml): the chain's LLVM-enabled compiler
# builds the current sources as the chain's step A does, with the release
# version bound as the module compiler.version. The build driver's
# --release-version (tools/build/steps.sn) writes the same module; the driver
# does not run here (build.sh is POSIX, and its LLVM selection knows no
# LLVM-C.dll), so this writes it.
function Invoke-Release {
    $version = $env:SULTANC_RELEASE_VERSION
    if (-not $version -or $version -cnotmatch '^[A-Za-z0-9.+_-]+$') {
        throw 'set SULTANC_RELEASE_VERSION (letters, digits and . + _ - only)'
    }
    $a = 'build\chain\sultanc-a.exe'
    if (-not (Test-Path $a)) { throw "no $a (run: windows.ps1 chain)" }
    $generated = 'build\bootstrap\generated'
    New-Item -ItemType Directory -Force $generated, 'build\release' | Out-Null
    $module = "$generated\compiler_version.sn"
    $source = "// Generated by .github/ci/windows.ps1 release, as the build driver writes it for`n" +
        "// --release-version; bound to the module compiler.version in place of compiler/version.sn.`n" +
        "عام عرف إصدار_السلطان(): اختياري نص {`n" +
        "    أرجع اختياري.موجود(«${version}»);`n}`n"
    [IO.File]::WriteAllText((Join-Path $root $module), $source, (New-Object Text.UTF8Encoding $false))
    $hostTarget = if ($arm) { 'aarch64-windows-msvc' } else { 'x86_64-windows-msvc' }
    $env:SULTANC_CACHE = 'off'
    & $a compiler/main.sn -O2 --backend=llvm "--target=$hostTarget" `
        "--module-source=compiler.version=$($module -replace '\\', '/')" -o build\release\sultanc.exe
    Check 'release compiler'
    Copy-Item -Force build\release\sultanc.exe build\sultanc.exe
    if (-not (Test-Path build\LLVM-C.dll)) { Copy-Item "$sdk\bin\LLVM-C.dll" build\LLVM-C.dll }
    & build\sultanc.exe version; Check 'release compiler run'
}

function Invoke-Tests {
    # The test suite is a bash script; Git for Windows provides bash on the runner.
    $env:PATH = "$sdk\bin;$env:PATH"
    # The compiler under test was generated by LLVM (the chain): the runner
    # builds the compiler the same way for its fixpoint check.
    $env:SULTANC_TEST_STAGE2_BACKEND = 'llvm'
    # The suite's target is the machine's own, as the chain built the
    # compiler for it. ci_run.sh runs in Git for
    # Windows' bash, an x64 program that runs emulated on an ARM64 runner and
    # sees an x64 machine: the ARM64 job tested x86_64 code, and its
    # convergence check ran an x86_64 Stage3 under emulation.
    $arch = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture
    $env:TARGET = if ($arch -eq [System.Runtime.InteropServices.Architecture]::Arm64) { 'aarch64-windows-msvc' } else { 'x86_64-windows-msvc' }
    Write-Host "Test target: $env:TARGET (OS architecture $arch)"
    Remove-Item -Force -ErrorAction SilentlyContinue (Join-Path $root 'build\tests-failures.txt')
    & "$env:ProgramFiles\Git\bin\bash.exe" tools/tests/ci_run.sh
    $status = $LASTEXITCODE
    python "$PSScriptRoot\report.py" "$root"
    if ($status -ne 0) { throw "tests failed with exit code $status" }
}

switch ($Job) {
    'llvm-sdk' { Step "LLVM $version SDK" { Invoke-LlvmSdk } }
    'chain' { Step 'C Stage0 chain' { Invoke-Chain } }
    'release' { Step 'Release compiler' { Invoke-Release } }
    'test' { Step 'Tests' { Invoke-Tests } }
}
