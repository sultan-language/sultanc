# Activate the runner-provided native MSVC developer environment for both
# Windows provisioning and qualification. No alternate toolchain is installed.
$ErrorActionPreference = 'Stop'

function ConvertTo-NativeArchitecture([string]$value) {
    switch ($value.Trim().ToLowerInvariant()) {
        'amd64' { return 'x86_64' }
        'x64'   { return 'x86_64' }
        'arm64' { return 'arm64' }
        default { throw "Unsupported native MSVC architecture: '$value'" }
    }
}

$runnerArchitecture = ConvertTo-NativeArchitecture $env:RUNNER_ARCH
$architecture = if ($runnerArchitecture -eq 'x86_64') { 'amd64' } else { 'arm64' }
$toolArchitecture = if ($runnerArchitecture -eq 'x86_64') { 'x64' } else { 'arm64' }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw "MSVC discovery tool missing: $vswhere" }
$installation = & $vswhere -latest -products '*' -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installation) { throw 'Native Visual Studio installation not found' }
$module = Join-Path $installation 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
if (-not (Test-Path $module)) { throw "MSVC developer shell missing: $module" }

$previousPath = @($env:PATH -split ';')
Import-Module $module
Enter-VsDevShell -VsInstallPath $installation -SkipAutomaticLocation -DevCmdArguments "-arch=$architecture -host_arch=$architecture"
$targetArchitecture = ConvertTo-NativeArchitecture $env:VSCMD_ARG_TGT_ARCH
$hostArchitecture = ConvertTo-NativeArchitecture $env:VSCMD_ARG_HOST_ARCH
if ($targetArchitecture -ne $runnerArchitecture -or $hostArchitecture -ne $runnerArchitecture) {
    throw "Native MSVC architecture mismatch: target=$env:VSCMD_ARG_TGT_ARCH host=$env:VSCMD_ARG_HOST_ARCH runner=$env:RUNNER_ARCH"
}

# Ensure PATH selects the native compiler from this Visual Studio installation,
# not an emulated or cross-target MSVC compiler found elsewhere on the runner.
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

# Confirm the compiler's actual PE architecture, not just its directory name.
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
$expectedMachine = if ($runnerArchitecture -eq 'x86_64') { 0x8664 } else { 0xaa64 }
if ($machine -ne $expectedMachine) {
    throw ('MSVC cl.exe PE machine mismatch: expected=0x{0:X4} actual=0x{1:X4} path={2}' -f $expectedMachine, $machine, $compilerPath)
}

# Propagate only the Visual Studio additions, and never replace the runner PATH.
foreach ($entry in ($env:PATH -split ';')) {
    if ($entry -and $previousPath -notcontains $entry) {
        Add-Content -LiteralPath $env:GITHUB_PATH -Value $entry -Encoding utf8
    }
}
# GitHub prepends each published directory; make the validated native compiler
# the first cl.exe resolved in later provisioning/qualification steps as well.
Add-Content -LiteralPath $env:GITHUB_PATH -Value (Split-Path -Parent $compilerPath) -Encoding utf8
foreach ($name in @('INCLUDE', 'LIB', 'LIBPATH', 'VSCMD_ARG_TGT_ARCH', 'VSCMD_ARG_HOST_ARCH')) {
    $value = [Environment]::GetEnvironmentVariable($name)
    if (-not $value) { throw "Native MSVC environment is missing: $name" }
    Add-Content -LiteralPath $env:GITHUB_ENV -Value "$name=$value" -Encoding utf8
}
