# The release check of the frozen C Stage0 on Windows (bootstrap/chain.sh is
# the POSIX one; docs/en/internals/bootstrap.md, "The frozen C Stage0").
#
#   ./bootstrap/chain.ps1 [-Stage0Only]
#
# Run from an MSVC developer environment (cl, link and lib on PATH), with
# $env:LLVM_SDK naming an LLVM SDK that holds include\, lib\LLVM-C.lib and
# bin\LLVM-C.dll (.github/ci/windows.ps1 llvm-sdk builds one).
#
# 1. The C Stage0 (bootstrap\src, C11) is built with cl against LLVM-C.
# 2. It compiles the compiler of the commit bootstrap/FROZEN pins (stage0);
#    link.exe links that object with the C runtime, the system libraries std
#    imports by DLL and LLVM-C: the first compiler of the chain.
# 3. Each `step` commit of bootstrap/FROZEN is built by the compiler before it
#    (LLVM backend: Windows has no Native backend).
# 4. The chain's last compiler builds the current sources (A, LLVM-enabled),
#    and A builds them again, Native-only, for x86_64-linux (D).
# 5. Stage1 from the committed seed (bootstrap.c with cl) builds the same
#    compiler (W): D must equal W byte for byte.
#
# Products: build\chain\ (sultanc-a.exe, beside LLVM-C.dll, is the chain's
# LLVM-enabled compiler for this host).
param([switch]$Stage0Only)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

function Check([string]$what) {
    if ($LASTEXITCODE -ne 0) { throw "$what failed with exit code $LASTEXITCODE" }
}
function Say([string]$text) { Write-Host "chain.ps1: $text" }

