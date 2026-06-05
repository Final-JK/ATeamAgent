# 추가 요구사항 분석 — Chapter 5: 공잡 제어 쉘(Shell Mode)

**작성일**: 2026-06-05  
**대상 프로젝트**: CustomInterpreter (CodeFab 교육 과정)  
**분석 범위**: InterpreterFactory + REPL 모드 + 파일 모드 정비 + 디버그 모드

---

## 1. 개요

기존 인터프리터는 `main.cpp`에서 파일을 읽어 일회성으로 실행하는 구조였다.  
Chapter 5에서는 실행 방식을 3가지 모드로 분리하고, **InterpreterFactory**가 모드를 생성·관리하도록 설계를 확장한다.

```
인수 없음              → 프롬프트 모드 (REPL)
파일명.fab             → 파일 모드 (기존 동작 정비)
--debug 파일명.fab     → 디버그 모드 (Stmt 단위 stepping)
```

처리 파이프라인 자체(`Lexer → Parser → Resolver → Interpreter`)는 변경하지 않는다.  
각 모드 Runner가 파이프라인을 적절한 방식으로 호출할 뿐이다.

---

## 2. 요구 모드 전체

| 모드 | 진입 방법 | 현재 상태 |
|------|-----------|-----------|
| **프롬프트 모드 (REPL)** | 인수 없이 실행 | ❌ 미구현 |
| **파일 모드** | `파일명.fab` 전달 | ✅ 대부분 구현 (리팩터링 필요) |
| **디버그 모드** | `--debug 파일명.fab` | ❌ 미구현 |

---

## 3. 아키텍처 설계

### 3-1. InterpreterFactory (GoF Factory Method 패턴)

```
InterpreterFactory
  ├── createFileRunner(path)   → FileRunner
  ├── createReplRunner()       → ReplRunner
  └── createDebugRunner(path)  → DebugRunner
```

> GoF Factory Method 패턴 적용 → **추가 점수 대상** (PDF 명시)

```cpp
// 최종 main.cpp 형태
int main(int argc, char* argv[]) {
    auto runner = InterpreterFactory::create(argc, argv);
    return runner->run();
}
```

모드 감지 규칙:
- `argc == 1` → REPL 모드
- `argc == 2` → 파일 모드
- `argc == 3 && argv[1] == "--debug"` → 디버그 모드

---

## 4. 프롬프트 모드 (REPL) 요구사항

### 4-1. 동작 예시

```
$ interpreter
> var x = 10;
> print x;
10
> var x = x + 1;
> print x;
11
> exit
```

### 4-2. 기능 요구사항

| 항목 | 요구사항 |
|------|----------|
| 프롬프트 | `> ` 출력 후 입력 대기 |
| 변수 유지 | `Interpreter` 인스턴스를 루프 전체에서 재사용 → Environment 유지 |
| 오류 처리 | 모든 오류(LexError/ParseError/StaticError/RuntimeError) catch → 출력 후 루프 **계속** |
| 종료 | `exit` 또는 `quit` 입력, 또는 EOF(Ctrl+Z/Ctrl+D) |
| 빈 줄 | 건너뜀 |

### 4-3. 기술 포인트: Resolver 중복 선언 처리

REPL에서 `var x = 1;`을 두 번 입력하면 Resolver가 동일 스코프 중복 선언으로 StaticError를 낸다.  
→ Resolver에 `replMode` 플래그를 추가하여, **전역 스코프(depth 0)에서의 재선언은 허용**하도록 처리.

---

## 5. 파일 모드 요구사항

기존 `main.cpp`의 파일 실행 로직을 `FileRunner`로 이동한다.  
기능 변경 없음 — 코드 위치 이동 및 구조 정비.

| 요구사항 | 현재 상태 |
|----------|-----------|
| 파일 없을 경우 명확한 오류 메시지 | ✅ 구현됨 |
| 런타임 오류 시 줄 번호 포함 출력 | ✅ 구현됨 |
| 오류 발생 시 즉시 종료 | ✅ 구현됨 |

---

## 6. 디버그 모드 요구사항

### 6-1. 진입 방법

```
$ interpreter --debug tests/test4_functions.fab
```

### 6-2. Interpreter 훅 설계

디버그 모드는 각 `Stmt` 실행 **직전**에 제어를 가로채야 한다.  
`Interpreter::execute(Stmt&)` 내부에 **콜백 훅**을 추가한다.

```cpp
// Interpreter.h 추가 예정
// 디버그 모드에서 Stmt 실행 전 호출되는 콜백 (nullptr = 일반 실행)
std::function<void(const Stmt&)> stmtHook;
```

`DebugRunner`가 이 훅을 설정해 stepping / breakpoint 로직을 삽입한다.

### 6-3. 명령어 목록

| 명령 | 약어 | 동작 |
|------|------|------|
| `step` | `s` | 현재 Stmt 실행 후 다음 Stmt에서 정지 |
| `next` | `n` | 현재 Stmt 실행 (블록 내부 진입 X) |
| `break <줄번호>` | `b <n>` | 브레이크포인트 설정 |
| `breakpoints` | `bp` | 현재 브레이크포인트 목록 출력 |
| `remove <줄번호>` | `r <n>` | 브레이크포인트 해제 |
| `continue` | `c` | 다음 브레이크포인트까지 실행 |
| `watch <변수명>` | `w <var>` | 감시 목록 추가 |
| `unwatch <변수명>` | `uw <var>` | 감시 목록에서 제거 |
| `watches` | `ws` | 감시 변수 목록 + 현재 값 출력 |
| `inspect` | `i` | 현재 스코프 전체 변수 출력 |

