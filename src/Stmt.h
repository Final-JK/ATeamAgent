#pragma once
#include "Expr.h"
#include <vector>
#include <memory>

// Forward declarations
struct PrintStmt;
struct ExprStmt;
struct VarStmt;
struct BlockStmt;
struct IfStmt;
struct ForStmt;
struct FuncStmt;    // Ch.2: 함수 선언
struct ReturnStmt;  // Ch.2: return 문

// ── Visitor interface ──────────────────────────────────────────────────────────
struct StmtVisitor {
    virtual void visitPrintStmt (PrintStmt&  stmt) = 0;
    virtual void visitExprStmt  (ExprStmt&   stmt) = 0;
    virtual void visitVarStmt   (VarStmt&    stmt) = 0;
    virtual void visitBlockStmt (BlockStmt&  stmt) = 0;
    virtual void visitIfStmt    (IfStmt&     stmt) = 0;
    virtual void visitForStmt   (ForStmt&    stmt) = 0;
    virtual void visitFuncStmt  (FuncStmt&   stmt) = 0;  // Ch.2
    virtual void visitReturnStmt(ReturnStmt& stmt) = 0;  // Ch.2
    virtual ~StmtVisitor() = default;
};

// ── Base Statement Node ────────────────────────────────────────────────────────
struct Stmt {
    virtual void accept(StmtVisitor& visitor) = 0;
    virtual ~Stmt() = default;
};
using StmtPtr = std::unique_ptr<Stmt>;

// ── Concrete Statement Nodes ───────────────────────────────────────────────────

// print expr ;
struct PrintStmt : Stmt {
    ExprPtr expression;
    explicit PrintStmt(ExprPtr expr) : expression(std::move(expr)) {}
    void accept(StmtVisitor& v) override { v.visitPrintStmt(*this); }
};

// expr ;
struct ExprStmt : Stmt {
    ExprPtr expression;
    explicit ExprStmt(ExprPtr expr) : expression(std::move(expr)) {}
    void accept(StmtVisitor& v) override { v.visitExprStmt(*this); }
};

// var name = initializer ;
struct VarStmt : Stmt {
    Token   name;
    ExprPtr initializer; // nullptr → nil
    VarStmt(Token name, ExprPtr init) : name(std::move(name)), initializer(std::move(init)) {}
    void accept(StmtVisitor& v) override { v.visitVarStmt(*this); }
};

// { statements }
struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
    explicit BlockStmt(std::vector<StmtPtr> stmts) : statements(std::move(stmts)) {}
    void accept(StmtVisitor& v) override { v.visitBlockStmt(*this); }
};

// if ( condition ) thenBranch [else elseBranch]
struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch; // nullptr if no else
    IfStmt(ExprPtr cond, StmtPtr then_, StmtPtr else_)
        : condition(std::move(cond)), thenBranch(std::move(then_)), elseBranch(std::move(else_)) {}
    void accept(StmtVisitor& v) override { v.visitIfStmt(*this); }
};

// for ( init ; condition ; increment ) body
struct ForStmt : Stmt {
    StmtPtr initializer; // nullptr if omitted
    ExprPtr condition;   // nullptr → run forever
    ExprPtr increment;   // nullptr if omitted
    StmtPtr body;
    ForStmt(StmtPtr init, ExprPtr cond, ExprPtr incr, StmtPtr body)
        : initializer(std::move(init)), condition(std::move(cond))
        , increment(std::move(incr)), body(std::move(body)) {}
    void accept(StmtVisitor& v) override { v.visitForStmt(*this); }
};

// Ch.2: 함수 선언 - Func name(param1, param2) { body }
// body를 소유하므로 FabFunction은 비소유 포인터로 참조
struct FuncStmt : Stmt {
    Token                name;
    std::vector<Token>   params;
    std::vector<StmtPtr> body;
    FuncStmt(Token name, std::vector<Token> params, std::vector<StmtPtr> body)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
    void accept(StmtVisitor& v) override { v.visitFuncStmt(*this); }
};

// Ch.2: return 문 - return [expr] ;
// keyword는 함수 외부 return 오류 보고 시 위치 추적용 'return' 토큰
// value가 nullptr이면 nil 반환
struct ReturnStmt : Stmt {
    Token   keyword;
    ExprPtr value;
    ReturnStmt(Token keyword, ExprPtr value)
        : keyword(std::move(keyword)), value(std::move(value)) {}
    void accept(StmtVisitor& v) override { v.visitReturnStmt(*this); }
};
