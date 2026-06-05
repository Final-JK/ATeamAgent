# 추가구현 개발문서 — Chapter 5: 공잡 제어 쉘(Shell Mode)

**작성일**: 2026-06-05  
**대상 프로젝트**: CustomInterpreter (CodeFab 교육 과정)  
**구현 범위**: InterpreterFactory + 파일 모드 분리 + REPL 모드 + 디버그 모드

---

## 1. 개요

기존 인터프리터는 `main.cpp` 단일 파일에서 파일을 읽어 일회성으로 실행하는 구조였다.  
Chapter 5에서는 실행 방식을 3가지 모드로 분리하고 **GoF Factory Method 패턴**으로 통합했다.

| 모드 | 진입 방법 | 설명 |
|------|-----------|------|
| 파일 모드 | `interpreter <file>` | 소스 파일 일괄 실행 (기존 동작) |
| REPL 모드 | `interpreter` | 한 줄씩 대화형 실행, 변수 세션 유지 |
| 디버그 모드 | `interpreter --debug <file>` | Stmt 단위 stepping, breakpoint, watch |

파이프라인(`Lexer → Parser → Resolver → Interpreter`) 자체는 변경하지 않았으며,  
각 모드 Runner가 파이프라인을 적절한 방식으로 호출한다.

---

## 2. 커밋 이력

| 커밋 해시 | 제목 | 범위 |
|-----------|------|------|
| `f390d6e` | feat(Ch.5): RunnerBase 추상 기반 클래스 추가 | RunnerBase.h |
| `122e76a` | feat(Ch.5): FileRunner — 파일 모드 분리 | FileRunner.h/.cpp |
| `5ea4a39` | feat(Ch.5): InterpreterFactory + ReplRunner/DebugRunner 스텁 | InterpreterFactory, 스텁 Runner |
| `0c0beda` | feat(Ch.5): main.cpp InterpreterFactory 호출로 단순화 | main.cpp, build.ps1, CMakeLists.txt |
| `97314e6` | feat(Ch.5): ReplRunner — REPL 모드 구현 + 테스트 케이스 추가 | ReplRunner, tests/repl/ |
| `7e25dbb` | feat(Ch.5): DebugRunner — stepping, breakpoint, watch, inspect 구현 | DebugRunner, Interpreter, Environment |
| `c082494` | docs: README.md — 언어 명세, 실행 모드, 테스트 가이드 작성 | README.md |

---

## 3. 아키텍처 — GoF Factory Method 패턴

```
InterpreterFactory::create(argc, argv)
        │
        ├── argc==1            → ReplRunner
        ├── argc==2            → FileRunner(path)
        └── argc==3 --debug    → DebugRunner(path)
              │
        (모두 RunnerBase* 반환)
              │
        main.cpp: runner->run()
```

`main.cpp`는 7줄로 축소되었다. 모드 선택 로직은 `InterpreterFactory`에, 실행 로직은 각 Runner에 위임한다.

---

## 4. 구현 내용

### 4-1. RunnerBase

모든 Runner의 공통 인터페이스.

```cpp
class RunnerBase {
public:
    virtual int run() = 0;   // 종료 코드 반환
    virtual ~RunnerBase() = default;
};
```

### 4-2. FileRunner

기존 `main.cpp`의 파일 실행 로직을 이동했다. 기능·오류 처리는 변경 없음.

- 파일 없을 시 → `exit 66`
- 문법/정적 오류 → `exit 65`
- 런타임 오류 → `exit 70` (줄 번호 포함)

### 4-3. ReplRunner — 주요 설계 결정

**변수 세션 유지**: `Interpreter` 인스턴스를 루프 밖에서 생성해 재사용한다.  
→ `globalEnv`가 입력 라인 간에 유지된다.

**댕글링 포인터 방지**: `FabFunction::body`는 `FuncStmt::body`에 대한 비소유 포인터이므로  
AST(`stmts`)가 소멸하면 포인터가 무효화된다.  
→ `sessionStmts` 컨테이너에 모든 AST를 세션 종료 전까지 보관한다.

