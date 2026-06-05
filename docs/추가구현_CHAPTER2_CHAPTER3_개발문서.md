# 추가구현 개발문서 — Chapter 2 & Chapter 3

**작성일**: 2026-06-05  
**대상 프로젝트**: CustomInterpreter (CodeFab 교육 과정)  
**구현 범위**: Chapter 2 함수(function) 기능 / Chapter 3 정적 배열(static array) 기능

---

## 1. 개요

기존 인터프리터(변수, 산술, 조건문, 반복문)에 함수와 배열을 추가한다.  
처리 파이프라인(`Lexer → Parser → Resolver → Interpreter`)의 각 단계를 일관되게 확장하였으며,  
기존 코드와 테스트에는 영향을 주지 않는다.

---

## 2. 커밋 이력

| 커밋 해시 | 제목 | 범위 |
|-----------|------|------|
| `2194b3d` | feat: Ch.2/3 지원을 위한 Token/Lexer 기반 확장 | Token.h, FabValue.h, Lexer.cpp |
| `d300470` | feat(Ch.2): 함수 선언/호출/return 기능 구현 | Expr.h, Stmt.h, Resolver, Interpreter, Parser + 테스트 |
| `4114ef6` | feat(Ch.3): 고정 크기 배열 생성/읽기/쓰기 기능 구현 | 테스트 파일 추가 및 검증 |

---

## 3. Chapter 2 — 함수(function) 구현

### 3-1. 지원 기능

| 기능 | 예시 코드 |
|------|-----------|
| 함수 선언 | `Func add(a, b) { return a + b; }` |
| 함수 호출 / 인수 전달 | `add(3, 7);` |
| return 값 대입 | `var ret = add(3, 7);` |
| return 없는 함수 → nil 반환 | `var r = noReturn();` → `r == nil` |
| 재귀 호출 | `Func fact(n) { if (n <= 1) return 1; return n * fact(n-1); }` |

### 3-2. 오류 검사

| 오류 상황 | 단계 | 오류 메시지 |
|-----------|------|-------------|
| 함수 외부에서 `return` 사용 | Resolver (Static) | `Can't return from top-level code.` |
| 파라미터 이름 중복 | Resolver (Static) | `Duplicate parameter name 'a'.` |
| 함수가 아닌 대상 호출 | Interpreter (Runtime) | `Can only call functions.` |
| 인자 개수 불일치 | Interpreter (Runtime) | `Expected N arguments but got M.` |

### 3-3. 신규/변경 파일

#### `src/Token.h`
- `COMMA`, `FUNC`, `RETURN` 토큰 추가
- `Value` variant에 `shared_ptr<FabFunction>` 추가
- `FabFunction` forward declaration 추가

#### `src/FabValue.h` (신규)
- `FabFunction` 구조체: 함수 런타임 객체
  - `name`, `params`, `body`(비소유 포인터), `closure`(환경 캡처)
- `ReturnSignal` 구조체: C++ 예외 기반 return 제어 흐름

#### `src/Expr.h`
- `CallExpr` 추가: `callee(arg1, arg2, ...)`
  - `callee`: 호출 대상 표현식
  - `paren`: 런타임 오류 위치 추적용 `(` 토큰
  - `args`: 인수 목록

#### `src/Stmt.h`
- `FuncStmt` 추가: `Func name(params) { body }`
  - `body`를 소유 (`FabFunction`은 비소유 포인터로 참조)
- `ReturnStmt` 추가: `return [expr] ;`
  - `keyword`: 오류 위치 추적용 `return` 토큰

#### `src/Resolver.h` / `Resolver.cpp`
- `FunctionType { NONE, FUNCTION }` enum 추가
- `currentFunction` 멤버로 함수 컨텍스트 추적
- `visitFuncStmt`: 외부 스코프에 함수 이름 등록(재귀 허용) → 파라미터 중복 검사 → 본문 resolve
- `visitReturnStmt`: `currentFunction == NONE`이면 Static Error
- `visitCallExpr`: callee + 인수 일괄 resolve

#### `src/Parser.h` / `Parser.cpp`
- `declaration()`: `FUNC` → `funcDeclaration()`
- `statement()`: `RETURN` → `returnStatement()`
- `unary()` 이후 `call()` 삽입 (후위 처리 계층 추가)
- `call()`: `(` 반복 → `CallExpr`, `[` 반복 → `IndexGetExpr`
- `finishCall()`: 인수 파싱 및 `CallExpr` 완성

