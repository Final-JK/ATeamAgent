#pragma once
#include "Token.h"
#include <vector>
#include <stdexcept>

struct LexError : std::runtime_error {
    int line;
    LexError(int line, const std::string& msg)
        : std::runtime_error("[line " + std::to_string(line) + "] Syntax Error: " + msg)
        , line(line) {}
};

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> scanTokens();

private:
    std::string        source;
    std::vector<Token> tokens;
    int start   = 0;
    int current = 0;
    int line    = 1;

    bool isAtEnd() const;
    char advance();
    bool match(char expected);
    char peek() const;
    char peekNext() const;
    void addToken(TokenType type, Value literal = nullptr);
    void scanToken();
    void readString();
    void readNumber();
    void readIdentifier();

    static bool isDigit(char c);
    static bool isAlpha(char c);
    static bool isAlphaNumeric(char c);
};
