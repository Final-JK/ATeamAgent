# 디버그 모드 자동 테스트 스크립트
# 사용법: powershell -ExecutionPolicy Bypass -File tests\run_debug_tests.ps1

$root   = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$exe    = "$root\out\interpreter.exe"
$dir    = "$root\tests\debug"
$target = "$root\tests\test6_debug_target.fab"

if (-not (Test-Path $exe)) {
    Write-Host "interpreter.exe not found. Run build.ps1 first." -ForegroundColor Red
    exit 1
}

$pass = 0; $fail = 0

function Test-Debug {
    param($name, $inputFile, [string[]]$mustContain, [string[]]$mustNotContain = @())

    $tmpOut = [System.IO.Path]::GetTempFileName()
    $tmpErr = [System.IO.Path]::GetTempFileName()
    Start-Process -FilePath $exe `
        -ArgumentList "--debug", $target `
        -RedirectStandardInput  $inputFile `
        -RedirectStandardOutput $tmpOut `
        -RedirectStandardError  $tmpErr `
        -NoNewWindow -Wait | Out-Null
    $output = (Get-Content $tmpOut -Encoding UTF8 -Raw) + (Get-Content $tmpErr -Encoding UTF8 -Raw)
    Remove-Item $tmpOut, $tmpErr -ErrorAction SilentlyContinue

    $ok = $true
    foreach ($expected in $mustContain) {
        if ($output -notmatch [regex]::Escape($expected)) {
            Write-Host "  FAIL [$name] expected: '$expected'" -ForegroundColor Red
            $ok = $false
        }
    }
    foreach ($unexpected in $mustNotContain) {
        if ($output -match [regex]::Escape($unexpected)) {
            Write-Host "  FAIL [$name] unexpected: '$unexpected'" -ForegroundColor Red
            $ok = $false
        }
    }

    if ($ok) {
        Write-Host "  PASS [$name]" -ForegroundColor Green
        $script:pass++
    } else {
        Write-Host "  output:`n$output" -ForegroundColor Yellow
        $script:fail++
    }
}

Write-Host "=== Debug Mode Tests ===" -ForegroundColor Cyan

# step 명령으로 모든 Stmt를 단계별로 실행
Test-Debug "step"        "$dir\debug_step.txt"       @("Stopped at", "Program finished", "30", "20")

# breakpoint 설정 후 continue로 도달
Test-Debug "breakpoint"  "$dir\debug_breakpoint.txt" @("Breakpoint set at line 3", "Breakpoint hit at line 3", "30")

# watch 추가 후 자동 출력 및 inspect
Test-Debug "watch"       "$dir\debug_watch.txt"      @("Watching 'x'", "watch x = 10", "y = 20")

Write-Host ""
Write-Host "Result: $pass passed, $fail failed" -ForegroundColor $(if ($fail -eq 0) { "Green" } else { "Red" })
exit $fail
