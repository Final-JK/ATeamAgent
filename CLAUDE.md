# CLAUDE.md — CodeFab Interpreter 프로젝트

이 파일은 Claude Code가 이 프로젝트에서 일관되게 동작하기 위한 지침서입니다.
4명의 팀원이 각자의 환경에서 Claude를 실행할 때 동일한 판단 기준을 갖도록 합니다.

---

## 프로젝트 컨텍스트

- **과목**: Code Review Agent 교육 과정 (CodeFab)
- **과제**: C++17 기반 커스텀 언어 인터프리터 구현
- **테스트 기준**: https://gist.github.com/aijeonghwan-star/d1535e870aeb6a4a928142d4d57c191e
- **팀 인원**: 4명 (공동 개발, 각자 Claude 사용)
- **언어**: C++17 (MSVC 19.51+, `/std:c++17 /utf-8 /EHsc`)

---

## 언어 스펙 (구현된 범위)

### 타입
| 타입 | 예시 |
|------|------|
| 숫자 (double) | `5`, `3.14`, `5.0` → 정수면 소수점 없이 출력 |
| 문자열 | `"Hello"` |
| 불리언 | `true`, `false` |
| nil | 초기화 없는 변수의 기본값 |

### 연산자
- 산술: `+`, `-`, `*`, `/` (왼쪽 결합)
- 비교: `<`, `<=`, `>`, `>=`, `==`, `!=`
- 단항: `-` (숫자만 허용)
- 문자열 연결: `"a" + "b"` (`+` 오버로드)

### 문 (Statement)
```
print expr;
var name = expr;
name = expr;
{ ... }
if (cond) stmt [else stmt]
for (init; cond; incr) stmt
```

---

## 아키텍처 — 처리 파이프라인

```
소스코드
  │
  ▼
[Lexer]         Token 배열 생성, 주석(//) 제거
  │
  ▼
[Parser]        재귀하강, AST 생성 (ExprPtr / StmtPtr)
  │
  ▼
[Resolver]      정적 검사 (스코프 스택 기반)
  │               - 자기 초기화자에서 변수 읽기
  │               - 동일 스코프 내 중복 선언
  ▼
[Interpreter]   트리-워크 실행 (Environment 체인)
  │
  ▼
stdout 출력 / stderr 오류
```

---

## 파일 구조

```
CustomInterpreter/
├── CLAUDE.md                   ← 이 파일 (Claude 행동 지침)
├── CMakeLists.txt
├── build.ps1                   ← PowerShell 빌드 스크립트 (권장)
├── build.bat                   ← CMD 빌드 스크립트
├── src/
│   ├── Token.h                 ← TokenType enum, Token struct, Value(variant) 타입
│   ├── Lexer.h / Lexer.cpp     ← 어휘 분석기
│   ├── Expr.h                  ← Expression AST 노드 (Visitor 패턴)
│   ├── Stmt.h                  ← Statement AST 노드 (Visitor 패턴)
│   ├── Environment.h / .cpp    ← 스코프 체인 (변수 저장소)
│   ├── Parser.h / Parser.cpp   ← 재귀하강 파서
│   ├── Resolver.h / Resolver.cpp ← 정적 검사기
│   ├── Interpreter.h / .cpp    ← 트리-워크 인터프리터
│   └── main.cpp                ← 진입점
├── tests/                      ← .fab 테스트 파일
│   ├── test1_expressions.fab
│   ├── test2_variables.fab
│   ├── test3_controlflow.fab
│   └── err*.fab
├── docs/
│   ├── 협업_가이드.md
│   └── 프롬프트_모음.md
└── out/
    └── interpreter.exe         ← 빌드 결과물
```

---

## 노드 설계 원칙

### Expression Node (`src/Expr.h`)
모든 Expression은 `Expr`를 상속하며 `Value accept(ExprVisitor&)`를 구현한다.

