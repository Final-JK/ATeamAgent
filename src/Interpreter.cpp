#include "Interpreter.h"
#include <iostream>
#include <sstream>
#include <cmath>

Interpreter::Interpreter() {
    globalEnv  = std::make_shared<Environment>();
    currentEnv = globalEnv;
}

void Interpreter::interpret(const std::vector<StmtPtr>& statements) {
    for (const auto& stmt : statements) execute(*stmt);
}

void Interpreter::execute(Stmt& stmt) { stmt.accept(*this); }
Value Interpreter::evaluate(Expr& expr) { return expr.accept(*this); }

// ── Utilities ──────────────────────────────────────────────────────────────────

bool Interpreter::isTruthy(const Value& v) {
    if (std::holds_alternative<std::nullptr_t>(v)) return false;
    if (std::holds_alternative<bool>(v))           return std::get<bool>(v);
    return true;
}

bool Interpreter::isEqual(const Value& a, const Value& b) {
    if (a.index() != b.index()) return false;
    if (std::holds_alternative<std::nullptr_t>(a)) return true;
    if (std::holds_alternative<bool>(a))   return std::get<bool>(a)   == std::get<bool>(b);
    if (std::holds_alternative<double>(a)) return std::get<double>(a) == std::get<double>(b);
    if (std::holds_alternative<std::string>(a))
        return std::get<std::string>(a) == std::get<std::string>(b);
    return false;
}

std::string Interpreter::stringify(const Value& v) {
    if (std::holds_alternative<std::nullptr_t>(v)) return "nil";
    if (std::holds_alternative<bool>(v))  return std::get<bool>(v) ? "true" : "false";
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        // 정수면 소수점 없이 출력 (5.0 → "5")
        if (!std::isinf(d) && d == std::floor(d))
            return std::to_string(static_cast<long long>(d));
        std::ostringstream oss;
        oss << d;
        return oss.str();
    }
    return "";
}

void Interpreter::checkNumberOperand(const Token& op, const Value& operand) {
    if (std::holds_alternative<double>(operand)) return;
    throw RuntimeError(op, "Operand must be a number.");
}

void Interpreter::checkNumberOperands(const Token& op, const Value& left, const Value& right) {
    if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) return;
    throw RuntimeError(op, "Operands must be numbers.");
}

void Interpreter::executeBlock(const std::vector<StmtPtr>& stmts,
                               std::shared_ptr<Environment> env) {
    auto saved = currentEnv;
    currentEnv = std::move(env);
    try {
        for (const auto& s : stmts) execute(*s);
    } catch (...) {
        currentEnv = saved;
        throw;
    }
    currentEnv = saved;
}

// ── ExprVisitor ────────────────────────────────────────────────────────────────

Value Interpreter::visitLiteralExpr(LiteralExpr& expr) { return expr.value; }

Value Interpreter::visitGroupingExpr(GroupingExpr& expr) {
    return evaluate(*expr.expression);
}

Value Interpreter::visitUnaryExpr(UnaryExpr& expr) {
    Value right = evaluate(*expr.right);
    if (expr.op.type == TokenType::MINUS) {
        checkNumberOperand(expr.op, right);
        return -std::get<double>(right);
    }
    return nullptr;
}

Value Interpreter::visitBinaryExpr(BinaryExpr& expr) {
    Value left  = evaluate(*expr.left);
    Value right = evaluate(*expr.right);

    switch (expr.op.type) {
    case TokenType::PLUS:
        if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right))
            return std::get<double>(left) + std::get<double>(right);
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
            return std::get<std::string>(left) + std::get<std::string>(right);
        throw RuntimeError(expr.op, "Operands must be two numbers or two strings.");
    case TokenType::MINUS:
        checkNumberOperands(expr.op, left, right);
        return std::get<double>(left) - std::get<double>(right);
    case TokenType::STAR:
        checkNumberOperands(expr.op, left, right);
        return std::get<double>(left) * std::get<double>(right);
    case TokenType::SLASH:
        checkNumberOperands(expr.op, left, right);
        return std::get<double>(left) / std::get<double>(right);
    case TokenType::LESS:
        checkNumberOperands(expr.op, left, right);
        return std::get<double>(left) < std::get<double>(right);
    case TokenType::LESS_EQUAL:
        checkNumberOperands(expr.op, left, right);
        return std::get<double>(left) <= std::get<double>(right);
    case TokenType::GREATER:
        checkNumberOperands(expr.op, left, right);
        return std::get<double>(left) > std::get<double>(right);
    case TokenType::GREATER_EQUAL:
        checkNumberOperands(expr.op, left, right);
        return std::get<double>(left) >= std::get<double>(right);
    case TokenType::EQUAL_EQUAL:
        return isEqual(left, right);
    case TokenType::BANG_EQUAL:
        return !isEqual(left, right);
    default:
        break;
    }
    return nullptr;
}

Value Interpreter::visitVariableExpr(VariableExpr& expr) {
    return currentEnv->get(expr.name);
}

Value Interpreter::visitAssignExpr(AssignExpr& expr) {
    Value val = evaluate(*expr.value);
    currentEnv->assign(expr.name, val);
    return val;
}

// ── StmtVisitor ────────────────────────────────────────────────────────────────

void Interpreter::visitPrintStmt(PrintStmt& stmt) {
    std::cout << stringify(evaluate(*stmt.expression)) << "\n";
}

void Interpreter::visitExprStmt(ExprStmt& stmt) {
    evaluate(*stmt.expression);
}

void Interpreter::visitVarStmt(VarStmt& stmt) {
    Value val = nullptr;
    if (stmt.initializer) val = evaluate(*stmt.initializer);
    currentEnv->define(stmt.name.lexeme, std::move(val));
}

void Interpreter::visitBlockStmt(BlockStmt& stmt) {
    executeBlock(stmt.statements, std::make_shared<Environment>(currentEnv));
}

void Interpreter::visitIfStmt(IfStmt& stmt) {
    if (isTruthy(evaluate(*stmt.condition)))
        execute(*stmt.thenBranch);
    else if (stmt.elseBranch)
        execute(*stmt.elseBranch);
}

void Interpreter::visitForStmt(ForStmt& stmt) {
    // for 초기화 변수를 위한 새 스코프
    auto loopEnv = std::make_shared<Environment>(currentEnv);
    auto saved   = currentEnv;
    currentEnv   = loopEnv;
    try {
        if (stmt.initializer) execute(*stmt.initializer);
        while (true) {
            if (stmt.condition && !isTruthy(evaluate(*stmt.condition))) break;
            execute(*stmt.body);
            if (stmt.increment) evaluate(*stmt.increment);
        }
    } catch (...) {
        currentEnv = saved;
        throw;
    }
    currentEnv = saved;
}