#### `src/Interpreter.h` / `Interpreter.cpp`
- `visitFuncStmt`: `FabFunction` 생성, 클로저 환경 캡처 후 환경에 등록
- `visitReturnStmt`: `ReturnSignal` throw
- `visitCallExpr`:
  1. callee가 `FabFunction`인지 검사
  2. 인수 개수 검사
  3. 클로저 기반 새 `Environment` 생성, 파라미터 바인딩
  4. `executeBlock()` 호출 → `ReturnSignal` catch → 반환값 추출
- `stringify()` / `isEqual()`: `FabFunction` 분기 추가

### 3-4. 설계 결정: return 제어 흐름

`return` 문이 중첩 블록 안에 있을 경우 호출 스택을 한 번에 탈출해야 한다.  
C++ 예외(`ReturnSignal`)를 throw/catch하는 방식을 채택한 이유:

- 기존 `executeBlock()`의 `catch(...)` 패턴과 일관성 유지
- `RuntimeError`와 구분되는 별도 타입으로 오동작 없음
- `for` 루프 내 `return`도 `visitForStmt`의 `catch(...) { rethrow; }` 구조를 통해 자연스럽게 전파

```cpp
// visitCallExpr 핵심 로직
try {
    executeBlock(*fn.body, callEnv);
} catch (ReturnSignal& ret) {
    return ret.value;  // return 값 추출
}
return nullptr;        // 명시적 return 없으면 nil
```

### 3-5. 설계 결정: 클로저(Lexical Scoping)

```cpp
// visitFuncStmt
fn->closure = currentEnv;  // 선언 시점의 환경 캡처

// visitCallExpr
auto callEnv = std::make_shared<Environment>(fn->closure);  // 클로저를 parent로
```

함수가 **호출된 위치**가 아닌 **선언된 위치**의 변수를 참조한다.  
`FabFunction::body`는 `FuncStmt`의 비소유 포인터이므로 AST가 살아있는 동안(= `interpret()` 수명) 안전하다.

---

## 4. Chapter 3 — 정적 배열(static array) 구현

### 4-1. 지원 기능

| 기능 | 예시 코드 |
|------|-----------|
| 배열 생성 (고정 크기, nil 초기화) | `var arr = Array(3);` |
| 인덱스 쓰기 | `arr[0] = 10;` |
| 인덱스 읽기 | `print arr[0];` |
| 표현식 인덱스 | `arr[i - 1] = 7;` |

### 4-2. 오류 검사 (모두 Runtime Error)

| 오류 상황 | 오류 메시지 |
|-----------|-------------|
| 범위를 벗어난 인덱스 접근 | `Index N out of bounds (size M).` |
| 인덱스가 숫자가 아닌 경우 | `Index must be a number.` |
| 배열이 아닌 대상에 `[]` 사용 | `Only arrays support index access.` |
| 배열 생성 시 크기가 숫자가 아님 | `Array size must be a number.` |

### 4-3. 신규/변경 파일

#### `src/Token.h`
- `LEFT_BRACKET`, `RIGHT_BRACKET`, `ARRAY_KW` 토큰 추가
- `Value` variant에 `shared_ptr<FabArray>` 추가
- `FabArray` forward declaration 추가

#### `src/FabValue.h`
- `FabArray` 구조체: `std::vector<Value> elements`, 생성 시 nil 초기화

#### `src/Expr.h`
- `IndexGetExpr` 추가: `arr[i]` 읽기
  - `object`, `index`, `bracket`(오류 위치 추적용 `[` 토큰)
- `IndexSetExpr` 추가: `arr[i] = value` 쓰기
  - Parser의 `assignment()`에서 `IndexGetExpr` 좌변 감지 후 변환
- `ArrayCreateExpr` 추가: `Array(size)`
  - `keyword`(오류 위치 추적용 `Array` 토큰), `size`

#### `src/Parser.cpp`
- `assignment()`: 좌변이 `IndexGetExpr`이면 `IndexSetExpr`로 변환
- `call()`: `[` → `IndexGetExpr` 후위 처리
- `primary()`: `ARRAY_KW` 감지 → `Array(n)` 파싱 → `ArrayCreateExpr` 반환