$root = Resolve-Path (Join-Path $PSScriptRoot '..')
Set-Location $root
$out = Join-Path $root 'build\chain'
New-Item -ItemType Directory -Force $out | Out-Null
$arm = [System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq [System.Runtime.InteropServices.Architecture]::Arm64
$hostTarget = if ($arm) { 'aarch64-windows-msvc' } else { 'x86_64-windows-msvc' }
$machine = if ($arm) { 'ARM64' } else { 'X64' }
$compareTarget = 'x86_64-linux'
$nativeOnly = '--module-source=compiler.backend.provider.llvm.llvm_codegen=compiler/backend/provider/llvm/unavailable.sn'
# The main-thread stack the compiler executables reserve (64 MiB, address
# space only), as the compiler asks for itself elsewhere.
$stack = '/STACK:67108864'

$sdk = $env:LLVM_SDK
if (-not $sdk -or -not (Test-Path "$sdk\lib\LLVM-C.lib") -or -not (Test-Path "$sdk\bin\LLVM-C.dll")) {
    throw 'chain.ps1: set LLVM_SDK to an LLVM SDK with include\, lib\LLVM-C.lib and bin\LLVM-C.dll'
}
$llvmLib = "$sdk\lib\LLVM-C.lib"
$llvmDll = "$sdk\bin\LLVM-C.dll"
Copy-Item $llvmDll (Join-Path $out 'LLVM-C.dll')

# --- 1. the C Stage0 -----------------------------------------------------------
Say 'building the C Stage0 (cl, C11)'
$objects = Join-Path $out 'stage0-objects'
New-Item -ItemType Directory -Force $objects | Out-Null
$sources = Get-ChildItem -Recurse -Filter '*.c' (Join-Path $root 'bootstrap\src') | Sort-Object FullName
$include = "$sdk\include"
$rootPath = $root.Path
# One object per source, named by its path (two sources share a file name).
$sources | ForEach-Object -ThrottleLimit 4 -Parallel {
    $relative = $_.FullName.Substring($using:rootPath.Length + 1)
    $object = Join-Path $using:objects (($relative -replace '[\\/]', '_') + '.obj')
    cl /nologo /std:c11 /O1 /D_CRT_SECURE_NO_WARNINGS "/I$using:rootPath\bootstrap\include\private" "/I$using:rootPath\bootstrap\include\public" `
        "/I$using:include" /c $_.FullName "/Fo$object" | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "cl $relative failed" }
}
$stage0 = Join-Path $out 'sultanc-stage0.exe'
$stage0Objects = Get-ChildItem -Filter '*.obj' $objects | ForEach-Object { $_.FullName }
link /nologo "/OUT:$stage0" @stage0Objects $llvmLib $stack; Check 'link Stage0'
Say "Stage0: $stage0"
if ($Stage0Only) { exit 0 }

# --- 2 and 3. the pinned commits ------------------------------------------------
# bcryptprimitives.dll (ProcessPrng) has no import library in every Windows
# SDK: made here from a .def file.
$def = Join-Path $out 'bcryptprimitives.def'
$prng = Join-Path $out 'bcryptprimitives.lib'
Set-Content -Path $def -Value "LIBRARY bcryptprimitives.dll`nEXPORTS`n    ProcessPrng`n" -Encoding ascii
lib /nologo "/def:$def" "/out:$prng" "/machine:$machine"; Check 'bcryptprimitives import library'

function Get-Tree([string]$commit) {
    $tree = Join-Path $out ('tree-' + $commit.Substring(0, 12))
    if (-not (Test-Path (Join-Path $tree 'compiler\main.sn'))) {
        git cat-file -e "$commit^{commit}"
        if ($LASTEXITCODE -ne 0) { throw "commit $commit is not in this checkout (git fetch origin $commit, or a full clone)" }
        if (Test-Path $tree) { Remove-Item -Recurse -Force $tree }
        New-Item -ItemType Directory -Force $tree | Out-Null
        $tar = Join-Path $out 'tree.tar'
        git archive --format=tar -o $tar $commit compiler library; Check "git archive $commit"
        tar -xf $tar -C $tree; Check "extract $commit"
        Remove-Item $tar
    }
    return $tree
}

$previous = $null
$index = 0
foreach ($line in Get-Content (Join-Path $root 'bootstrap\FROZEN')) {
    $words = ($line.Trim() -split '\s+') | Where-Object { $_ -ne '' }
    if ($words.Count -eq 0 -or $words[0].StartsWith('#')) { continue }
    if ($words.Count -ne 2) { throw "bootstrap/FROZEN: unexpected line: $line" }
    $kind = $words[0]
    $commit = $words[1]
    $tree = Get-Tree $commit
    $product = Join-Path $out "chain-$index.exe"
    Push-Location $tree
    try {
        if ($kind -eq 'stage0') {
            if ($previous) { throw 'bootstrap/FROZEN: stage0 must be the first line' }
            Say "Stage0 compiles the compiler of $commit"
            # The Stage1 the C route always built: the interpreter's host
            # threads, the copy of later-phase facts and the width-exact raw
            # memory access are their stubs.
            $object = Join-Path $out "chain-$index.obj"
            & $stage0 compiler/main.sn "--target=$hostTarget" `
                '--module-source=compiler.interpreter.machine.host_threads=compiler/interpreter/machine/host_threads_unavailable.sn' `
                '--module-source=compiler.frontend.later_phase_facts_copy=compiler/frontend/later_phase_facts_copy_unavailable.sn' `
                '--module-source=compiler.interpreter.machine.raw_memory_access=compiler/interpreter/machine/raw_memory_access_unavailable.sn' `
                -o $object; Check 'Stage0'
            # Stage0's object names no default library: the DLL C runtime
            # (msvcrt with ucrt and vcruntime, which give mainCRTStartup,
            # __chkstk and _fltused), the system libraries std imports by DLL
            # (library/std/os/platform/windows/win32) and LLVM-C.
            link /nologo "/OUT:$product" $object msvcrt.lib ucrt.lib vcruntime.lib ws2_32.lib crypt32.lib `
                synchronization.lib ntdll.lib $prng $llvmLib $stack; Check 'link the first compiler'
        } elseif ($kind -eq 'step') {
            if (-not $previous) { throw 'bootstrap/FROZEN: a step needs the stage0 line before it' }
            Say "the compiler of the line before builds the compiler of $commit"
            & $previous compiler/main.sn -O2 --backend=llvm "--target=$hostTarget" -o $product; Check "step $commit"
        } else {
            throw "bootstrap/FROZEN: unknown line kind $kind (stage0 or step)"
        }
    } finally {
        Pop-Location
    }
    $previous = $product
    $index++
}
if (-not $previous) { throw 'bootstrap/FROZEN names no stage0 commit' }

# --- 4. the current sources ------------------------------------------------------
$a = Join-Path $out 'sultanc-a.exe'
$d = Join-Path $out 'sultanc-d'
$w = Join-Path $out 'sultanc-w'
Say "the chain's last compiler builds the current sources (A, LLVM-enabled)"
& $previous compiler/main.sn -O2 --backend=llvm "--target=$hostTarget" -o $a; Check 'A'
Say "A builds them again, Native-only, for $compareTarget (D)"
& $a compiler/main.sn -O2 --backend=native "--target=$compareTarget" $nativeOnly -o $d; Check 'D'

# --- 5. the seed route -----------------------------------------------------------
Say 'Stage1 from the seed builds the same compiler (W)'
$seed = Join-Path $root 'build\bootstrap\seed-msvc'
New-Item -ItemType Directory -Force $seed | Out-Null
cl /nologo "/Fe:$seed\build-bootstrap.exe" bootstrap-wasm/bootstrap.c; Check 'bootstrap.c'
& "$seed\build-bootstrap.exe" --cc=cl "--out=$seed" --stage1-only; Check 'Stage1 from the seed'
& "$seed\sultanc-stage1.exe" compiler/main.sn -O2 --backend=native "--target=$compareTarget" $nativeOnly -o $w; Check 'W'
if ((Get-FileHash $d).Hash -ne (Get-FileHash $w).Hash) {
    throw "the C Stage0 chain and the seed give different compilers: $d and $w differ"
}
Say "the C Stage0 chain and the seed give one compiler ($compareTarget): $d == $w"
Say "the chain's LLVM-enabled compiler for ${hostTarget}: $a"
