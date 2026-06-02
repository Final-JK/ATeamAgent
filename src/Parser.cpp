#include "Parser.h"

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> stmts;
    while (!isAtEnd()) stmts.push_back(declaration());
    return stmts;
}

// ── Statements ─────────────────────────────────────────────────────────────────

StmtPtr Parser::declaration() {
    if (match({TokenType::VAR})) return varDeclaration();
    return statement();
}

StmtPtr Parser::varDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name.");
    ExprPtr init;
    if (match({TokenType::EQUAL})) init = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    return std::make_unique<VarStmt>(std::move(name), std::move(init));
}

StmtPtr Parser::statement() {
    if (match({TokenType::PRINT}))      return printStatement();
    if (match({TokenType::LEFT_BRACE})) return blockStatement();
    if (match({TokenType::IF}))         return ifStatement();
    if (match({TokenType::FOR}))        return forStatement();
    return expressionStatement();
}

StmtPtr Parser::printStatement() {
    ExprPtr value = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after value.");
    return std::make_unique<PrintStmt>(std::move(value));
}

StmtPtr Parser::blockStatement() {
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
        stmts.push_back(declaration());
    consume(TokenType::RIGHT_BRACE, "Expected '}' after block.");
    return std::make_unique<BlockStmt>(std::move(stmts));
}

StmtPtr Parser::ifStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'.");
    ExprPtr cond = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition.");
    StmtPtr then_ = statement();
    StmtPtr else_;
    if (match({TokenType::ELSE})) else_ = statement();
    return std::make_unique<IfStmt>(std::move(cond), std::move(then_), std::move(else_));
}

StmtPtr Parser::forStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'for'.");

    // initializer
    StmtPtr init;
    if      (match({TokenType::SEMICOLON})) { /* no init */ }
    else if (match({TokenType::VAR}))       init = varDeclaration();
    else                                    init = expressionStatement();

    // condition
    ExprPtr cond;
    if (!check(TokenType::SEMICOLON)) cond = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after for condition.");

    // increment
    ExprPtr incr;
    if (!check(TokenType::RIGHT_PAREN)) incr = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after for clauses.");

    StmtPtr body = statement();
    return std::make_unique<ForStmt>(std::move(init), std::move(cond), std::move(incr), std::move(body));
}

StmtPtr Parser::expressionStatement() {
    ExprPtr expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return std::make_unique<ExprStmt>(std::move(expr));
}

// ── Expressions ────────────────────────────────────────────────────────────────

ExprPtr Parser::expression() { return assignment(); }

ExprPtr Parser::assignment() {
    ExprPtr expr = comparison();
    if (match({TokenType::EQUAL})) {
        Token equals = previous();
        ExprPtr val  = assignment(); // right-associative
        if (auto* var = dynamic_cast<VariableExpr*>(expr.get())) {
            return std::make_unique<AssignExpr>(var->name, std::move(val));
        }
        throw error(equals, "Invalid assignment target.");
    }
    return expr;
}

ExprPtr Parser::comparison() {
    ExprPtr expr = addition();
    while (match({TokenType::LESS, TokenType::LESS_EQUAL,
                  TokenType::GREATER, TokenType::GREATER_EQUAL,
                  TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL})) {
        Token   op    = previous();
        ExprPtr right = addition();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

ExprPtr Parser::addition() {
    ExprPtr expr = multiplication();
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        Token   op    = previous();
        ExprPtr right = multiplication();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

ExprPtr Parser::multiplication() {
    ExprPtr expr = unary();
    while (match({TokenType::STAR, TokenType::SLASH})) {
        Token   op    = previous();
        ExprPtr right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }
    return expr;
}

ExprPtr Parser::unary() {
    if (match({TokenType::MINUS})) {
        Token   op    = previous();
        ExprPtr right = unary();
        return std::make_unique<UnaryExpr>(std::move(op), std::move(right));
    }
    return primary();
}

ExprPtr Parser::primary() {
    if (match({TokenType::NUMBER}))     return std::make_unique<LiteralExpr>(previous().literal);
    if (match({TokenType::STRING}))     return std::make_unique<LiteralExpr>(previous().literal);
    if (match({TokenType::TRUE_KW}))    return std::make_unique<LiteralExpr>(true);
    if (match({TokenType::FALSE_KW}))   return std::make_unique<LiteralExpr>(false);
    if (match({TokenType::IDENTIFIER})) return std::make_unique<VariableExpr>(previous());
    if (match({TokenType::LEFT_PAREN})) {
        ExprPtr expr = expression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }
    throw error(peek(), "Expected expression.");
}

// ── Helpers ────────────────────────────────────────────────────────────────────

bool Parser::check(TokenType type) const {
    return !isAtEnd() && tokens[current].type == type;
}
bool Parser::isAtEnd() const { return tokens[current].type == TokenType::EOF_TOKEN; }

Token& Parser::advance()  { if (!isAtEnd()) ++current; return tokens[current - 1]; }
Token& Parser::peek()     { return tokens[current]; }
Token& Parser::previous() { return tokens[current - 1]; }

bool Parser::match(std::initializer_list<TokenType> types) {
    for (auto t : types) { if (check(t)) { advance(); return true; } }
    return false;
}

Token Parser::consume(TokenType type, const std::string& msg) {
    if (check(type)) return advance();
    throw error(peek(), msg);
}

ParseError Parser::error(const Token& token, const std::string& msg) {
    std::string where = (token.type == TokenType::EOF_TOKEN)
        ? " at end"
        : " at '" + token.lexeme + "'";
    return ParseError("[line " + std::to_string(token.line) + "] Syntax Error" + where + ": " + msg);
}
