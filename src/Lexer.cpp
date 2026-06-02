#include "Lexer.h"
#include <unordered_map>

static const std::unordered_map<std::string, TokenType> keywords = {
    {"var",   TokenType::VAR},
    {"if",    TokenType::IF},
    {"else",  TokenType::ELSE},
    {"for",   TokenType::FOR},
    {"print", TokenType::PRINT},
    {"true",  TokenType::TRUE_KW},
    {"false", TokenType::FALSE_KW},
};

Lexer::Lexer(std::string source) : source(std::move(source)) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }
    tokens.emplace_back(TokenType::EOF_TOKEN, "", nullptr, line);
    return std::move(tokens);
}

bool Lexer::isAtEnd() const { return current >= (int)source.size(); }
char Lexer::advance()       { return source[current++]; }

bool Lexer::match(char expected) {
    if (isAtEnd() || source[current] != expected) return false;
    ++current;
    return true;
}

char Lexer::peek() const     { return isAtEnd() ? '\0' : source[current]; }
char Lexer::peekNext() const { return (current + 1 >= (int)source.size()) ? '\0' : source[current + 1]; }

void Lexer::addToken(TokenType type, Value literal) {
    tokens.emplace_back(type, source.substr(start, current - start), std::move(literal), line);
}

void Lexer::scanToken() {
    char c = advance();
    switch (c) {
    case '(': addToken(TokenType::LEFT_PAREN);  break;
    case ')': addToken(TokenType::RIGHT_PAREN); break;
    case '{': addToken(TokenType::LEFT_BRACE);  break;
    case '}': addToken(TokenType::RIGHT_BRACE); break;
    case ';': addToken(TokenType::SEMICOLON);   break;
    case '+': addToken(TokenType::PLUS);        break;
    case '-': addToken(TokenType::MINUS);       break;
    case '*': addToken(TokenType::STAR);        break;
    case '/':
        if (match('/')) { while (peek() != '\n' && !isAtEnd()) advance(); }
        else            addToken(TokenType::SLASH);
        break;
    case '<': addToken(match('=') ? TokenType::LESS_EQUAL    : TokenType::LESS);    break;
    case '>': addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER); break;
    case '=': addToken(match('=') ? TokenType::EQUAL_EQUAL   : TokenType::EQUAL);   break;
    case '!':
        if (match('=')) addToken(TokenType::BANG_EQUAL);
        else throw LexError(line, std::string("Unexpected character '!'."));
        break;
    case '"': readString(); break;
    case ' ': case '\r': case '\t': break;
    case '\n': ++line; break;
    default:
        if      (isDigit(c)) readNumber();
        else if (isAlpha(c)) readIdentifier();
        else throw LexError(line, std::string("Unexpected character '") + c + "'.");
        break;
    }
}

void Lexer::readString() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') ++line;
        advance();
    }
    if (isAtEnd()) throw LexError(line, "Unterminated string.");
    advance(); // closing "
    addToken(TokenType::STRING, source.substr(start + 1, current - start - 2));
}

void Lexer::readNumber() {
    while (isDigit(peek())) advance();
    if (peek() == '.' && isDigit(peekNext())) {
        advance();
        while (isDigit(peek())) advance();
    }
    addToken(TokenType::NUMBER, std::stod(source.substr(start, current - start)));
}

void Lexer::readIdentifier() {
    while (isAlphaNumeric(peek())) advance();
    std::string text = source.substr(start, current - start);
    auto it = keywords.find(text);
    if (it == keywords.end()) { addToken(TokenType::IDENTIFIER); return; }

    TokenType kw = it->second;
    if      (kw == TokenType::TRUE_KW)  addToken(kw, true);
    else if (kw == TokenType::FALSE_KW) addToken(kw, false);
    else                                addToken(kw);
}

bool Lexer::isDigit(char c)       { return c >= '0' && c <= '9'; }
bool Lexer::isAlpha(char c)       { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool Lexer::isAlphaNumeric(char c){ return isAlpha(c) || isDigit(c); }
