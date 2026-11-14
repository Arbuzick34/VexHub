#requires -Version 5.1
<#
    Builds and runs the rage solver tests with the MSVC toolchain.

    cl.exe is not on PATH on this machine; the script locates vcvars64.bat in
    the Visual Studio Build Tools installation and uses it to set up the
    environment inside cmd.

    Usage:
        .\build.ps1            build and run
        .\build.ps1 -NoRun     build only
        .\build.ps1 -Clean     remove build/ first
#>
param(
    [switch]$NoRun,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$out = Join-Path $root 'build'

$vcvarsCandidates = @(
    'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat'
)

$vcvars = $vcvarsCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $vcvars) {
    $found = Get-ChildItem 'C:\Program Files*\Microsoft Visual Studio' -Recurse -Filter vcvars64.bat -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
    if ($found) { $vcvars = $found }
}
if (-not $vcvars) {
    throw 'vcvars64.bat not found. Install the Visual Studio Build Tools with the C++ workload.'
}

if ($Clean -and (Test-Path $out)) { Remove-Item $out -Recurse -Force }
New-Item -ItemType Directory -Force -Path $out | Out-Null

$sources = Get-ChildItem (Join-Path $root 'src\*.cpp') | ForEach-Object { "`"$($_.FullName)`"" }
$sources += "`"$(Join-Path $root 'tests\test_main.cpp')`""

$includes = "`"$(Join-Path $root 'include')`""
$command = "call `"$vcvars`" >nul && cl /nologo /std:c++20 /EHsc /O2 /W4 /permissive- /utf-8 /MT /I $includes $($sources -join ' ') /Fe:rage_test.exe"

Push-Location $out
try {
    Write-Host "toolchain: $vcvars"
    cmd /c $command
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
finally {
    Pop-Location
}

$exe = Join-Path $out 'rage_test.exe'
if (-not (Test-Path $exe)) { throw "build failed: $exe not produced" }

if (-not $NoRun) {
    & $exe
    exit $LASTEXITCODE
}

Write-Host "built $exe"
