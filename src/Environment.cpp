#include "Environment.h"

void Environment::define(const std::string& name, Value value) {
    values[name] = std::move(value);
}

Value Environment::get(const Token& name) const {
    auto it = values.find(name.lexeme);
    if (it != values.end()) return it->second;
    if (enclosing)          return enclosing->get(name);
    throw RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
}

void Environment::assign(const Token& name, Value value) {
    auto it = values.find(name.lexeme);
    if (it != values.end()) { it->second = std::move(value); return; }
    if (enclosing)          { enclosing->assign(name, std::move(value)); return; }
    throw RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
}