```cpp
std::vector<std::vector<StmtPtr>> sessionStmts;
// ...
interpreter.interpret(stmts);
sessionStmts.push_back(std::move(stmts));  // AST 수명 연장
```

**오류 복구**: 모든 오류를 catch 후 출력만 하고 루프를 계속 진행한다.  
→ REPL은 오류로 종료되지 않는다.

**전역 재선언 허용**: Resolver의 `declare()`는 `scopes.empty()`(전역 스코프)일 때  
중복 검사를 건너뛰므로 REPL에서 `var x`를 재선언해도 StaticError가 발생하지 않는다.  
→ 별도 플래그 추가 불필요.

### 4-4. DebugRunner — 주요 설계 결정

**stmtHook 콜백**: `Interpreter::execute()`에 `std::function<void(const Stmt&)> stmtHook`을 추가했다.  
디버그 모드에서만 훅이 설정되며, 일반 실행(파일/REPL)에서는 `nullptr`이므로 오버헤드 없음.

```cpp
void Interpreter::execute(Stmt& stmt) {
    if (stmtHook) stmtHook(stmt);   // 디버그 모드에서만 호출
    stmt.accept(*this);
}
```

**상태 머신**:

```
[STEPPING] ─step/next─▶ 다음 Stmt에서 정지 (STEPPING 유지)
[STEPPING] ─continue──▶ [RUNNING]
[RUNNING]  ─breakpoint hit─▶ [STEPPING]
```

**watch 자동 출력**: 매 정지 시점마다 감시 변수 목록을 순회하며 현재 값을 출력한다.

**inspect**: `Environment::getValues()`로 현재 스코프의 변수 맵에 직접 접근한다.

---

## 5. 산출물 (신규/변경 파일)

| 파일 | 상태 | 주요 내용 |
|------|------|-----------|
| `src/main.cpp` | 수정 | InterpreterFactory 호출로 7줄로 축소 |
| `src/RunnerBase.h` | **신규** | Runner 추상 기반 클래스 |
| `src/InterpreterFactory.h/.cpp` | **신규** | Factory 패턴, argc/argv 기반 모드 감지 |
| `src/FileRunner.h/.cpp` | **신규** | 파일 모드 (기존 main.cpp 로직 이동) |
| `src/ReplRunner.h/.cpp` | **신규** | REPL 루프, AST 수명 관리, 오류 복구 |
| `src/DebugRunner.h/.cpp` | **신규** | Stmt stepping, breakpoint, watch/inspect |
| `src/Interpreter.h/.cpp` | 수정 | `stmtHook` 콜백, `getCurrentEnv()` 추가 |
| `src/Environment.h` | 수정 | `getValues()` 추가 |
| `build.ps1` | 수정 | 신규 .cpp 파일 5개 추가 |
| `CMakeLists.txt` | 수정 | 신규 .cpp 파일 5개 추가 |
| `README.md` | **신규** | 언어 명세 + 실행 모드 + 테스트 가이드 |
| `tests/repl/repl_basic.txt` | **신규** | REPL 기본 산술·출력 테스트 입력 |
| `tests/repl/repl_persist.txt` | **신규** | REPL 변수 세션 유지 테스트 입력 |
| `tests/repl/repl_error_recovery.txt` | **신규** | REPL 오류 후 계속 테스트 입력 |
| `tests/repl/repl_function.txt` | **신규** | REPL 함수·재귀 테스트 입력 |
| `tests/repl/repl_redeclare.txt` | **신규** | REPL 전역 재선언 테스트 입력 |
| `tests/debug/debug_step.txt` | **신규** | 디버그 step 명령 테스트 입력 |
| `tests/debug/debug_breakpoint.txt` | **신규** | 디버그 breakpoint+continue 테스트 입력 |
| `tests/debug/debug_watch.txt` | **신규** | 디버그 watch+inspect 테스트 입력 |
| `tests/test6_debug_target.fab` | **신규** | 디버그 모드 테스트용 소스 파일 |
| `tests/run_repl_tests.ps1` | **신규** | REPL 자동화 테스트 스크립트 |
| `tests/run_debug_tests.ps1` | **신규** | 디버그 자동화 테스트 스크립트 |