| 노드 | 역할 | 주요 필드 |
|------|------|-----------|
| `LiteralExpr` | 숫자/문자열/bool/nil | `Value value` |
| `UnaryExpr` | 단항 연산 `-` | `Token op, ExprPtr right` |
| `BinaryExpr` | 이항 연산 | `ExprPtr left, Token op, ExprPtr right` |
| `GroupingExpr` | 괄호 그룹 | `ExprPtr expression` |
| `VariableExpr` | 변수 참조 | `Token name` |
| `AssignExpr` | 변수 대입 | `Token name, ExprPtr value` |

### Statement Node (`src/Stmt.h`)
모든 Statement는 `Stmt`를 상속하며 `void accept(StmtVisitor&)`를 구현한다.

| 노드 | 역할 |
|------|------|
| `PrintStmt` | `print expr;` |
| `ExprStmt` | `expr;` |
| `VarStmt` | `var name = expr;` |
| `BlockStmt` | `{ stmts... }` |
| `IfStmt` | `if/else` (dangling-else는 가장 가까운 if에 바인딩) |
| `ForStmt` | `for(init; cond; incr)` |

---

## 값 타입 (Value)

```cpp
// src/Token.h
using Value = std::variant<double, std::string, bool, std::nullptr_t>;
```

런타임 값과 토큰 리터럴 모두 이 타입을 사용한다.

---

## 오류 계층

| 예외 클래스 | 발생 위치 | 출력 형식 |
|------------|-----------|-----------|
| `LexError` | Lexer | `[line N] Syntax Error: 메시지` |
| `ParseError` | Parser | `[line N] Syntax Error at 'token': 메시지` |
| `StaticError` | Resolver | `[line N] Static Error: 메시지` |
| `RuntimeError` | Interpreter | `[line N] Runtime Error: 메시지` |

오류 발생 시 즉시 throw → main에서 catch → stderr 출력 후 종료.

---

## 빌드 방법

```powershell
# 프로젝트 루트에서 실행
powershell -ExecutionPolicy Bypass -File build.ps1
```

결과물: `out\interpreter.exe`

실행:
```
out\interpreter.exe tests\test1_expressions.fab
```

---

## Claude에게 주는 행동 지침

### 코드 작성 시
1. **C++17 표준**을 준수한다. MSVC에서 컴파일되어야 하므로 `/utf-8` 플래그를 가정한다.
2. 소스 파일의 **한글 주석**은 허용되지만, 빌드 시 `/utf-8` 플래그가 필요함을 항상 인지한다.
3. 새 노드 추가 시: `Expr.h` 또는 `Stmt.h`에 노드 정의 → 모든 Visitor 구현체(`Resolver`, `Interpreter`)에 visit 메서드 추가.
4. `ExprPtr` / `StmtPtr`는 `std::unique_ptr<Expr>` / `std::unique_ptr<Stmt>`의 alias다. 반환 시 `std::move`를 명시한다.
5. `Value`는 `std::variant`이므로 접근 시 `std::holds_alternative<T>()` 및 `std::get<T>()` 사용.

### 새 기능 추가 순서
1. Lexer에 토큰 추가 (필요한 경우)
2. Parser에 grammar rule 추가
3. Resolver에 visit 메서드 추가 (정적 검사 로직)
4. Interpreter에 visit 메서드 추가 (실행 로직)
5. `tests/` 에 테스트 파일 추가 후 실행 확인

### 절대 하지 말아야 할 것
- 기존 테스트 파일 변경
- `Value` 타입을 다른 타입으로 교체
- Visitor 패턴 우회 (직접 `dynamic_cast` 사용)
- 에러 계층 구조 변경 없이 예외 추가

---

## 팀원 온보딩 체크리스트

- [ ] `build.ps1` 실행 → `out\interpreter.exe` 생성 확인
- [ ] `tests/test1_expressions.fab` 실행 → 기댓값과 일치 확인
- [ ] `tests/err5_self_init.fab` 실행 → Static Error 출력 확인
- [ ] `docs/협업_가이드.md` 읽기
- [ ] `docs/프롬프트_모음.md`에서 세션 흐름 파악
