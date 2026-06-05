# 추가요구사항분석

**작성일**: 2026-06-05  
**대상 PC**: Developer-JK (CustomInterpreter 프로젝트)  
**목적**: 3일차 PDF(Chapter 2 함수, Chapter 3 정적 배열) 요구사항을 현재 코드베이스와 비교하여 변경 범위 및 구현 전략 정리

---

## 1. 요구사항 개요

### 1-1. Chapter 2 — function 관련 요구사항

| 기능 | 예시 |
|------|------|
| 함수 선언 | `Func add(a, b) { return a + b; }` |
| 함수 호출, 매개변수 전달 | `add(1, 2);` |
| return 처리 (값 없음) | `return;` → nil 반환 |
| return 처리 (값 있음) | `ret = add(1, 2);` → ret에 반환값 저장 |
| 재귀 호출 | `Func fact(n) { if(n <= 1) return 1; return n * fact(n-1); }` |

**오류 검사 (정적/런타임):**

| 오류 상황 | 예시 | 분류 |
|---|---|---|
| 함수 외부에서 return 사용 | `return 5;` (최상위 레벨) | Static Error |
| 파라미터 이름 중복 | `Func foo(a, a) { }` | Static Error |
| 함수가 아닌 대상 호출 | `var x = "hello"; x();` | Runtime Error |
| 인자 개수 불일치 | `Func foo(a,b,c){}` 를 `foo(1,2)` 로 호출 | Runtime Error |

---

### 1-2. Chapter 3 — 정적 배열 구현

| 기능 | 예시 |
|------|------|
| 배열 생성 (고정 크기) | `var arr = Array(3);` → `[null, null, null]` |
| 인덱스 읽기 | `print arr[0];` |
| 인덱스 쓰기 | `arr[1] = 20;` |
| 표현식 인덱스 | `arr[i - 1] = 7;` |

**런타임 오류 검사:**

| 오류 상황 | 예시 |
|---|---|
| 범위를 벗어난 인덱스 | `print arr[5];` (크기 3짜리 배열) |
| 인덱스가 숫자가 아님 | `print arr["hello"];` |
| 배열이 아닌 대상에 `[]` 사용 | `var x = 10; print x[0];` |
| 배열 생성 시 크기가 숫자가 아님 | `var brr = Array("hi");` |

---

## 2. 현재 코드베이스 현황

처리 파이프라인: `Lexer → Parser → Resolver → Interpreter`

### 현재 구현된 범위

```
Token.h      : 36개 토큰, Value = variant<double, string, bool, nullptr_t>
Lexer        : 숫자/문자열/식별자/키워드/주석(//) 처리
Expr.h       : LiteralExpr, UnaryExpr, BinaryExpr, GroupingExpr, VariableExpr, AssignExpr (6종)
Stmt.h       : PrintStmt, ExprStmt, VarStmt, BlockStmt, IfStmt, ForStmt (6종)
Parser       : var 선언, if/else, for, 블록, 기본 표현식
Resolver     : 자기 초기화 감지, 동일 스코프 중복 선언 감지
Interpreter  : 산술/비교/논리 연산, 변수 스코프 체인
```

### 현재 코드에 없는 것

- `Func`, `return`, `,`, `[`, `]`, `Array` 토큰/키워드
- 함수 값 타입 (Value variant에 미포함)
- 배열 값 타입 (Value variant에 미포함)
- 함수 선언·호출·반환 관련 AST 노드
- 배열 생성·읽기·쓰기 관련 AST 노드

---

## 3. 파일별 변경 목록

### 3-1. 변경 영향 범위 요약

| 파일 | Chapter 2 함수 | Chapter 3 배열 | 비고 |
|------|:-:|:-:|------|
| `Token.h` | ✅ | ✅ | Value 타입도 확장 |
| `Lexer.cpp` | ✅ | ✅ | 키워드/문자 추가 |
| `Expr.h` | ✅ | ✅ | 노드 및 Visitor 확장 |
| `Stmt.h` | ✅ | — | FuncStmt, ReturnStmt |
| `Parser.cpp` | ✅ | ✅ | 문법 규칙 추가 |
| `Resolver.cpp` | ✅ | ✅ | visit 메서드 추가 |
| `Interpreter.cpp` | ✅ | ✅ | visit 메서드 추가 |
| `FabFunction.h` | 신규 | — | 함수/배열 런타임 타입 |

---

### 3-2. `Token.h` 변경 내용

**추가 TokenType:**
```cpp
// Chapter 2
FUNC, RETURN, COMMA,

// Chapter 3
LEFT_BRACKET, RIGHT_BRACKET,
ARRAY_KW,
```