---

## 6. 테스트 방법

### 6-1. 빌드

```powershell
powershell -ExecutionPolicy Bypass -File build.ps1
```

### 6-2. 파일 모드 회귀 테스트

```powershell
# 기존 test1~test5 모두 통과 확인
foreach ($f in Get-ChildItem tests\test*.fab) {
    Write-Host "=== $($f.Name) ==="
    out\interpreter.exe $f.FullName
}
```

### 6-3. REPL 모드 자동 테스트

```powershell
powershell -ExecutionPolicy Bypass -File tests\run_repl_tests.ps1
```

| 테스트 | 입력 파일 | 검증 항목 |
|--------|-----------|-----------|
| basic | `repl_basic.txt` | `3`, `hello`, `true` 출력 |
| persist | `repl_persist.txt` | 변수 합산(`30`) 세션 유지 |
| error_recovery | `repl_error_recovery.txt` | 오류 출력 후 이후 줄 계속 실행 |
| function | `repl_function.txt` | `add(3,7)=10`, `fact(5)=120` |
| redeclare | `repl_redeclare.txt` | `var x` 재선언 후 새 값(`99`) 적용 |

### 6-4. 디버그 모드 자동 테스트

```powershell
powershell -ExecutionPolicy Bypass -File tests\run_debug_tests.ps1
```

| 테스트 | 입력 파일 | 검증 항목 |
|--------|-----------|-----------|
| step | `debug_step.txt` | `Stopped at`, `Program finished`, 출력값 |
| breakpoint | `debug_breakpoint.txt` | `Breakpoint set`, `Breakpoint hit`, 출력값 |
| watch | `debug_watch.txt` | `Watching 'x'`, `watch x = 10`, `y = 20` |

### 6-5. 수동 테스트 예시

```powershell
# REPL 직접 실행
out\interpreter.exe

# 디버그 직접 실행
out\interpreter.exe --debug tests\test6_debug_target.fab
```

---

## 7. 회귀 테스트 결과

| 파일 | 결과 |
|------|------|
| `tests/test1_expressions.fab` | ✅ 통과 |
| `tests/test2_variables.fab` | ✅ 통과 |
| `tests/test3_controlflow.fab` | ✅ 통과 |
| `tests/test4_functions.fab` | ✅ 통과 |
| `tests/test5_arrays.fab` | ✅ 통과 |
| REPL 자동 테스트 5종 | ✅ 통과 |
| 디버그 자동 테스트 3종 | ✅ 통과 |

---

## 작성 경위 (프롬프트 이력)

> **[2026-06-05]** "자 이제 다음 단계야. chapter5 에 shell mode 도 지원하라는 추가 기능이 있어. 우선 필요한 것이 무엇인지 파악부터 해줄래?"

> **[2026-06-05]** "우선 지금 분석한 내용을 개발문서로 작성 먼저 해주고, 단계별로 진행하면서 각 commit 을 진행해줘. 중요한건 어떤 작업을 한건지 코드 상 주석을 남기는 것도 중요해."

> **[2026-06-05]** "step2 에서도 각 구현별로 commit 을 나누어서 진행해줘!"

> **[2026-06-05]** "test case 도 보강할 수 있다면 보강하면서 진행해줘. 각 단계별로 추가 되는 것도 좋을 것 같아."

> **[2026-06-05]** "지금 작업했던 것을 어떤 것을 했는지, 산출물은 어떤 것인지, 어떻게 테스트 하는지, commit 은 무엇인지에 대해 개발 문서 작성해줘."
