#pragma once
#include "Environment.h"  // FabFunction::closure에 필요한 완전 타입
#include "Stmt.h"          // StmtPtr, Token, Value (간접 포함)
#include <vector>

// ── Ch.2: 사용자 정의 함수 런타임 객체 ────────────────────────────────────────────
// body는 FuncStmt가 소유하는 벡터의 비소유 포인터
// AST(stmts)의 수명이 interpret() 호출 전체를 포괄하므로 dangling 없음
// closure는 함수 선언 시점의 렉시컬 환경을 캡처 (lexical scoping 구현)
struct FabFunction {
    std::string                  name;
    std::vector<Token>           params;
    const std::vector<StmtPtr>*  body;     // 비소유; FuncStmt::body를 가리킴
    std::shared_ptr<Environment> closure;

    int arity() const { return static_cast<int>(params.size()); }
};

// ── Ch.3: 고정 크기 배열 런타임 객체 ──────────────────────────────────────────────
// 생성 시 크기가 확정되며 모든 원소는 nil(nullptr_t)로 초기화
// shared_ptr로 보관하므로 여러 변수가 같은 배열을 참조할 수 있음
struct FabArray {
    std::vector<Value> elements;
    explicit FabArray(int size) : elements(size, nullptr) {}
};

// ── Ch.2: return 제어 흐름 신호 ────────────────────────────────────────────────
// C++ 예외 기반으로 return 문을 구현
// RuntimeError와 구분하기 위해 별도 타입 사용
// Interpreter::visitCallExpr에서 catch하여 반환값을 추출
struct ReturnSignal {
    Value value;
};
