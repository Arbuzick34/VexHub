param(
    [string]$Output = (Join-Path $PSScriptRoot '..\VexHub Web Radar.lua')
)

$ErrorActionPreference = 'Stop'

function ConvertTo-LuaLongString([string]$Value) {
    for ($level = 4; $level -lt 16; $level++) {
        $equals = '=' * $level
        $terminator = "]$equals]"
        if (-not $Value.Contains($terminator)) {
            return "[$equals[$Value]$equals]"
        }
    }
    throw 'Unable to encode embedded asset as a Lua long string.'
}

$helper = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'helper.ps1'))
$html = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'public\index.html'))
$main = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'main.lua'))
$content = @(
	'-- VexHub Web Radar single-file distribution. Generated; edit vexhub_web_radar sources.'
	'-- Copy only this file into %TEMP%\vexhub\lua\scripts, refresh, then enable it.'
    ('__VEXHUB_WEB_RADAR_HELPER = ' + (ConvertTo-LuaLongString $helper))
    ('__VEXHUB_WEB_RADAR_HTML = ' + (ConvertTo-LuaLongString $html))
    $main
) -join "`r`n`r`n"

$target = [IO.Path]::GetFullPath($Output)
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
[IO.File]::WriteAllText($target, $content, [Text.UTF8Encoding]::new($false))
Write-Output $target