**Value variant 확장 (교체가 아닌 확장 — 기존 타입 유지):**
```cpp
// 현재
using Value = std::variant<double, std::string, bool, std::nullptr_t>;

// 변경 후
struct FabFunction;
struct FabArray;
using Value = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<FabFunction>,
    std::shared_ptr<FabArray>
>;
```

> ⚠️ Value 확장 이후 `stringify()`, `isEqual()`, `isTruthy()` 등 Value를 switch/visit하는
> **모든 코드에 새 타입 분기가 추가**되어야 합니다.

---

### 3-3. 신규 파일 `FabFunction.h`

함수와 배열의 런타임 표현 타입을 정의합니다.

```cpp
// 함수 런타임 객체
struct FabFunction {
    std::string              name;
    std::vector<Token>       params;
    std::vector<StmtPtr>*    body;          // Parser가 소유; 포인터로 참조
    std::shared_ptr<Environment> closure;   // 선언 시점 환경 캡처
};

// 배열 런타임 객체
struct FabArray {
    std::vector<Value> elements;
    explicit FabArray(int size) : elements(size, nullptr) {}
};

// return 제어 흐름: C++ 예외로 구현
struct ReturnSignal {
    Value value;
};
```

---

### 3-4. `Expr.h` 변경 내용

**추가 노드 3종:**

```cpp
// 함수 호출: callee(arg1, arg2, ...)
struct CallExpr : Expr {
    ExprPtr              callee;
    Token                paren;    // 오류 위치 추적용 '(' 토큰
    std::vector<ExprPtr> args;
    // ...
};

// 배열 읽기: arr[i]
struct IndexGetExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    Token   bracket;   // 오류 위치 추적용 '[' 토큰
    // ...
};

// 배열 쓰기: arr[i] = value
struct IndexSetExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    ExprPtr value;
    Token   bracket;
    // ...
};
```

**ExprVisitor에 추가:**
```cpp
virtual Value visitCallExpr    (CallExpr&     expr) = 0;
virtual Value visitIndexGetExpr(IndexGetExpr& expr) = 0;
virtual Value visitIndexSetExpr(IndexSetExpr& expr) = 0;
```

> Resolver와 Interpreter가 `ExprVisitor`를 상속하므로 **양쪽 모두에 구현 필수**.

---

### 3-5. `Stmt.h` 변경 내용

**추가 노드 2종 (Chapter 2):**

```cpp
// Func name(param1, param2) { body }
struct FuncStmt : Stmt {
    Token                name;
    std::vector<Token>   params;
    std::vector<StmtPtr> body;
    // ...
};

// return [expr] ;
struct ReturnStmt : Stmt {
    Token   keyword;   // 오류 위치 추적용 'return' 토큰
    ExprPtr value;     // nullptr이면 nil 반환
    // ...
};
```

**StmtVisitor에 추가:**
```cpp
virtual void visitFuncStmt  (FuncStmt&   stmt) = 0;
virtual void visitReturnStmt(ReturnStmt& stmt) = 0;
```

---

### 3-6. `Parser.cpp` 변경 내용

**문법 규칙 확장:**

```
// 선언부 확장
declaration → funcDeclaration | varDeclaration | statement

// 함수 선언
funcDeclaration → "Func" IDENTIFIER "(" params? ")" blockStatement
params          → IDENTIFIER ( "," IDENTIFIER )*

// 문(statement) 확장
statement → ... | returnStatement

// return 문
returnStatement → "return" expression? ";"

// 표현식 후위 처리 (primary 이후)
postfix → postfix "(" args? ")"       // 함수 호출 → CallExpr
        | postfix "[" expression "]"  // 인덱스 읽기 → IndexGetExpr

// 대입 좌변 확장
assignment → (postfix "[" expr "]") "=" assignment   // IndexSetExpr
           | IDENTIFIER "=" assignment               // AssignExpr (기존)
           | comparison                              // 기존
```

---

### 3-7. `Resolver.cpp` 변경 내용

**Chapter 2 추가 검사:**

| 추가 항목 | 구현 방법 |
|---|---|
| 함수 내부 여부 추적 | `enum FunctionType { NONE, FUNCTION };` 멤버 변수 추가 |
| `visitFuncStmt` | 새 스코프 시작 → params 등록 (중복 체크) → body resolve → 스코프 종료 |
| `visitReturnStmt` | `currentFunction == NONE`이면 StaticError throw |
| `visitCallExpr` | callee와 args를 각각 `resolveExpr()` 호출 |

**Chapter 3 추가:**

| 추가 항목 | 구현 방법 |
|---|---|
| `visitIndexGetExpr` | object, index를 각각 `resolveExpr()` 호출 |
| `visitIndexSetExpr` | object, index, value를 각각 `resolveExpr()` 호출 |

