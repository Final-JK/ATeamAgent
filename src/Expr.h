#pragma once
#include "Token.h"
#include <memory>

// Forward declarations
struct LiteralExpr;
struct UnaryExpr;
struct BinaryExpr;
struct GroupingExpr;
struct VariableExpr;
struct AssignExpr;

// ── Visitor interface ──────────────────────────────────────────────────────────
struct ExprVisitor {
    virtual Value visitLiteralExpr (LiteralExpr&  expr) = 0;
    virtual Value visitUnaryExpr   (UnaryExpr&    expr) = 0;
    virtual Value visitBinaryExpr  (BinaryExpr&   expr) = 0;
    virtual Value visitGroupingExpr(GroupingExpr& expr) = 0;
    virtual Value visitVariableExpr(VariableExpr& expr) = 0;
    virtual Value visitAssignExpr  (AssignExpr&   expr) = 0;
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
    Token  op;
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

// 대입: identifier = expr
struct AssignExpr : Expr {
    Token   name;
    ExprPtr value;
    AssignExpr(Token name, ExprPtr val) : name(std::move(name)), value(std::move(val)) {}
    Value accept(ExprVisitor& v) override { return v.visitAssignExpr(*this); }
};
