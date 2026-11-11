# Windows CI (x64 and ARM64). PowerShell. Windows has no SultanC Native
# backend, so a Stage2 always comes from the C Stage0 and LLVM. One job per
# workflow step (as linux.sh and macos.sh):
#
#   seed          wasm seed through cmake/ (no LLVM), Stage1 only, with every
#                 C toolchain: MSVC (cl), clang-cl, and MinGW gcc (x64;
#                 bootstrap.c). The MSVC Stage1 rebuilds the committed seed,
#                 which must match byte for byte.
#   llvm-sdk      the pinned LLVM SDK (llvm.lock) in $env:LLVM_SDK, unless cached
#   llvm-build    C Stage0 against the SDK -> Stage1 -> LLVM-generated Stage2
#   llvm-verify   LLVM Stage3 == Stage4
#   test          the test suite on build\sultanc.exe, reported to the job summary
#   llvm          llvm-sdk, llvm-build, llvm-verify and test in order
#
# LLVM: the upstream Windows archive builds only some targets, so the SDK
# (LLVM-C.dll + LLVM-C.lib) is built from the pinned source with the pinned
# archive's clang-cl, once, and cached by the workflow in $env:LLVM_SDK.
param([Parameter(Mandatory)][ValidateSet('seed', 'llvm-sdk', 'llvm-build', 'llvm-verify', 'test', 'llvm')][string]$Job)
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

# Native MSVC developer environment of the runner's Visual Studio.
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vs = & $vswhere -latest -products '*' -property installationPath
Import-Module (Join-Path $vs 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments "-arch=$arch -host_arch=$arch"

if ($Job -eq 'seed') {
    # 1. MSVC through the main CMake build, then the seed check.
    $tree = 'build\bootstrap\cmake-wasm'
    if (Test-Path $tree) { Remove-Item -Recurse -Force $tree }
    cmake -S cmake -B $tree -G Ninja -DCMAKE_C_COMPILER=cl -DCMAKE_BUILD_TYPE=Release `
        -DSULTANC_BOOTSTRAP=wasm -DSULTANC_REQUESTED_STAGE=stage1; Check 'MSVC configure'
    cmake --build $tree --parallel 4 --target sultanc-stage1; Check 'MSVC Stage1'
    cmake --build $tree --target sultanc-check-seed; Check 'seed check'

    # 2. clang-cl through the same CMake build (the pinned upstream clang-cl).
    $clangCl = Join-Path (Get-HostLlvm) 'clang-cl.exe'
    $tree = 'build\bootstrap\cmake-wasm-clang-cl'
    if (Test-Path $tree) { Remove-Item -Recurse -Force $tree }
    cmake -S cmake -B $tree -G Ninja -DCMAKE_C_COMPILER="$clangCl" -DCMAKE_BUILD_TYPE=Release `
        -DSULTANC_BOOTSTRAP=wasm -DSULTANC_REQUESTED_STAGE=stage1; Check 'clang-cl configure'
    cmake --build $tree --parallel 4 --target sultanc-stage1; Check 'clang-cl Stage1'
    & "$tree\bin\sultanc-stage1.exe" version; Check 'clang-cl Stage1 run'

    # 3. MinGW gcc with bootstrap.c (x64 runners ship MinGW-w64).
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

# --- LLVM route ---------------------------------------------------------------
function Write-Summary([string]$line) {
    if ($env:GITHUB_STEP_SUMMARY) { Add-Content -Path $env:GITHUB_STEP_SUMMARY -Value $line -Encoding utf8 }
    else { Write-Host "summary: $line" }
}

function Step([string]$name, [scriptblock]$body) {
    Write-Host "::group::$name"
    $watch = [Diagnostics.Stopwatch]::StartNew()
    try {
        & $body
        $ok = $true
    } catch {
        $ok = $false
        $failure = $_
    }
    $watch.Stop()
    Write-Host '::endgroup::'
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

$tree = 'build\bootstrap\cmake'

function Invoke-LlvmBuild {
    if (-not (Test-Path "$sdk\bin\LLVM-C.dll")) { throw "no LLVM SDK in $sdk (run: windows.ps1 llvm-sdk)" }
    cmake -S cmake -B $tree -G Ninja `
        -DSULTANC_BOOTSTRAP=c -DSULTANC_COMPILER_LLVM=ON -DSULTANC_STAGE2_BACKEND=llvm `
        -DSULTANC_REQUESTED_STAGE=verify -DSULTANC_LLVM_ROOT="$sdk"; Check 'configure'
    cmake --build $tree --target sultanc-stage2; Check 'Stage0 -> Stage1 -> LLVM Stage2'
    & build\sultanc.exe version; Check 'Stage2 run'
}

function Invoke-LlvmVerify {
    cmake --build $tree --target sultanc-verify; Check 'LLVM Stage3 == Stage4'
}

function Invoke-Tests {
    # The test suite is a bash script; Git for Windows provides bash on the runner.
    $env:PATH = "$sdk\bin;$env:PATH"
    & "$env:ProgramFiles\Git\bin\bash.exe" tools/tests/ci_run.sh
    $status = $LASTEXITCODE
    python "$PSScriptRoot\report.py" "$root"
    if ($status -ne 0) { throw "tests failed with exit code $status" }
}

switch ($Job) {
    'llvm-sdk' { Step "LLVM $version SDK" { Invoke-LlvmSdk } }
    'llvm-build' { Step 'LLVM-generated Stage2' { Invoke-LlvmBuild } }
    'llvm-verify' { Step 'Convergence' { Invoke-LlvmVerify } }
    'test' { Step 'Tests' { Invoke-Tests } }
    'llvm' {
        Step "LLVM $version SDK" { Invoke-LlvmSdk }
        Step 'LLVM-generated Stage2' { Invoke-LlvmBuild }
        Step 'Convergence' { Invoke-LlvmVerify }
        Step 'Tests' { Invoke-Tests }
    }
}