#### `src/Resolver.cpp`
- `visitIndexGetExpr`: object, index를 각각 resolve
- `visitIndexSetExpr`: object, index, value를 각각 resolve
- `visitArrayCreateExpr`: size를 resolve

#### `src/Interpreter.cpp`
- `visitArrayCreateExpr`: size 평가 → 타입/양수 검사 → `FabArray` 생성
- `visitIndexGetExpr`: 배열 타입 검사 → 숫자 인덱스 검사 → 범위 검사 → 원소 반환
- `visitIndexSetExpr`: 동일 검사 → 원소 갱신 후 값 반환
- `stringify()` / `isEqual()`: `FabArray` 분기 추가

### 4-4. 설계 결정: `Array()` 처리 방식

`Array`를 키워드(`ARRAY_KW`)로 등록하고 `Parser::primary()`에서 `ArrayCreateExpr`로 직접 파싱한다.  
`call()` 후위 처리를 거치지 않으므로 일반 함수 호출과 구분된다.

```
// Parser 흐름
primary() → ARRAY_KW 감지
          → '(' size ')' 파싱
          → ArrayCreateExpr 반환
          (call()의 후위 체인 밖에서 처리)
```

대안(내장 함수 등록)보다 AST 노드를 명시적으로 분리하는 방식이  
Resolver/Interpreter에서 타입 검사를 더 명확하게 표현할 수 있어 채택했다.

---

## 5. 파일 변경 요약

| 파일 | 상태 | 주요 변경 내용 |
|------|------|----------------|
| `src/Token.h` | 수정 | 토큰 6종 추가, Value variant 확장 |
| `src/FabValue.h` | **신규** | FabFunction, FabArray, ReturnSignal 정의 |
| `src/Lexer.cpp` | 수정 | 키워드 3종, 문자 3종 인식 추가 |
| `src/Expr.h` | 수정 | 노드 4종 추가(CallExpr, IndexGetExpr, IndexSetExpr, ArrayCreateExpr) |
| `src/Stmt.h` | 수정 | 노드 2종 추가(FuncStmt, ReturnStmt) |
| `src/Resolver.h` | 수정 | FunctionType enum, visit 메서드 6종 추가 |
| `src/Resolver.cpp` | 수정 | 정적 검사 구현 (함수 4종 + 배열 3종) |
| `src/Interpreter.h` | 수정 | FabValue.h 포함, visit 메서드 6종 추가 |
| `src/Interpreter.cpp` | 수정 | 실행 로직 구현 (함수 3종 + 배열 3종) |
| `src/Parser.h` | 수정 | call(), finishCall() 등 메서드 4종 추가 |
| `src/Parser.cpp` | 수정 | 문법 규칙 확장 (함수 + 배열 파싱) |

---

## 6. 테스트 파일

| 파일 | 종류 | 기대 출력 / 오류 메시지 |
|------|------|------------------------|
| `tests/test4_functions.fab` | 정상 | `10`, `120`, `Hello World`, `nil` |
| `tests/test5_arrays.fab` | 정상 | `10`, `20`, `30`, `7` |
| `tests/err6_return_outside_function.fab` | Static Error (65) | `Can't return from top-level code.` |
| `tests/err7_duplicate_param.fab` | Static Error (65) | `Duplicate parameter name 'a'.` |
| `tests/err8_call_non_function.fab` | Runtime Error (70) | `Can only call functions.` |
| `tests/err9_arg_count_mismatch.fab` | Runtime Error (70) | `Expected 3 arguments but got 2.` |
| `tests/err10_array_out_of_bounds.fab` | Runtime Error (70) | `Index 5 out of bounds (size 3).` |
| `tests/err11_array_non_number_index.fab` | Runtime Error (70) | `Index must be a number.` |
| `tests/err12_index_non_array.fab` | Runtime Error (70) | `Only arrays support index access.` |
| `tests/err13_array_non_number_size.fab` | Runtime Error (70) | `Array size must be a number.` |

실행 방법: `out\interpreter.exe tests\<파일명>.fab`

---

## 7. 회귀 테스트 결과

기존 테스트(`test1~test3`) 모두 변경 없이 통과 확인.

| 파일 | 결과 |
|------|------|
| `tests/test1_expressions.fab` | ✅ 통과 |
| `tests/test2_variables.fab` | ✅ 통과 |
| `tests/test3_controlflow.fab` | ✅ 통과 |
