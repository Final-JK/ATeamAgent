#pragma once
#include "FabValue.h"   // FabFunction, FabArray, ReturnSignal (Environment.h 포함)
#include "Stmt.h"
#include <functional>
#include <memory>
#include <vector>

class Interpreter : public ExprVisitor, public StmtVisitor {
public:
    Interpreter();
    void interpret(const std::vector<StmtPtr>& statements);
    void executeBlock(const std::vector<StmtPtr>& stmts, std::shared_ptr<Environment> env);

    // Ch.5 디버그 모드: 각 Stmt 실행 직전에 호출되는 콜백.
    // nullptr이면 일반 실행(파일/REPL 모드).
    // DebugRunner가 이 훅을 설정해 stepping·breakpoint 로직을 삽입한다.
    std::function<void(const Stmt&)> stmtHook;

    // Ch.5 디버그 모드: currentEnv 외부 접근용 (inspect/watch에서 사용)
    std::shared_ptr<Environment> getCurrentEnv() const { return currentEnv; }

private:
    std::shared_ptr<Environment> globalEnv;
    std::shared_ptr<Environment> currentEnv;

    Value evaluate(Expr& expr);
    void  execute (Stmt& stmt);

    bool        isTruthy(const Value& v);
    bool        isEqual (const Value& a, const Value& b);
    std::string stringify(const Value& v);
    void checkNumberOperand (const Token& op, const Value& operand);
    void checkNumberOperands(const Token& op, const Value& left, const Value& right);

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
