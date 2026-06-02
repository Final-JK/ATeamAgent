#pragma once
#include "Stmt.h"
#include "Environment.h"
#include <memory>
#include <vector>

class Interpreter : public ExprVisitor, public StmtVisitor {
public:
    Interpreter();
    void interpret(const std::vector<StmtPtr>& statements);
    void executeBlock(const std::vector<StmtPtr>& stmts, std::shared_ptr<Environment> env);

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
