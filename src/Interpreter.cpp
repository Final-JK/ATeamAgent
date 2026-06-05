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

void Interpreter::execute(Stmt& stmt) {
    // Ch.5: 디버그 모드일 때 Stmt 실행 직전에 훅을 호출한다.
    // stmtHook이 nullptr이면 일반 실행(파일/REPL 모드)으로 동작.
    if (stmtHook) stmtHook(stmt);
    stmt.accept(*this);
}
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
    // Ch.2: 함수 동등 비교는 포인터 동등성 (같은 선언 객체인지)
    if (std::holds_alternative<std::shared_ptr<FabFunction>>(a))
        return std::get<std::shared_ptr<FabFunction>>(a) == std::get<std::shared_ptr<FabFunction>>(b);
    // Ch.3: 배열 동등 비교는 포인터 동등성 (같은 배열 인스턴스인지)
    if (std::holds_alternative<std::shared_ptr<FabArray>>(a))
        return std::get<std::shared_ptr<FabArray>>(a) == std::get<std::shared_ptr<FabArray>>(b);
    return false;
}

std::string Interpreter::stringify(const Value& v) {
    if (std::holds_alternative<std::nullptr_t>(v)) return "nil";
    if (std::holds_alternative<bool>(v))           return std::get<bool>(v) ? "true" : "false";
    if (std::holds_alternative<std::string>(v))    return std::get<std::string>(v);
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        // 정수면 소수점 없이 출력 (5.0 → "5")
        if (!std::isinf(d) && d == std::floor(d))
            return std::to_string(static_cast<long long>(d));
        std::ostringstream oss;
        oss << d;
        return oss.str();
    }
    // Ch.2: 함수는 이름 포함 표현으로 출력
    if (std::holds_alternative<std::shared_ptr<FabFunction>>(v))
        return "<function " + std::get<std::shared_ptr<FabFunction>>(v)->name + ">";
    // Ch.3: 배열은 크기 포함 표현으로 출력
    if (std::holds_alternative<std::shared_ptr<FabArray>>(v))
        return "<array[" + std::to_string(std::get<std::shared_ptr<FabArray>>(v)->elements.size()) + "]>";
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
        // Ch.2: ReturnSignal 포함 모든 예외에서 currentEnv 복원 후 재전파
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

// Ch.2: 함수 호출 실행
// 1) callee가 FabFunction인지 타입 검사 (아니면 Runtime Error)
// 2) 인수 개수가 파라미터 개수와 일치하는지 검사
// 3) 클로저 환경을 parent로 하는 새 환경에서 함수 본문 실행
// 4) ReturnSignal 예외로 반환값을 전달받음
Value Interpreter::visitCallExpr(CallExpr& expr) {
    Value callee = evaluate(*expr.callee);

    if (!std::holds_alternative<std::shared_ptr<FabFunction>>(callee))
        throw RuntimeError(expr.paren, "Can only call functions.");

    auto& fn = *std::get<std::shared_ptr<FabFunction>>(callee);

    std::vector<Value> args;
    args.reserve(expr.args.size());
    for (auto& arg : expr.args) args.push_back(evaluate(*arg));

    if ((int)args.size() != fn.arity())
        throw RuntimeError(expr.paren,
            "Expected " + std::to_string(fn.arity()) +
            " arguments but got " + std::to_string(args.size()) + ".");

    // 클로저 환경을 parent로 하는 새 환경 (렉시컬 스코핑)
    auto callEnv = std::make_shared<Environment>(fn.closure);
    for (int i = 0; i < fn.arity(); ++i)
        callEnv->define(fn.params[i].lexeme, std::move(args[i]));

    try {
        executeBlock(*fn.body, callEnv);
    } catch (ReturnSignal& ret) {
        return ret.value;
    }
    return nullptr;  // 명시적 return 없으면 nil 반환
}

// Ch.3: 배열 인덱스 읽기 실행
// 런타임 오류: 배열 타입 아님 / 인덱스가 숫자 아님 / 범위 초과
Value Interpreter::visitIndexGetExpr(IndexGetExpr& expr) {
    Value obj = evaluate(*expr.object);
    Value idx = evaluate(*expr.index);

    if (!std::holds_alternative<std::shared_ptr<FabArray>>(obj))
        throw RuntimeError(expr.bracket, "Only arrays support index access.");

    if (!std::holds_alternative<double>(idx))
        throw RuntimeError(expr.bracket, "Index must be a number.");

    auto& arr = std::get<std::shared_ptr<FabArray>>(obj);
    int i = static_cast<int>(std::get<double>(idx));
    if (i < 0 || i >= (int)arr->elements.size())
        throw RuntimeError(expr.bracket,
            "Index " + std::to_string(i) + " out of bounds (size " +
            std::to_string(arr->elements.size()) + ").");

    return arr->elements[i];
}

// Ch.3: 배열 인덱스 쓰기 실행
// 런타임 오류: 배열 타입 아님 / 인덱스가 숫자 아님 / 범위 초과
Value Interpreter::visitIndexSetExpr(IndexSetExpr& expr) {
    Value obj = evaluate(*expr.object);
    Value idx = evaluate(*expr.index);

    if (!std::holds_alternative<std::shared_ptr<FabArray>>(obj))
        throw RuntimeError(expr.bracket, "Only arrays support index access.");

    if (!std::holds_alternative<double>(idx))
        throw RuntimeError(expr.bracket, "Index must be a number.");

    auto& arr = std::get<std::shared_ptr<FabArray>>(obj);
    int i = static_cast<int>(std::get<double>(idx));
    if (i < 0 || i >= (int)arr->elements.size())
        throw RuntimeError(expr.bracket,
            "Index " + std::to_string(i) + " out of bounds (size " +
            std::to_string(arr->elements.size()) + ").");

    Value val = evaluate(*expr.value);
    arr->elements[i] = val;
    return val;
}

// Ch.3: 배열 생성 실행 - Array(n)
// 런타임 오류: 크기가 숫자가 아닌 경우
Value Interpreter::visitArrayCreateExpr(ArrayCreateExpr& expr) {
    Value sizeVal = evaluate(*expr.size);
    if (!std::holds_alternative<double>(sizeVal))
        throw RuntimeError(expr.keyword, "Array size must be a number.");
    int size = static_cast<int>(std::get<double>(sizeVal));
    if (size < 0)
        throw RuntimeError(expr.keyword, "Array size must be non-negative.");
    return std::make_shared<FabArray>(size);
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

// Ch.2: 함수 선언 실행
// FabFunction 객체를 생성하고 현재 환경에 함수 이름으로 등록
// closure에 선언 시점의 환경을 캡처하여 렉시컬 스코핑을 구현
void Interpreter::visitFuncStmt(FuncStmt& stmt) {
    auto fn = std::make_shared<FabFunction>();
    fn->name    = stmt.name.lexeme;
    fn->params  = stmt.params;
    fn->body    = &stmt.body;
    fn->closure = currentEnv;  // 선언 시점의 환경 캡처
    currentEnv->define(stmt.name.lexeme, fn);
}

// Ch.2: return 문 실행
// ReturnSignal 예외를 throw하여 호출 스택을 되감음
// visitCallExpr의 catch 블록에서 반환값을 추출
void Interpreter::visitReturnStmt(ReturnStmt& stmt) {
    Value val = nullptr;
    if (stmt.value) val = evaluate(*stmt.value);
    throw ReturnSignal{ val };
}
