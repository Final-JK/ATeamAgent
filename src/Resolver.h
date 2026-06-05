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

// Ch.2: 현재 분석 중인 함수 컨텍스트 추적
// NONE = 최상위 스코프, FUNCTION = 함수 본문 내부
// return 문이 올바른 위치에 있는지 검사하기 위해 사용
enum class FunctionType { NONE, FUNCTION };

// ── Resolver: static scope checker ────────────────────────────────────────────
// 검사 항목:
//   1. 자기 초기화자에서 변수 읽기  (var a = a;)
//   2. 같은 스코프 내 중복 선언     (var a = 1; var a = 2;)
//   3. [Ch.2] 함수 외부에서 return  (return 5; at top-level)
//   4. [Ch.2] 파라미터 이름 중복    (Func foo(a, a) { })
class Resolver : public ExprVisitor, public StmtVisitor {
public:
    void resolve(const std::vector<StmtPtr>& statements);

private:
    // 스코프 스택: 변수명 → initialized 여부
    std::vector<std::unordered_map<std::string, bool>> scopes;
    // Ch.2: 현재 함수 컨텍스트; return 유효성 검사에 사용
    FunctionType currentFunction = FunctionType::NONE;

    void resolveStmt (Stmt& stmt);
    void resolveExpr (Expr& expr);
    void resolveBlock(const std::vector<StmtPtr>& stmts);
    void beginScope();
    void endScope();
    void declare(const Token& name);
    void define (const Token& name);

    // ExprVisitor
    Value visitLiteralExpr    (LiteralExpr&     expr) override;
    Value visitUnaryExpr      (UnaryExpr&       expr) override;
    Value visitBinaryExpr     (BinaryExpr&      expr) override;
    Value visitGroupingExpr   (GroupingExpr&    expr) override;
    Value visitVariableExpr   (VariableExpr&    expr) override;
    Value visitAssignExpr     (AssignExpr&      expr) override;
    Value visitCallExpr       (CallExpr&        expr) override;  // Ch.2
    Value visitIndexGetExpr   (IndexGetExpr&    expr) override;  // Ch.3
    Value visitIndexSetExpr   (IndexSetExpr&    expr) override;  // Ch.3
    Value visitArrayCreateExpr(ArrayCreateExpr& expr) override;  // Ch.3

    // StmtVisitor
    void visitPrintStmt (PrintStmt&  stmt) override;
    void visitExprStmt  (ExprStmt&   stmt) override;
    void visitVarStmt   (VarStmt&    stmt) override;
    void visitBlockStmt (BlockStmt&  stmt) override;
    void visitIfStmt    (IfStmt&     stmt) override;
    void visitForStmt   (ForStmt&    stmt) override;
    void visitFuncStmt  (FuncStmt&   stmt) override;  // Ch.2
    void visitReturnStmt(ReturnStmt& stmt) override;  // Ch.2
};
