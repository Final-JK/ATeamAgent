#pragma once
#include "Stmt.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <stdexcept>

// ── Static (Semantic) Error ────────────────────────────────────────────────────
struct StaticError : std::runtime_error {
    explicit StaticError(const std::string& msg) : std::runtime_error(msg) {}
};

// ── Resolver: static scope checker ────────────────────────────────────────────
// 검사 항목:
//   1. 자기 초기화자에서 변수 읽기  (var a = a;)
//   2. 같은 스코프 내 중복 선언     (var a = 1; var a = 2;)
class Resolver : public ExprVisitor, public StmtVisitor {
public:
    void resolve(const std::vector<StmtPtr>& statements);

private:
    // 스코프 스택: 변수명 → initialized 여부
    std::vector<std::unordered_map<std::string, bool>> scopes;

    void resolveStmt (Stmt& stmt);
    void resolveExpr (Expr& expr);
    void resolveBlock(const std::vector<StmtPtr>& stmts);
    void beginScope();
    void endScope();
    void declare(const Token& name);
    void define (const Token& name);

    // ExprVisitor (반환값은 사용 안 함 — nullptr 반환)
    Value visitLiteralExpr (LiteralExpr&  expr) override;
    Value visitUnaryExpr   (UnaryExpr&    expr) override;
    Value visitBinaryExpr  (BinaryExpr&   expr) override;
    Value visitGroupingExpr(GroupingExpr& expr) override;
    Value visitVariableExpr(VariableExpr& expr) override;
    Value visitAssignExpr  (AssignExpr&   expr) override;

    // StmtVisitor
    void visitPrintStmt(PrintStmt& stmt) override;
    void visitExprStmt (ExprStmt&  stmt) override;
    void visitVarStmt  (VarStmt&   stmt) override;
    void visitBlockStmt(BlockStmt& stmt) override;
    void visitIfStmt   (IfStmt&    stmt) override;
    void visitForStmt  (ForStmt&   stmt) override;
};
