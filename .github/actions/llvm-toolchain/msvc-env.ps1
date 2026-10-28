# Activate the runner-provided native MSVC developer environment for both
# Windows provisioning and qualification. No alternate toolchain is installed.
$ErrorActionPreference = 'Stop'

$architecture = switch ($env:RUNNER_ARCH) {
    'X64' { 'amd64' }
    'ARM64' { 'arm64' }
    default { throw "Unsupported Windows runner architecture: $env:RUNNER_ARCH" }
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw "MSVC discovery tool missing: $vswhere" }
$installation = & $vswhere -latest -products '*' -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installation) { throw 'Native Visual Studio installation not found' }
$module = Join-Path $installation 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll'
if (-not (Test-Path $module)) { throw "MSVC developer shell missing: $module" }

$previousPath = @($env:PATH -split ';')
Import-Module $module
Enter-VsDevShell -VsInstallPath $installation -SkipAutomaticLocation -DevCmdArguments "-arch=$architecture -host_arch=$architecture"
if ($env:VSCMD_ARG_TGT_ARCH -ne $architecture -or $env:VSCMD_ARG_HOST_ARCH -ne $architecture) {
    throw "Native MSVC architecture mismatch: target=$env:VSCMD_ARG_TGT_ARCH host=$env:VSCMD_ARG_HOST_ARCH expected=$architecture"
}
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw 'Native MSVC cl.exe was not enabled by the developer shell'
}

# Propagate only the Visual Studio additions, and never replace the runner PATH.
foreach ($entry in ($env:PATH -split ';')) {
    if ($entry -and $previousPath -notcontains $entry) {
        Add-Content -LiteralPath $env:GITHUB_PATH -Value $entry -Encoding utf8
    }
}
foreach ($name in @('INCLUDE', 'LIB', 'LIBPATH', 'VSCMD_ARG_TGT_ARCH', 'VSCMD_ARG_HOST_ARCH')) {
    $value = [Environment]::GetEnvironmentVariable($name)
    if (-not $value) { throw "Native MSVC environment is missing: $name" }
    Add-Content -LiteralPath $env:GITHUB_ENV -Value "$name=$value" -Encoding utf8
}
