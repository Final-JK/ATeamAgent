@echo off
setlocal

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

cd /d "%~dp0"

if not exist out mkdir out

cl.exe /std:c++17 /EHsc /W4 /O2 /Fe:out\interpreter.exe ^
    src\main.cpp ^
    src\Lexer.cpp ^
    src\Parser.cpp ^
    src\Environment.cpp ^
    src\Resolver.cpp ^
    src\Interpreter.cpp ^
    /I src

if %ERRORLEVEL% EQU 0 (
    echo Build succeeded: out\interpreter.exe
) else (
    echo Build failed.
)
