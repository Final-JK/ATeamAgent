#include "Resolver.h"

void Resolver::resolve(const std::vector<StmtPtr>& statements) {
    resolveBlock(statements);
}

void Resolver::resolveStmt (Stmt& stmt) { stmt.accept(*this); }
void Resolver::resolveExpr (Expr& expr) { expr.accept(*this); }

void Resolver::resolveBlock(const std::vector<StmtPtr>& stmts) {
    for (const auto& s : stmts) resolveStmt(*s);
}

void Resolver::beginScope() { scopes.push_back({}); }
void Resolver::endScope()   { scopes.pop_back(); }

void Resolver::declare(const Token& name) {
    if (scopes.empty()) return; // 전역 스코프는 중복 검사 안 함
    auto& scope = scopes.back();
    if (scope.count(name.lexeme))
        throw StaticError("[line " + std::to_string(name.line) +
            "] Static Error: Already a variable with this name in this scope.");
    scope[name.lexeme] = false; // declared, not yet initialized
}

void Resolver::define(const Token& name) {
    if (!scopes.empty()) scopes.back()[name.lexeme] = true;
}

// ── ExprVisitor ────────────────────────────────────────────────────────────────

Value Resolver::visitLiteralExpr(LiteralExpr&) { return nullptr; }

Value Resolver::visitGroupingExpr(GroupingExpr& expr) {
    resolveExpr(*expr.expression);
    return nullptr;
}

Value Resolver::visitUnaryExpr(UnaryExpr& expr) {
    resolveExpr(*expr.right);
    return nullptr;
}

Value Resolver::visitBinaryExpr(BinaryExpr& expr) {
    resolveExpr(*expr.left);
    resolveExpr(*expr.right);
    return nullptr;
}

Value Resolver::visitVariableExpr(VariableExpr& expr) {
    // 현재 스코프에 선언만 되고 아직 초기화 안 됐으면 자기 참조 오류
    if (!scopes.empty()) {
        auto& scope = scopes.back();
        auto  it    = scope.find(expr.name.lexeme);
        if (it != scope.end() && !it->second)
            throw StaticError("[line " + std::to_string(expr.name.line) +
                "] Static Error: Can't read local variable in its own initializer.");
    }
    return nullptr;
}

Value Resolver::visitAssignExpr(AssignExpr& expr) {
    resolveExpr(*expr.value);
    return nullptr;
}

// ── StmtVisitor ────────────────────────────────────────────────────────────────

void Resolver::visitPrintStmt(PrintStmt& stmt) { resolveExpr(*stmt.expression); }
void Resolver::visitExprStmt (ExprStmt&  stmt) { resolveExpr(*stmt.expression); }

void Resolver::visitVarStmt(VarStmt& stmt) {
    declare(stmt.name);
    if (stmt.initializer) resolveExpr(*stmt.initializer);
    define(stmt.name);
}

void Resolver::visitBlockStmt(BlockStmt& stmt) {
    beginScope();
    resolveBlock(stmt.statements);
    endScope();
}

void Resolver::visitIfStmt(IfStmt& stmt) {
    resolveExpr(*stmt.condition);
    resolveStmt(*stmt.thenBranch);
    if (stmt.elseBranch) resolveStmt(*stmt.elseBranch);
}

void Resolver::visitForStmt(ForStmt& stmt) {
    beginScope(); // for 초기화 변수용 스코프
    if (stmt.initializer) resolveStmt(*stmt.initializer);
    if (stmt.condition)   resolveExpr(*stmt.condition);
    if (stmt.increment)   resolveExpr(*stmt.increment);
    resolveStmt(*stmt.body);
    endScope();
}
