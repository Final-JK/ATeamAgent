# PowerShell build script for CustomInterpreter
$ErrorActionPreference = "Stop"

$MSVC_VER = "14.51.36231"
$VS_BASE   = "C:\Program Files\Microsoft Visual Studio\18\Community"
$MSVC_BASE = "$VS_BASE\VC\Tools\MSVC\$MSVC_VER"
$SDK_VER   = "10.0.26100.0"
$SDK_BASE  = "C:\Program Files (x86)\Windows Kits\10"

$env:INCLUDE = @(
    "$MSVC_BASE\include",
    "$SDK_BASE\Include\$SDK_VER\ucrt",
    "$SDK_BASE\Include\$SDK_VER\um",
    "$SDK_BASE\Include\$SDK_VER\shared"
) -join ";"

$env:LIB = @(
    "$MSVC_BASE\lib\x64",
    "$SDK_BASE\Lib\$SDK_VER\ucrt\x64",
    "$SDK_BASE\Lib\$SDK_VER\um\x64"
) -join ";"

$env:PATH = "$MSVC_BASE\bin\Hostx64\x64;" + $env:PATH

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $root

if (-not (Test-Path "out")) { New-Item -ItemType Directory "out" | Out-Null }

$sources = @(
    "src\main.cpp",
    "src\Lexer.cpp",
    "src\Parser.cpp",
    "src\Environment.cpp",
    "src\Resolver.cpp",
    "src\Interpreter.cpp",
    # Ch.5: 실행 모드 Runner 및 Factory
    "src\InterpreterFactory.cpp",
    "src\FileRunner.cpp",
    "src\ReplRunner.cpp",
    "src\DebugRunner.cpp"
)

Write-Host "Compiling..." -ForegroundColor Cyan

& cl.exe /std:c++17 /EHsc /utf-8 /W4 /O2 `
    /Fe:out\interpreter.exe `
    $sources `
    /I src

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nBuild succeeded: out\interpreter.exe" -ForegroundColor Green
} else {
    Write-Host "`nBuild failed." -ForegroundColor Red
    exit 1
}

Pop-Location
