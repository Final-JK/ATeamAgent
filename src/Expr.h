#pragma once
#include "Token.h"
#include <memory>
#include <vector>

// Forward declarations
struct LiteralExpr;
struct UnaryExpr;
struct BinaryExpr;
struct GroupingExpr;
struct VariableExpr;
struct AssignExpr;
struct CallExpr;       // Ch.2: 함수 호출
struct IndexGetExpr;   // Ch.3: 배열 인덱스 읽기
struct IndexSetExpr;   // Ch.3: 배열 인덱스 쓰기
struct ArrayCreateExpr;// Ch.3: 배열 생성 Array(n)

// ── Visitor interface ──────────────────────────────────────────────────────────
struct ExprVisitor {
    virtual Value visitLiteralExpr    (LiteralExpr&     expr) = 0;
    virtual Value visitUnaryExpr      (UnaryExpr&       expr) = 0;
    virtual Value visitBinaryExpr     (BinaryExpr&      expr) = 0;
    virtual Value visitGroupingExpr   (GroupingExpr&    expr) = 0;
    virtual Value visitVariableExpr   (VariableExpr&    expr) = 0;
    virtual Value visitAssignExpr     (AssignExpr&      expr) = 0;
    virtual Value visitCallExpr       (CallExpr&        expr) = 0;  // Ch.2
    virtual Value visitIndexGetExpr   (IndexGetExpr&    expr) = 0;  // Ch.3
    virtual Value visitIndexSetExpr   (IndexSetExpr&    expr) = 0;  // Ch.3
    virtual Value visitArrayCreateExpr(ArrayCreateExpr& expr) = 0;  // Ch.3
    virtual ~ExprVisitor() = default;
};

// ── Base Expression Node ───────────────────────────────────────────────────────
struct Expr {
    virtual Value accept(ExprVisitor& visitor) = 0;
    virtual ~Expr() = default;
};
using ExprPtr = std::unique_ptr<Expr>;

// ── Concrete Expression Nodes ──────────────────────────────────────────────────

// 숫자, 문자열, bool, nil 리터럴
struct LiteralExpr : Expr {
    Value value;
    explicit LiteralExpr(Value v) : value(std::move(v)) {}
    Value accept(ExprVisitor& v) override { return v.visitLiteralExpr(*this); }
};

// 단항 연산자: -expr
struct UnaryExpr : Expr {
    Token   op;
    ExprPtr right;
    UnaryExpr(Token op, ExprPtr right) : op(std::move(op)), right(std::move(right)) {}
    Value accept(ExprVisitor& v) override { return v.visitUnaryExpr(*this); }
};

// 이항 연산자: left op right
struct BinaryExpr : Expr {
    ExprPtr left;
    Token   op;
    ExprPtr right;
    BinaryExpr(ExprPtr l, Token op, ExprPtr r)
        : left(std::move(l)), op(std::move(op)), right(std::move(r)) {}
    Value accept(ExprVisitor& v) override { return v.visitBinaryExpr(*this); }
};

// 괄호 그룹: ( expr )
struct GroupingExpr : Expr {
    ExprPtr expression;
    explicit GroupingExpr(ExprPtr expr) : expression(std::move(expr)) {}
    Value accept(ExprVisitor& v) override { return v.visitGroupingExpr(*this); }
};

// 변수 참조: identifier
struct VariableExpr : Expr {
    Token name;
    explicit VariableExpr(Token name) : name(std::move(name)) {}
    Value accept(ExprVisitor& v) override { return v.visitVariableExpr(*this); }
};

// 변수 대입: identifier = expr
struct AssignExpr : Expr {
    Token   name;
    ExprPtr value;
    AssignExpr(Token name, ExprPtr val) : name(std::move(name)), value(std::move(val)) {}
    Value accept(ExprVisitor& v) override { return v.visitAssignExpr(*this); }
};

// Ch.2: 함수 호출 표현식 - callee(arg1, arg2, ...)
// paren은 런타임 오류 발생 시 정확한 위치를 보고하기 위해 보관하는 '(' 토큰
struct CallExpr : Expr {
    ExprPtr              callee;
    Token                paren;
    std::vector<ExprPtr> args;
    CallExpr(ExprPtr callee, Token paren, std::vector<ExprPtr> args)
        : callee(std::move(callee)), paren(std::move(paren)), args(std::move(args)) {}
    Value accept(ExprVisitor& v) override { return v.visitCallExpr(*this); }
};

// Ch.3: 배열 인덱스 읽기 - arr[i]
// bracket은 런타임 오류 발생 시 정확한 위치를 보고하기 위해 보관하는 '[' 토큰
struct IndexGetExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    Token   bracket;
    IndexGetExpr(ExprPtr obj, ExprPtr idx, Token bracket)
        : object(std::move(obj)), index(std::move(idx)), bracket(std::move(bracket)) {}
    Value accept(ExprVisitor& v) override { return v.visitIndexGetExpr(*this); }
};

// Ch.3: 배열 인덱스 쓰기 - arr[i] = value
// IndexGetExpr에서 파싱 후 assignment에서 IndexSetExpr로 변환됨
struct IndexSetExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    ExprPtr value;
    Token   bracket;
    IndexSetExpr(ExprPtr obj, ExprPtr idx, ExprPtr val, Token bracket)
        : object(std::move(obj)), index(std::move(idx)), value(std::move(val)), bracket(std::move(bracket)) {}
    Value accept(ExprVisitor& v) override { return v.visitIndexSetExpr(*this); }
};

// Ch.3: 배열 생성 - Array(size)
// size는 양의 정수여야 하며, 모든 원소를 nil로 초기화한 배열 반환
struct ArrayCreateExpr : Expr {
    Token   keyword;  // 런타임 오류 위치 추적용 'Array' 토큰
    ExprPtr size;
    ArrayCreateExpr(Token keyword, ExprPtr size)
        : keyword(std::move(keyword)), size(std::move(size)) {}
    Value accept(ExprVisitor& v) override { return v.visitArrayCreateExpr(*this); }
};
