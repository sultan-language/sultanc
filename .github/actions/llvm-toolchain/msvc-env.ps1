# Activate the runner-provided native Visual Studio/Windows SDK environment.
# Architecture details are supplied by ci_hosts.bootstrap_environment, not
# inferred through a list of Windows runner/architecture names.
$ErrorActionPreference = 'Stop'

$architecture = $env:SULTANC_MSVC_ARCH
$toolArchitecture = $env:SULTANC_MSVC_TOOL_ARCH
$machineText = $env:SULTANC_MSVC_PE_MACHINE
$architectureAliases = @($env:SULTANC_MSVC_ARCH_ALIASES -split ',')
$invalidAliases = @($architectureAliases | Where-Object { $_ -notmatch '^[a-zA-Z0-9_]+$' })
if (-not $env:SULTANC_MSVC_ARCH_ALIASES -or
    $invalidAliases.Count -gt 0 -or
    $architectureAliases -notcontains $architecture -or
    $architecture -notmatch '^[a-zA-Z0-9_]+$' -or
    $toolArchitecture -notmatch '^[a-zA-Z0-9_]+$' -or
    $machineText -notmatch '^0x[0-9a-fA-F]{4}$') {
    throw 'Native MSVC architecture and PE-machine contract is incomplete'
}
$expectedMachine = [Convert]::ToUInt16($machineText.Substring(2), 16)

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw "MSVC discovery tool missing: $vswhere" }
$installation = & $vswhere -latest -products '*' -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installation) { throw 'Native Visual Studio installation not found' }
$module = Join-Path $installation 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
if (-not (Test-Path $module)) { throw "MSVC developer shell missing: $module" }

$previousPath = @($env:PATH -split ';')
Import-Module $module
Enter-VsDevShell -VsInstallPath $installation -SkipAutomaticLocation -DevCmdArguments "-arch=$architecture -host_arch=$architecture"
if ($architectureAliases -notcontains $env:VSCMD_ARG_TGT_ARCH -or
    $architectureAliases -notcontains $env:VSCMD_ARG_HOST_ARCH) {
    throw "Native MSVC architecture mismatch: target=$env:VSCMD_ARG_TGT_ARCH host=$env:VSCMD_ARG_HOST_ARCH configured=$architecture"
}

# Ensure PATH selects the native compiler from the selected Visual Studio
# installation, never a cross-compiler silently selected by its runner image.
$compiler = Get-Command cl.exe -CommandType Application -ErrorAction Stop
$compilerPath = [IO.Path]::GetFullPath($compiler.Source)
$toolsetRoot = [IO.Path]::GetFullPath((Join-Path $installation 'VC\Tools\MSVC')).TrimEnd('\') + '\'
if (-not $compilerPath.StartsWith($toolsetRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "MSVC cl.exe is outside the selected Visual Studio toolchain: $compilerPath"
}
$relativeCompiler = $compilerPath.Substring($toolsetRoot.Length)
$expectedCompiler = "^[^\\]+\\bin\\Host${toolArchitecture}\\${toolArchitecture}\\cl\.exe$"
if ($relativeCompiler -notmatch $expectedCompiler) {
    throw "MSVC cl.exe does not match native Host${toolArchitecture}/${toolArchitecture}: $compilerPath"
}

# Verify the executable's PE machine, not just the directory name.
$stream = [IO.File]::OpenRead($compilerPath)
$reader = [IO.BinaryReader]::new($stream)
try {
    if ($stream.Length -lt 64) { throw "MSVC cl.exe has an invalid PE header: $compilerPath" }
    $stream.Position = 0x3c
    $peOffset = $reader.ReadInt32()
    if ($peOffset -lt 0 -or $peOffset -gt ($stream.Length - 6)) {
        throw "MSVC cl.exe has an invalid PE offset: $compilerPath"
    }
    $stream.Position = $peOffset
    if ($reader.ReadUInt32() -ne 0x00004550) { throw "MSVC cl.exe is not PE/COFF: $compilerPath" }
    $machine = $reader.ReadUInt16()
} finally {
    $reader.Dispose()
}
if ($machine -ne $expectedMachine) {
    throw ('MSVC cl.exe PE machine mismatch: expected=0x{0:X4} actual=0x{1:X4} path={2}' -f $expectedMachine, $machine, $compilerPath)
}

# Optionally expose the clang-cl bundled with this Visual Studio installation
# (C++ Clang tools component) for steps that declare clang-cl, e.g. the seed
# bootstrap's clang-cl driver. Its absence is reported, not fatal: such a step
# fails on its own. LLVM provisioning never uses it; it uses the pinned
# clang-cl from the host's checksum-verified compiler archive.
$llvmBin = $null
if ($env:SULTANC_MSVC_LLVM_DIR) {
    if ($env:SULTANC_MSVC_LLVM_DIR -notmatch '^[A-Za-z0-9_]+$') {
        throw "Invalid SULTANC_MSVC_LLVM_DIR: $env:SULTANC_MSVC_LLVM_DIR"
    }
    $candidate = Join-Path $installation "VC\Tools\Llvm\$($env:SULTANC_MSVC_LLVM_DIR)\bin"
    if (Test-Path (Join-Path $candidate 'clang-cl.exe')) {
        $llvmBin = $candidate
    } else {
        Write-Warning "Visual Studio clang-cl not found under $candidate"
    }
}

# Publish only new VS directories; preserve GitHub runner PATH and GNU tools.
foreach ($entry in ($env:PATH -split ';')) {
    if ($entry -and $previousPath -notcontains $entry) {
        Add-Content -LiteralPath $env:GITHUB_PATH -Value $entry -Encoding utf8
    }
}
Add-Content -LiteralPath $env:GITHUB_PATH -Value (Split-Path -Parent $compilerPath) -Encoding utf8
if ($llvmBin) {
    Add-Content -LiteralPath $env:GITHUB_PATH -Value $llvmBin -Encoding utf8
}
foreach ($name in @('INCLUDE', 'LIB', 'LIBPATH', 'VSCMD_ARG_TGT_ARCH', 'VSCMD_ARG_HOST_ARCH')) {
    $value = [Environment]::GetEnvironmentVariable($name)
    if (-not $value) { throw "Native MSVC environment is missing: $name" }
    Add-Content -LiteralPath $env:GITHUB_ENV -Value "$name=$value" -Encoding utf8
}