---

### 3-8. `Interpreter.cpp` 변경 내용

**Chapter 2 추가:**

| 추가 항목 | 구현 내용 |
|---|---|
| `visitFuncStmt` | `FabFunction` 생성 → `shared_ptr`로 Value에 담아 환경에 `define()` |
| `visitReturnStmt` | value 평가 후 `ReturnSignal` throw |
| `visitCallExpr` | ① callee 평가 → FabFunction 확인 (아니면 RuntimeError)<br>② args 개수 != params 개수 → RuntimeError<br>③ 새 Environment(closure 기반) 생성 → params 바인딩<br>④ body 실행 → `ReturnSignal` catch → 반환값 추출 |

**Chapter 3 추가:**

| 추가 항목 | 구현 내용 |
|---|---|
| `visitIndexGetExpr` | ① object 평가 → `FabArray` 확인 (아니면 RuntimeError)<br>② index 평가 → double 확인 (아니면 RuntimeError)<br>③ 범위 검사 (아니면 RuntimeError) → `elements[idx]` 반환 |
| `visitIndexSetExpr` | 위와 동일하되 `elements[idx] = value` 설정 |
| `Array(n)` 내장 처리 | `visitCallExpr`에서 callee 이름이 `"Array"`이면 별도 분기: size 평가 → double 확인 → `FabArray` 생성 |

---

## 4. 설계 결정 사항

### 4-1. `return` 제어 흐름 — 예외 방식 채택

함수 body는 중첩된 블록/조건문 속에서 `return`이 발생할 수 있습니다.  
C++ 예외(`ReturnSignal`)를 throw/catch하는 방식이 가장 단순하며,  
기존 `RuntimeError` 예외 패턴과 일관성이 있습니다.

```cpp
// visitReturnStmt
throw ReturnSignal{ value };

// visitCallExpr 내부
try {
    executeBlock(func.body, callEnv);
} catch (ReturnSignal& ret) {
    return ret.value;
}
return nullptr;  // return 없으면 nil
```

### 4-2. `Array(n)` 처리 방식

`Array`는 키워드로 등록(`ARRAY_KW`)하고 `visitCallExpr`에서 특수 케이스로 처리합니다.  
별도 `ArrayExpr` 노드를 만들지 않아 Parser/Resolver/Interpreter 변경을 최소화합니다.

### 4-3. Value 확장에 따른 기존 코드 영향

`stringify()`, `isEqual()` 에 분기 추가 필요:
- `FabFunction` → `"<function 이름>"` 형태 문자열
- `FabArray` → 출력 시 `"[Array(N)]"` 또는 원소 나열 (요구사항 명시 없음, 팀 결정 필요)

---

## 5. 구현 순서 (권장)

```
Step 1  Token.h        토큰 추가 + Value variant 확장
        FabFunction.h  신규 작성

Step 2  Expr.h         CallExpr, IndexGetExpr, IndexSetExpr 노드 추가
        Stmt.h         FuncStmt, ReturnStmt 노드 추가
        → 컴파일 오류로 미구현 visit 위치 자동 가이드

Step 3  Resolver       새 visit 메서드 추가 (오류 검사 로직 포함)

Step 4  Interpreter    visitFuncStmt, visitReturnStmt, visitCallExpr 구현
        Parser         함수 선언/호출 파싱
        → Chapter 2 기능 완성 및 테스트

Step 5  Lexer          '[', ']', Array 토큰 추가
        Resolver       visitIndexGetExpr, visitIndexSetExpr 추가
        Interpreter    visitIndexGetExpr, visitIndexSetExpr, Array 내장 처리
        Parser         배열 파싱 (후위 '[', IndexSetExpr)
        → Chapter 3 기능 완성 및 테스트

Step 6  UnitTest       추가 기능 테스트 케이스 작성 (요구사항 유의사항 3번)
```

---

## 6. 팀 분담 시 주의사항

`Expr.h`, `Stmt.h`, `Resolver.cpp`, `Interpreter.cpp`는 두 챕터 모두 수정하는 **충돌 다발 파일**입니다.

- **권장 Option A (순차)**: Chapter 2 완료 후 merge → Chapter 3 시작
- **권장 Option B (병렬)**: `Expr.h`/`Stmt.h` 노드 정의를 먼저 합의·commit 후 각자 브랜치에서 Resolver/Interpreter 작업

> 유의사항: 기존 `test1_expressions.fab` 등 기존 테스트 파일은 변경 금지,  
> Value variant 확장 후 기존 테스트가 모두 통과하는지 반드시 확인.
