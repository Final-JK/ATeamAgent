#pragma once
#include <string>
#include <variant>
#include <memory>

enum class TokenType {
    // Single-char tokens
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET,  // Ch.3: 배열 인덱스 접근 [ ]
    SEMICOLON,
    COMMA,                         // Ch.2: 함수 파라미터/인수 구분자 ,
    PLUS, MINUS, STAR, SLASH,

    // One or two char tokens
    LESS, LESS_EQUAL,
    GREATER, GREATER_EQUAL,
    EQUAL, EQUAL_EQUAL,
    BANG_EQUAL,

    // Literals
    NUMBER, STRING, IDENTIFIER,

    // Keywords
    VAR, IF, ELSE, FOR, PRINT,
    TRUE_KW, FALSE_KW,
    FUNC,      // Ch.2: 함수 선언 키워드 "Func"
    RETURN,    // Ch.2: 함수 반환 키워드 "return"
    ARRAY_KW,  // Ch.3: 배열 생성 키워드 "Array"

    EOF_TOKEN
};

// Ch.2/3: 함수와 배열은 런타임 값이므로 Value variant에 포함
// 순환 의존성 방지를 위해 forward declaration 후 shared_ptr로 보관
struct FabFunction;
struct FabArray;

using Value = std::variant<
    double,
    std::string,
    bool,
    std::nullptr_t,
    std::shared_ptr<FabFunction>,  // Ch.2: 사용자 정의 함수 타입
    std::shared_ptr<FabArray>      // Ch.3: 고정 크기 배열 타입
>;

struct Token {
    TokenType   type;
    std::string lexeme;
    Value       literal;
    int         line;

    Token(TokenType type, std::string lexeme, Value literal, int line)
        : type(type), lexeme(std::move(lexeme)), literal(std::move(literal)), line(line) {}
};
