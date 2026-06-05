#pragma once
#include "Token.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>

// ── Runtime Error ──────────────────────────────────────────────────────────────
struct RuntimeError : std::runtime_error {
    Token token;
    RuntimeError(Token tok, const std::string& msg)
        : std::runtime_error(msg), token(std::move(tok)) {}
};

// ── Lexical Environment (scope chain) ─────────────────────────────────────────
class Environment {
public:
    std::shared_ptr<Environment> enclosing;

    explicit Environment(std::shared_ptr<Environment> enc = nullptr)
        : enclosing(std::move(enc)) {}

    void  define(const std::string& name, Value value);
    Value get   (const Token& name) const;
    void  assign(const Token& name, Value value);

    // Ch.5 디버그 모드: 이 스코프에 정의된 변수 목록을 반환한다.
    // inspect 명령에서 현재 스코프 전체 변수를 출력할 때 사용.
    const std::unordered_map<std::string, Value>& getValues() const { return values; }

private:
    std::unordered_map<std::string, Value> values;
};
