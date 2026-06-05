#include "Resolver.h"
#include <unordered_set>

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

// Ch.2: 함수 호출 - callee와 모든 인수를 resolve
Value Resolver::visitCallExpr(CallExpr& expr) {
    resolveExpr(*expr.callee);
    for (auto& arg : expr.args) resolveExpr(*arg);
    return nullptr;
}

// Ch.3: 배열 인덱스 읽기 - object와 index 표현식을 resolve
Value Resolver::visitIndexGetExpr(IndexGetExpr& expr) {
    resolveExpr(*expr.object);
    resolveExpr(*expr.index);
    return nullptr;
}

// Ch.3: 배열 인덱스 쓰기 - object, index, value 표현식을 resolve
Value Resolver::visitIndexSetExpr(IndexSetExpr& expr) {
    resolveExpr(*expr.object);
    resolveExpr(*expr.index);
    resolveExpr(*expr.value);
    return nullptr;
}

// Ch.3: 배열 생성 - size 표현식을 resolve
Value Resolver::visitArrayCreateExpr(ArrayCreateExpr& expr) {
    resolveExpr(*expr.size);
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

// Ch.2: 함수 선언 정적 검사
// - 함수 이름을 외부 스코프에 등록 (재귀 호출 허용)
// - 파라미터 이름 중복 검사
// - 함수 본문을 새 스코프에서 resolve (return 유효성 포함)
void Resolver::visitFuncStmt(FuncStmt& stmt) {
    declare(stmt.name);
    define(stmt.name);

    // 함수 본문 분석 시 currentFunction을 FUNCTION으로 설정
    FunctionType enclosing = currentFunction;
    currentFunction = FunctionType::FUNCTION;
    beginScope();

    // 파라미터 중복 이름 검사
    std::unordered_set<std::string> seen;
    for (const auto& param : stmt.params) {
        if (!seen.insert(param.lexeme).second)
            throw StaticError("[line " + std::to_string(param.line) +
                "] Static Error: Duplicate parameter name '" + param.lexeme + "'.");
        declare(param);
        define(param);
    }

    resolveBlock(stmt.body);
    endScope();
    // 중첩 함수를 지원하기 위해 외부 컨텍스트 복원
    currentFunction = enclosing;
}

// Ch.2: return 문 정적 검사
// - 함수 외부(최상위 코드)에서 return을 사용하면 Static Error
void Resolver::visitReturnStmt(ReturnStmt& stmt) {
    if (currentFunction == FunctionType::NONE)
        throw StaticError("[line " + std::to_string(stmt.keyword.line) +
            "] Static Error: Can't return from top-level code.");
    if (stmt.value) resolveExpr(*stmt.value);
}
