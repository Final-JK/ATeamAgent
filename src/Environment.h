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

private:
    std::unordered_map<std::string, Value> values;
};
