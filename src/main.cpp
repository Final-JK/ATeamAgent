#include <iostream>
#include <fstream>
#include <sstream>
#include "Lexer.h"
#include "Parser.h"
#include "Resolver.h"
#include "Interpreter.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: interpreter <script>\n";
        return 64;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << argv[1] << "\n";
        return 66;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    try {
        Lexer lexer(source);
        auto  tokens = lexer.scanTokens();

        Parser parser(std::move(tokens));
        auto   stmts = parser.parse();

        Resolver resolver;
        resolver.resolve(stmts);

        Interpreter interpreter;
        interpreter.interpret(stmts);

    } catch (const LexError& e) {
        std::cerr << e.what() << "\n";
        return 65;
    } catch (const ParseError& e) {
        std::cerr << e.what() << "\n";
        return 65;
    } catch (const StaticError& e) {
        std::cerr << e.what() << "\n";
        return 65;
    } catch (const RuntimeError& e) {
        std::cerr << "[line " << e.token.line << "] Runtime Error: " << e.what() << "\n";
        return 70;
    }

    return 0;
}
