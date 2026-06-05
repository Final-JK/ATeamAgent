# REPL 모드 자동 테스트 스크립트
# 사용법: powershell -ExecutionPolicy Bypass -File tests\run_repl_tests.ps1

$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$exe  = "$root\out\interpreter.exe"
$dir  = "$root\tests\repl"

if (-not (Test-Path $exe)) {
    Write-Host "interpreter.exe not found. Run build.ps1 first." -ForegroundColor Red
    exit 1
}

$pass = 0; $fail = 0

function Test-Repl {
    param($name, $inputFile, [string[]]$mustContain, [string[]]$mustNotContain = @())

    # PowerShell 파이프는 stdout/stdin 인코딩을 변형시키므로
    # Start-Process로 파일을 stdin에 직접 연결한다.
    $tmpOut = [System.IO.Path]::GetTempFileName()
    $tmpErr = [System.IO.Path]::GetTempFileName()
    $p = Start-Process -FilePath $exe `
        -RedirectStandardInput  $inputFile `
        -RedirectStandardOutput $tmpOut `
        -RedirectStandardError  $tmpErr `
        -NoNewWindow -Wait -PassThru
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
        Write-Host "  output: $output" -ForegroundColor Yellow
        $script:fail++
    }
}

Write-Host "=== REPL Tests ===" -ForegroundColor Cyan

# 기본 산술·출력
Test-Repl "basic"          "$dir\repl_basic.txt"         @("3", "hello", "true")

# 변수 세션 유지
Test-Repl "persist"        "$dir\repl_persist.txt"       @("30")

# 오류 후 계속 실행
Test-Repl "error_recovery" "$dir\repl_error_recovery.txt" @("Runtime Error", "5", "6")

# 함수 선언/재귀 호출
Test-Repl "function"       "$dir\repl_function.txt"      @("10", "120")

# 전역 변수 재선언 허용
Test-Repl "redeclare"      "$dir\repl_redeclare.txt"     @("1", "99") @("Static Error")

Write-Host ""
Write-Host "Result: $pass passed, $fail failed" -ForegroundColor $(if ($fail -eq 0) { "Green" } else { "Red" })
exit $fail