### 6-4. Stepping 상태 머신

```
[STEPPING] ──step/next──▶ 다음 Stmt에서 정지 (STEPPING 유지)
[STEPPING] ──continue──▶  [RUNNING]
[RUNNING]  ──breakpoint hit──▶ [STEPPING]
```

### 6-5. watch / inspect 동작

- `watch <var>` 등록 후, 매 정지 시점마다 해당 변수의 현재 값 자동 출력
- `inspect`: `Environment` 내 모든 변수를 출력
  → `Environment`에 `dump()` 메서드 추가 필요

---

## 7. 신규/변경 파일 요약

| 파일 | 상태 | 주요 내용 |
|------|------|-----------|
| `src/main.cpp` | 수정 | InterpreterFactory 호출로 단순화 |
| `src/RunnerBase.h` | **신규** | Runner 추상 기반 클래스 (`virtual run()`) |
| `src/InterpreterFactory.h/.cpp` | **신규** | Factory 패턴, 모드 감지 및 Runner 생성 |
| `src/FileRunner.h/.cpp` | **신규** | 파일 모드 (기존 main.cpp 로직 이동) |
| `src/ReplRunner.h/.cpp` | **신규** | REPL 루프, 오류 복구, 지속 Environment |
| `src/DebugRunner.h/.cpp` | **신규** | Stmt stepping, breakpoint, watch/inspect |
| `src/Interpreter.h/.cpp` | 수정 | stmtHook 콜백 추가 |
| `src/Resolver.h/.cpp` | 수정 | replMode 플래그 추가 |
| `src/Environment.h/.cpp` | 수정 | dump() 메서드 추가 |
| `README.md` | **신규** | 언어 사용법 + 실행 모드 가이드 |

---

## 8. 구현 커밋 계획

| 단계 | 커밋 제목 | 포함 내용 |
|------|-----------|-----------|
| 1 | `docs: Chapter 5 요구사항 분석 문서 작성` | 이 파일 |
| 2 | `feat(Ch.5): InterpreterFactory + RunnerBase + 모드 분기` | main.cpp 개편, 추상 클래스 |
| 3 | `feat(Ch.5): FileRunner — 파일 모드 분리` | FileRunner |
| 4 | `feat(Ch.5): ReplRunner — REPL 모드 구현` | ReplRunner, Resolver replMode |
| 5 | `feat(Ch.5): DebugRunner — stepping + breakpoint` | DebugRunner, Interpreter stmtHook |
| 6 | `feat(Ch.5): DebugRunner — watch / inspect` | DebugRunner 확장, Environment dump() |
| 7 | `docs: README.md 언어 사용법 및 실행 모드 가이드` | README.md |

---

## 9. 테스트 계획

| 테스트 항목 | 방법 | 기대 동작 |
|-------------|------|-----------|
| REPL 기본 실행 | 수동 | `> print 1+2;` → `3` 출력 |
| REPL 변수 유지 | 수동 | `var x=5;` 후 `print x;` → `5` |
| REPL 오류 후 계속 | 수동 | 오류 출력 후 `> ` 재표시 |
| REPL 재선언 | 수동 | `var x=1;` 두 번 → 허용 |
| REPL exit | 수동 | `exit` → 정상 종료 |
| 파일 모드 회귀 | 기존 test1~test5 | 기존과 동일 통과 |
| 디버그 step | 수동 | `step` → 한 Stmt씩 실행 |
| 디버그 break | 수동 | 지정 줄 번호에서 정지 |
| 디버그 watch | 수동 | 변수 값 정지마다 자동 출력 |
| 디버그 inspect | 수동 | 현재 스코프 변수 전체 출력 |

---

## 10. 유의사항 (PDF 명시)

1. TDD 방식 강제 아님 (3일차부터)
2. **기존 UnitTest는 유지**되어야 함
3. 추가 기능에 대한 UnitTest 생성 필요
4. `README.md`에 언어 사용 방법 및 특이사항 명시 필수
5. **GoF 디자인 패턴 사용 시 추가 점수** → Factory Method 패턴 적용

---

## 작성 경위 (프롬프트 이력)

> **[2026-06-05]** "자 이제 다음 단계야. chapter5 에 shell mode 도 지원하라는 추가 기능이 있어. 우선 필요한 것이 무엇인지 파악부터 해줄래?"

> **[2026-06-05]** "우선 지금 분석한 내용을 개발문서로 작성 먼저 해주고, 단계별로 진행하면서 각 commit 을 진행해줘. 중요한건 어떤 작업을 한건지 코드 상 주석을 남기는 것도 중요해."

> **[2026-06-05]** "이름을 조금바꾸자, 지금은 요구사항 분석이었으니까 추가요구사항분석_chapter_5 라는 이름으로 작성해줘"

> **[2026-06-05]** "각 문서를 업데이트 할 때, 내가 어떤 prompt 를 사용한 것인지도 같이 남겨줘. 이 지시사항은 앞으로 개발 문서를 남길 때 계속해서 진행할 수 있도록 해줘."
