#include "Parser.h"

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> stmts;
    while (!isAtEnd()) stmts.push_back(declaration());
    return stmts;
}

// ── Statements ─────────────────────────────────────────────────────────────────

StmtPtr Parser::declaration() {
    if (match({TokenType::FUNC})) return funcDeclaration();  // Ch.2: 함수 선언
    if (match({TokenType::VAR}))  return varDeclaration();
    return statement();
}

// Ch.2: 함수 선언 파싱 - Func name(param1, param2) { body }
StmtPtr Parser::funcDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name.");
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name.");

    std::vector<Token> params;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name."));
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters.");
    consume(TokenType::LEFT_BRACE, "Expected '{' before function body.");

    std::vector<StmtPtr> body;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
        body.push_back(declaration());
    consume(TokenType::RIGHT_BRACE, "Expected '}' after function body.");

    return std::make_unique<FuncStmt>(std::move(name), std::move(params), std::move(body));
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
    if (match({TokenType::RETURN}))     return returnStatement();  // Ch.2: return 문
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

// Ch.2: return 문 파싱 - return [expr] ;
// 세미콜론 전에 표현식이 있으면 반환값, 없으면 nil 반환
StmtPtr Parser::returnStatement() {
    Token keyword = previous();
    ExprPtr value;
    if (!check(TokenType::SEMICOLON)) value = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after return value.");
    return std::make_unique<ReturnStmt>(std::move(keyword), std::move(value));
}

StmtPtr Parser::expressionStatement() {
    ExprPtr expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return std::make_unique<ExprStmt>(std::move(expr));
}

// ── Expressions ────────────────────────────────────────────────────────────────

ExprPtr Parser::expression() { return assignment(); }

// Ch.3: 배열 쓰기 - arr[i] = value
// IndexGetExpr 좌변을 감지해 IndexSetExpr로 변환
ExprPtr Parser::assignment() {
    ExprPtr expr = comparison();
    if (match({TokenType::EQUAL})) {
        Token   equals = previous();
        ExprPtr val    = assignment(); // right-associative
        if (auto* var = dynamic_cast<VariableExpr*>(expr.get())) {
            return std::make_unique<AssignExpr>(var->name, std::move(val));
        }
        // Ch.3: arr[i] = value → IndexSetExpr
        if (auto* idx = dynamic_cast<IndexGetExpr*>(expr.get())) {
            ExprPtr obj    = std::move(idx->object);
            ExprPtr index  = std::move(idx->index);
            Token   bracket = idx->bracket;
            return std::make_unique<IndexSetExpr>(
                std::move(obj), std::move(index), std::move(val), std::move(bracket));
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
    return call();  // Ch.2/3: 단항 연산 이후 호출/인덱스 후위 처리
}

// Ch.2/3: 함수 호출과 배열 인덱스 접근을 후위(postfix) 방식으로 파싱
// primary() 이후 '(' 또는 '[' 가 반복될 때마다 새 노드로 감쌈
// 예) foo(1)(2)[0] 같은 체이닝도 자연스럽게 처리됨
ExprPtr Parser::call() {
    ExprPtr expr = primary();
    while (true) {
        if (match({TokenType::LEFT_PAREN})) {
            // Ch.2: 함수 호출 후위 처리
            expr = finishCall(std::move(expr));
        } else if (match({TokenType::LEFT_BRACKET})) {
            // Ch.3: 배열 인덱스 읽기 후위 처리
            Token   bracket = previous();
            ExprPtr index   = expression();
            consume(TokenType::RIGHT_BRACKET, "Expected ']' after index.");
            expr = std::make_unique<IndexGetExpr>(std::move(expr), std::move(index), std::move(bracket));
        } else {
            break;
        }
    }
    return expr;
}

// Ch.2: 함수 호출 인수 파싱 완성
// '(' 이후 인수 목록을 파싱하고 CallExpr를 반환
ExprPtr Parser::finishCall(ExprPtr callee) {
    Token paren = previous();  // '(' 토큰 (런타임 오류 위치 추적용)
    std::vector<ExprPtr> args;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            args.push_back(expression());
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments.");
    return std::make_unique<CallExpr>(std::move(callee), std::move(paren), std::move(args));
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
    // Ch.3: Array(n) 배열 생성 표현식
    // '(' size ')' 를 여기서 직접 파싱하므로 call()의 후위 처리와 별도로 동작
    if (match({TokenType::ARRAY_KW})) {
        Token keyword = previous();
        consume(TokenType::LEFT_PAREN, "Expected '(' after 'Array'.");
        ExprPtr sizeExpr = expression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after array size.");
        return std::make_unique<ArrayCreateExpr>(std::move(keyword), std::move(sizeExpr));
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
