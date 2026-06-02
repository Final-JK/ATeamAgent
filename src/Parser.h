#pragma once
#include "Stmt.h"
#include <vector>
#include <stdexcept>
#include <initializer_list>

// ── Parse Error ────────────────────────────────────────────────────────────────
struct ParseError : std::runtime_error {
    explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

// ── Recursive Descent Parser ───────────────────────────────────────────────────
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::vector<StmtPtr> parse();

private:
    std::vector<Token> tokens;
    int current = 0;

    // ── Statement grammar ──────────────────────────────────────────────────────
    StmtPtr declaration();
    StmtPtr varDeclaration();
    StmtPtr statement();
    StmtPtr printStatement();
    StmtPtr blockStatement();
    StmtPtr ifStatement();
    StmtPtr forStatement();
    StmtPtr expressionStatement();

    // ── Expression grammar ─────────────────────────────────────────────────────
    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr comparison();
    ExprPtr addition();
    ExprPtr multiplication();
    ExprPtr unary();
    ExprPtr primary();

    // ── Helpers ────────────────────────────────────────────────────────────────
    bool    check(TokenType type) const;
    bool    isAtEnd() const;
    Token&  advance();
    Token&  peek();
    Token&  previous();
    bool    match(std::initializer_list<TokenType> types);
    Token   consume(TokenType type, const std::string& msg);
    ParseError error(const Token& token, const std::string& msg);
};
