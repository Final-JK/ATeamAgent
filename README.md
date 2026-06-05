# CustomInterpreter — Fab Language

CodeFab 교육 과정용 C++17 커스텀 언어 인터프리터.

---

## 빌드

```powershell
powershell -ExecutionPolicy Bypass -File build.ps1
# 결과: out\interpreter.exe
```

**요구 환경**: Visual Studio Build Tools 2022 이상 (MSVC 19.x), Windows 10/11

---

## 실행 모드

### 파일 모드

소스 파일을 읽어 실행합니다.

```
out\interpreter.exe <파일명.fab>
```

### REPL 모드 (대화형)

인수 없이 실행하면 한 줄씩 입력하는 대화형 모드로 진입합니다.

```
out\interpreter.exe
```

```
Fab REPL  (type 'exit' or 'quit' to quit)
> var x = 10;
> print x * 2;
20
> exit
```

- 변수·함수는 `exit`/`quit` 전까지 유지됩니다.
- 오류가 발생해도 종료되지 않고 다음 입력을 받습니다.
- EOF(Ctrl+Z on Windows, Ctrl+D on Unix)로도 종료 가능합니다.

### 디버그 모드

Stmt 단위로 멈추며 실행 상태를 점검하는 모드입니다.

```
out\interpreter.exe --debug <파일명.fab>
```

```
[debug] Stopped at line 2 (VarStmt)
(debug) step
[debug] Stopped at line 3 (VarStmt)
(debug) watch x
Watching 'x'
(debug) continue
```

**디버그 명령어:**

| 명령 | 약어 | 설명 |
|------|------|------|
| `step` | `s` | 현재 Stmt 실행 후 다음에서 정지 |
| `next` | `n` | step과 동일 |
| `continue` | `c` | 다음 브레이크포인트까지 실행 |
| `break <줄>` | `b <줄>` | 브레이크포인트 설정 |
| `breakpoints` | `bp` | 브레이크포인트 목록 출력 |
| `remove <줄>` | `r <줄>` | 브레이크포인트 해제 |
| `watch <변수>` | `w <변수>` | 변수 감시 목록 추가 |
| `unwatch <변수>` | `uw <변수>` | 감시 목록에서 제거 |
| `watches` | `ws` | 감시 변수 목록·값 출력 |
| `inspect` | `i` | 현재 스코프 전체 변수 출력 |
| `help` | `h` | 명령어 목록 출력 |

---

## 언어 명세

### 타입

| 타입 | 예시 | 비고 |
|------|------|------|
| 숫자 | `5`, `3.14` | 정수면 소수점 없이 출력 |
| 문자열 | `"Hello"` | `+`로 연결 가능 |
| 불리언 | `true`, `false` | |
| nil | — | 초기화 없는 변수의 기본값 |

### 연산자

```
+  -  *  /       산술 (숫자), 문자열 연결 (+)
<  <=  >  >=     비교
==  !=           동등 비교
-expr            단항 마이너스
```

### 변수

```fab
var x = 10;
var name = "Fab";
var uninit;        // nil
x = x + 1;
```

### 출력

```fab
print x;
print "Hello, " + name + "!";
```

### 조건문

```fab
if (x > 5) {
    print "big";
} else {
    print "small";
}
```

### 반복문

```fab
for (var i = 0; i < 3; i = i + 1) {
    print i;
}
```

### 함수

```fab
Func add(a, b) {
    return a + b;
}
print add(3, 7);   // 10

// 재귀 호출
Func fact(n) {
    if (n <= 1) return 1;
    return n * fact(n - 1);
}
print fact(5);     // 120

// return 없는 함수는 nil 반환
Func noReturn() { }
var r = noReturn();   // r == nil
```

### 배열

```fab
var arr = Array(3);   // [nil, nil, nil]
arr[0] = 10;
arr[1] = 20;
arr[2] = 30;
print arr[1];         // 20

// 표현식 인덱스
var i = 2;
print arr[i - 1];     // 20
```

---

## 오류 코드

| 종료 코드 | 의미 |
|-----------|------|
| `0` | 정상 종료 |
| `64` | 잘못된 인수 |
| `65` | 문법·정적 오류 (Lex/Parse/Static Error) |
| `66` | 파일을 열 수 없음 |
| `70` | 런타임 오류 |

---

## 테스트 실행

```powershell
# 파일 모드 회귀 테스트 (test1~test5)
foreach ($f in Get-ChildItem tests\test*.fab) {
    Write-Host "=== $($f.Name) ==="
    out\interpreter.exe $f.FullName
}

# REPL 모드 자동 테스트
powershell -ExecutionPolicy Bypass -File tests\run_repl_tests.ps1

# 디버그 모드 자동 테스트
powershell -ExecutionPolicy Bypass -File tests\run_debug_tests.ps1
```

---

## 특이사항

- `1 / 0` 은 런타임 오류가 아닌 `inf`를 반환합니다 (C++ double 부동소수점 표준).
- 함수 선언 후 같은 이름으로 재선언하면 덮어씁니다.
- 배열 크기는 생성 시 고정되며, 음수 크기는 런타임 오류입니다.
- REPL에서 `var x = 1;`을 두 번 입력하면 두 번째 선언이 적용됩니다 (전역 재선언 허용).
